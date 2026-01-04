#pragma once

#include <grpcpp/grpcpp.h>
#include <map>
#include <memory>

#include "event.h"
#include "subscriber_config.h"
#include "market_plant/market_plant.grpc.pb.h"
#include "market_plant/market_plant.pb.h"

namespace ms = market_plant::v1;

struct LocalOrderBookCopy {
    std::map<Price, Quantity, std::greater<Price>> bids;
    std::map<Price, Quantity, std::less<Price>> asks;
};

class MarketDataSubscriber {
public:
    explicit MarketDataSubscriber(SubscriberConfig config);
    
    void Run();
    
private:
    void HandleEvent(const ms::OrderBookEventUpdate& event);
    void HandleSnapshot(const ms::SnapshotUpdate& snapshot);
    void PrintBookState();
    
    SubscriberConfig config_;
    LocalOrderBookCopy book_;
    std::unique_ptr<ms::MarketPlantService::Stub> stub_;
};