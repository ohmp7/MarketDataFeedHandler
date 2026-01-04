#include "exchange.h"

#include "endian.h"
#include "moldudp64.h"
#include "udp_messenger.h"

#include <chrono>
#include <condition_variable>
#include <cstring>
#include <deque>
#include <iostream>
#include <mutex>
#include <random>
#include <stdexcept>
#include <thread>
#include <unordered_map>
#include <vector>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>

BookState::BookState() {
    avail_prices.reserve(kMaxPrice);

    for (int i = 1; i <= kMaxPrice; ++i) {
        avail_prices.push_back(static_cast<Price>(i));
    }
}

ExchangeSimulator::ExchangeSimulator()
    : sockfd_(socket(AF_INET, SOCK_DGRAM, 0)),
      config_(ExchangeConfig::New()),
      generate_id_(config_.min_instrument_id, config_.max_instrument_id),
      generate_side_(0, 1),
      generate_event_(1, 100),
      generate_price_(config_.min_price, config_.max_price),
      generate_quantity_(config_.min_quantity, config_.max_quantity),
      generate_interval_(config_.min_interval_ms, config_.max_interval_ms) {

    if (sockfd_ < 0) throw std::runtime_error("Error: socket creation to exchange failed.");

    memset(&plantaddr_, 0, sizeof(plantaddr_));
    plantaddr_.sin_family = AF_INET;
    plantaddr_.sin_port = htons(config_.exchange_port);
    plantaddr_.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sockfd_, reinterpret_cast<sockaddr*>(&plantaddr_), sizeof(plantaddr_)) < 0) {
        close(sockfd_);
        sockfd_ = -1;
        throw std::runtime_error("Error: bind failed.");
    }

    {
        std::lock_guard<std::mutex> lock(history_mutex_);
        events_history_.resize(kMaxExchangeEvents);
    }
}

void ExchangeSimulator::SendDatagrams() {
    UdpMessenger messenger(sockfd_, config_.plant_ip.c_str(), config_.plant_port);

    while (true) {

        // wait for event
        std::unique_lock<std::mutex> lock(queue_mutex_);
        while (events_queue_.empty()) {
            cv_.wait(lock);
        }

        EventToSend next = events_queue_.front(); 
        events_queue_.pop_front();

        lock.unlock();

        std::uint8_t buf[kPacketSize];
        SerializeEvent(buf, next);

        messenger.SendDatagram(buf, kPacketSize);
    }
}

void ExchangeSimulator::GenerateMarketEvents() {
    while (true) {
        const InstrumentId id = static_cast<InstrumentId>(generate_id_(number_generator_));
        const Side side = static_cast<Side>(generate_side_(number_generator_));
        BookState& book = GetBook(id, side);

        const bool add_level = book.levels.empty() || generate_event_(number_generator_) <= config_.chance_of_add;
        
        MarketEvent e{};

        if (add_level) {
            // update live state
            const Quantity quantity = static_cast<Quantity>(generate_quantity_(number_generator_));

            // decide new price or existing price
            const bool new_price = generate_event_(number_generator_) <= config_.chance_of_new_price;

            Price price;

            if (book.levels.empty() || new_price) {
                price = PickNewPrice(book.avail_prices);
                book.levels[price] = quantity;
            } else {
                auto it = PickExistingPrice(book);
                price = it->first;
                book.levels[price] += quantity;
            }
            
            e.instrument_id = id;
            e.side = side;
            e.event = LevelEvent::kAddLevel;
            e.price = price;
            e.quantity = quantity;
            e.exchange_ts = CurrentTime();
            
        } else {
            auto it = PickExistingPrice(book);
            const Price price = it->first;
            const Quantity curr_quantity = it->second;

            const bool delete_level = generate_event_(number_generator_) <= config_.chance_of_delete;
            Quantity quantity_to_remove;

            if (delete_level) {
                quantity_to_remove = curr_quantity;
                ReleasePrice(book, price);
            } else {
                std::uniform_int_distribution<Quantity> generate_quantity_to_remove(1, curr_quantity - 1);
                quantity_to_remove = generate_quantity_to_remove(number_generator_);
                it->second -= quantity_to_remove;
            }

            e.instrument_id = id;
            e.side = side;
            e.event = LevelEvent::kModifyLevel;
            e.price = price;
            e.quantity = quantity_to_remove;
            e.exchange_ts = CurrentTime();
        }
        
        SequenceNumber seq;
        {
            std::lock_guard<std::mutex> lock(history_mutex_);
            seq = sequence_number_++;
            events_history_[seq] = e;
        }

        EnqueueEvent(e, seq);
        
        Timestamp sleep = static_cast<Timestamp>(generate_interval_(number_generator_));
        std::this_thread::sleep_for(std::chrono::milliseconds(sleep));
    }
}

