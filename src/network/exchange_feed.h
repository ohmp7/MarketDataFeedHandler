#pragma once

#include <cstdint>
#include <string>

#include <netinet/in.h>

#include "event.h"
#include "market_plant_config.h"
#include "moldudp64.h"

class BookManager;
class OrderBook;

class ExchangeFeed {
public:
    ExchangeFeed(BookManager& books, const MarketPlantConfig& mp_config, int cpu_core = -1);
    
    ~ExchangeFeed();

    void ConnectToExchange();
    
    const OrderBook& GetOrderBook(InstrumentId id) const;

private:
    void HandleEvent(const MessageView& message);

    MarketEvent ParseEvent(const MessageView& message);

    sockaddr_in ConstructIpv4(const std::string& ip, std::uint16_t port);

    int sockfd_{-1};
    MoldUDP64 protocol_;
    BookManager& books_;
    int cpu_core_;
};