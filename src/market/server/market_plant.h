#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <memory>
#include <mutex>
#include <random>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include <grpcpp/grpcpp.h>
#include "market_plant/market_plant.grpc.pb.h"

#include "event.h"
#include "market_core.h"

namespace ms = market_plant::v1;

using grpc::Status;
using grpc::ServerContext;
using grpc::ServerWriter;
using StreamResponsePtr = std::shared_ptr<const ms::StreamResponse>;

class SessionGenerator {
public:
    static std::string Generate();
private:
    inline static std::mutex generator_mutex_;
    inline static std::mt19937_64 byte_generator_{std::random_device{}()};
};

struct Identifier {
    SubscriberId subscriber_id;
    std::string session_key;
};


// on construction, queue should be init to n snapshots of the n instruments subscribed to
class Subscriber {
public:
    Subscriber(const Identifier& subscriber, const ms::InstrumentIds& instruments);

    bool Subscribe(InstrumentId id);

    void Unsubscribe(InstrumentId id);

    void Enqueue(const StreamResponsePtr& next);

    StreamResponsePtr WaitDequeue(ServerContext* ctx);

    const Identifier& subscriber() const { return subscriber_; }

private:
    Identifier subscriber_;

    std::condition_variable cv_;
    std::mutex mutex_;
    
    // queue of Update(s) to send
    std::deque<StreamResponsePtr> updates_;

    // unordered_set of instruments subscribed to
    std::unordered_set<InstrumentId> subscribed_to_;
};

// handle all subscription and order
class MarketPlantServer final : public ms::MarketPlantService::Service {
public:
    explicit MarketPlantServer(BookManager& books);

    // Server-side streaming
    Status StreamUpdates(ServerContext* context, const ms::Subscription* request, ServerWriter< ms::StreamResponse>* writer) override;

    // Control-plane for modifying subscriptions
    Status UpdateSubscriptions(ServerContext* context, const ms::UpdateSubscriptionRequest* request, google::protobuf::Empty* response) override;

    std::shared_ptr<Subscriber> AddSubscriber(const ms::InstrumentIds& subscriptions);

    void RemoveSubscriber(const SubscriberId id);

    static StreamResponsePtr ConstructEventUpdate(const MarketEvent& e);

private:
    static Identifier InitSubscriber();

    inline static std::atomic<SubscriberId> next_subscriber_id_{1};
    BookManager& books_;

    inline static std::shared_mutex sub_lock_;
    std::unordered_map<SubscriberId, std::weak_ptr<Subscriber>> subscribers_;
};