void ExchangeSimulator::Retransmitter() {
    PacketHeader header;
    while (true) {
        std::uint8_t buf[kHeaderLength];
        
        ssize_t bytes_received = recvfrom(sockfd_, buf, kHeaderLength, 0, nullptr, nullptr);
        if (bytes_received <= 0) continue;

        try {
            header = ParsePacketHeader(buf, static_cast<Bytes>(bytes_received));
        } catch (const PacketTruncatedError& e) {
            std::cerr << e.what() << "\n";
            continue;
        }

        if (std::memcmp(header.session, session_, kSessionLength) == 0) {
            for (MessageCount i = 0; i < header.message_count; ++i) {
                std::lock_guard<std::mutex> lock(history_mutex_);

                const SequenceNumber seq = header.sequence_number + i;
                if (seq >= sequence_number_) break;
                EnqueueEvent(events_history_[seq], seq);
            }
        }
    }
}

void ExchangeSimulator::EnqueueEvent(const MarketEvent& e, const SequenceNumber sequence_number) {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    events_queue_.push_back(EventToSend{e, sequence_number});
    cv_.notify_one();
}

Price ExchangeSimulator::PickNewPrice(std::vector<Price>& avail_prices) {
    std::uniform_int_distribution<std::size_t> generate_idx(0, avail_prices.size() - 1);
    std::size_t i = generate_idx(number_generator_);

    Price p = avail_prices[i];
    avail_prices[i] = avail_prices.back();
    avail_prices.pop_back();
    return p;
}

std::unordered_map<Price, Quantity>::iterator ExchangeSimulator::PickExistingPrice(BookState& book) {
    std::uniform_int_distribution<std::size_t> generate_it(0, book.levels.size() - 1);
    std::size_t skip = generate_it(number_generator_);

    auto it = book.levels.begin();
    std::advance(it, skip);
    return it;
}
    
void ExchangeSimulator::ReleasePrice(BookState& book, Price price_to_release) {
    book.levels.erase(price_to_release);
    book.avail_prices.push_back(price_to_release);
}

void ExchangeSimulator::SerializeEvent(std::uint8_t* buf, const EventToSend& next) {
    const MarketEvent& event = next.event;
    Bytes offset = WriteMoldUDP64Header(buf, next.sequence_number);

    WriteBigEndian<InstrumentId>(buf, offset, event.instrument_id);
    offset += sizeof(InstrumentId);

    WriteBigEndian<uint8_t>(buf, offset, static_cast<uint8_t>(event.side));
    offset += sizeof(Side);

    WriteBigEndian<uint8_t>(buf, offset, static_cast<uint8_t>(event.event));
    offset += sizeof(event.event);

    WriteBigEndian<Price>(buf, offset, event.price);
    offset += sizeof(Price);

    WriteBigEndian<Quantity>(buf, offset, event.quantity);
    offset += sizeof(Quantity);

    WriteBigEndian<Timestamp>(buf, offset, event.exchange_ts);
    
}

Bytes ExchangeSimulator::WriteMoldUDP64Header(std::uint8_t* buf, SequenceNumber sequence_number) {
    Bytes offset = 0;

    std::memcpy(buf, session_, kSessionLength);
    offset += kSessionLength;

    WriteBigEndian<SequenceNumber>(buf, offset, sequence_number);
    offset += sizeof(SequenceNumber);

    WriteBigEndian<MessageCount>(buf, offset, kMessageCount);
    offset += sizeof(MessageCount);

    const Bytes remaining = kPacketSize - (offset + sizeof(MessageDataSize));
    WriteBigEndian<MessageDataSize>(buf, offset, static_cast<MessageDataSize>(remaining));

    offset += sizeof(MessageDataSize);

    return offset;
}

Timestamp ExchangeSimulator::CurrentTime() {
    return static_cast<Timestamp>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count()
    );
}

BookState& ExchangeSimulator::GetBook(InstrumentId id, Side side) {
    if (side == Side::kBid) return books_[id].bids;
    else return books_[id].asks;
}


int main() {
    ExchangeSimulator exchange;
    
    std::thread sender([&] {exchange.SendDatagrams(); } );
    std::thread generator([&] {exchange.GenerateMarketEvents(); } );
    std::thread retransmitter([&]{ exchange.Retransmitter(); });

    std::cout << "Exchange simulator has started.\n";

    generator.join();
}