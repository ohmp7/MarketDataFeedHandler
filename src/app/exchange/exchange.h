#pragma once

#include "event.h"
#include "exchange_config.h"

#include <condition_variable>
#include <cstdint>
#include <deque>
#include <mutex>
#include <random>
#include <unordered_map>
#include <vector>

#include <netinet/in.h>
#include <sys/socket.h>

inline constexpr int kMaxPrice = 100;

struct BookState {
    std::unordered_map<Price, Quantity> levels;
    std::vector<Price> avail_prices;

    BookState();
};

struct InstrumentState {
    BookState bids;
    BookState asks;
};

struct EventToSend {
    MarketEvent event;
    SequenceNumber sequence_number;
};

class ExchangeSimulator {
public:

    ExchangeSimulator();

    void SendDatagrams();

    void GenerateMarketEvents();

    void GenerateHeartbeats();

    void Retransmitter();

private:

    void EnqueueEvent(const MarketEvent& e, SequenceNumber sequence_number);

    Price PickNewPrice(std::vector<Price>& avail_prices);

    std::unordered_map<Price, Quantity>::iterator PickExistingPrice(BookState& book);
    
    void ReleasePrice(BookState& book, Price price_to_release);

    void SerializeEvent(std::uint8_t* buf, const EventToSend& next);

    Bytes WriteMoldUDP64Header(std::uint8_t* buf, SequenceNumber sequence_number);

    static Timestamp CurrentTime();

    BookState& GetBook(InstrumentId id, Side side);

    // network
    int sockfd_{-1};
    sockaddr_in plantaddr_{};

    ExchangeConfig config_;
    inline static constexpr char session_[kSessionLength] = {'E','X','C','H','A','N','G','E','I','D'};

    // Live Exchange State
    std::unordered_map<InstrumentId, InstrumentState> books_;
    std::deque<EventToSend> events_queue_;
    std::vector<MarketEvent> events_history_;  // events[sequence_number]
    std::uint64_t sequence_number_{0};
    std::mutex queue_mutex_;
    std::mutex history_mutex_;
    std::condition_variable cv_;

    // Generators
    std::mt19937_64 number_generator_{std::random_device{}()};
    std::uniform_int_distribution<int> generate_id_;
    std::uniform_int_distribution<int> generate_side_;
    std::uniform_int_distribution<int> generate_event_;
    std::uniform_int_distribution<Price> generate_price_;
    std::uniform_int_distribution<Quantity> generate_quantity_;
    std::uniform_int_distribution<int> generate_interval_;
};