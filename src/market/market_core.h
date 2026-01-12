#pragma once

#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <unordered_map>

#include "event.h"
#include "market_cli.h"

class Subscriber;

namespace market_plant::v1 {
    class StreamResponse;
    class SnapshotUpdate;
}

namespace ms = market_plant::v1;
using StreamResponsePtr = std::shared_ptr<const ms::StreamResponse>;

// All orderbook updates happen from ExchangeFeed
// All subscription updates happen from the MarketPlantServer
class OrderBook {
public:
    OrderBook(InstrumentId id, Depth depth);
    
    void PushEventToSubscribers(const MarketEvent& data);

    void InitializeSubscription(std::shared_ptr<Subscriber> subscriber);
    
    void CancelSubscription(SubscriberId id);
private:
    void AddOrder(Side side, Price price, Quantity quantity);
        
    void RemoveOrder(Side side, Price price, Quantity quantity);

    void Snapshot(ms::SnapshotUpdate* snapshot);

    template <class Levels>
    static void UpdateLevel(Levels& levels, Price price, Quantity quantity) {
        auto [it, added] = levels.try_emplace(price, quantity);
        if (!added) it->second += quantity;
    }
    
    template <class Levels>
    static void ModifyLevel(Levels& levels, typename Levels::iterator it, Quantity quantity) {
        if (quantity >= it->second) {
            levels.erase(it);
        } else {
            it->second -= quantity;
        }
    }

    std::mutex mutex_;
    std::map<Price, Quantity, std::greater<Price>> bids_;
    std::map<Price, Quantity, std::less<Price>> asks_;

    std::unordered_map<SubscriberId, std::weak_ptr<Subscriber>> subscriptions_;
    InstrumentId id_;
    Depth depth_;
};


class BookManager {
public:
    explicit BookManager(const InstrumentConfig& instruments);

    OrderBook& Book(InstrumentId id);

    const OrderBook& Book(InstrumentId id) const;

private:
    std::unordered_map<InstrumentId, OrderBook> books_;
};