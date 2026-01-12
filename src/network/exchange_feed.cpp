#include "exchange_feed.h"

#include "cpu_affinity.h"
#include "endian.h"
#include "market_core.h"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstdint>
#include <iostream>
#include <stdexcept>

ExchangeFeed::ExchangeFeed(BookManager& books, const MarketPlantConfig& mp_config, int cpu_core)
    : sockfd_(socket(AF_INET, SOCK_DGRAM, 0)),
        protocol_(0, sockfd_, mp_config.exchange_ip, mp_config.exchange_port),
        books_(books),
        cpu_core_(cpu_core) {
    
    if (sockfd_ < 0) throw std::runtime_error("Error: socket creation to exchange failed.");
    
    // MARKET
    sockaddr_in plantaddr = ConstructIpv4(mp_config.market_ip, mp_config.market_port);
    if (bind(sockfd_, reinterpret_cast<sockaddr*>(&plantaddr), sizeof(plantaddr)) < 0) {
        throw std::runtime_error("Error: bind failed.");
    }

    // EXCHANGE
    sockaddr_in exaddr = ConstructIpv4(mp_config.exchange_ip, mp_config.exchange_port);
    if (connect(sockfd_, reinterpret_cast<sockaddr*>(&exaddr), sizeof(exaddr)) < 0) {
        throw std::runtime_error("udp connect failed");
    }
}

ExchangeFeed::~ExchangeFeed() {
        if (sockfd_ >= 0) close(sockfd_);
}

void ExchangeFeed::ConnectToExchange() {
    if (cpu_core_ >= 0) {
        if (CPUAffinity::PinToCore(cpu_core_)) [[likely]] 
            std::cout << "Successfully pinned Exchange Feed to core " << cpu_core_ << ".\n";
        else {
            std::cout << "Failed to pin Exchange Feed to core " << cpu_core_ << ".\n";
        }
    }

    std::uint8_t buf[512];

    while (true) {
        ssize_t n = recvfrom(sockfd_, buf, sizeof(buf), 0, nullptr, nullptr);
        if (n <= 0) [[unlikely]] continue;
        
        try {
            if (protocol_.HandlePacket(buf, static_cast<Bytes>(n))) {
                auto message = protocol_.message_view();
                HandleEvent(message);
            }
        } catch (const PacketTruncatedError& e) {
            std::cerr << e.what() << "\n";
        }
    }
}

const OrderBook& ExchangeFeed::GetOrderBook(InstrumentId id) const {
    return books_.Book(id);
}   

void ExchangeFeed::HandleEvent(const MessageView& message) {
    MarketEvent e = ParseEvent(message);
    books_.Book(e.instrument_id).PushEventToSubscribers(e);
}

MarketEvent ExchangeFeed::ParseEvent(const MessageView& message) {
    const std::uint8_t* p = message.data;
    Bytes off = 0;

    MarketEvent m_event{};

    m_event.instrument_id = ReadBigEndian<InstrumentId>(p, off); off += sizeof(InstrumentId);
    m_event.side = static_cast<Side>(p[off++]);
    m_event.event = static_cast<LevelEvent>(p[off++]);
    m_event.price = ReadBigEndian<Price>(p, off); off += sizeof(Price);
    m_event.quantity = ReadBigEndian<Quantity>(p, off); off += sizeof(Quantity);
    m_event.exchange_ts = ReadBigEndian<Timestamp>(p, off); off += sizeof(Timestamp);

    return m_event;
}

sockaddr_in ExchangeFeed::ConstructIpv4(const std::string& ip, std::uint16_t port) {
    sockaddr_in res{};
    res.sin_family = AF_INET;
    res.sin_port = htons(port);
    if (inet_pton(AF_INET, ip.c_str(), &res.sin_addr) != 1) {
        close(sockfd_);
        sockfd_ = -1;
        throw std::runtime_error("Error: failed to convert IPv4 address from text to binary form.");
    }
    return res;
}
