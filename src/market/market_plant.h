#pragma once

#include <iostream>
#include <cstdint>
#include <unordered_map>
#include <cstddef>

#include <grpcpp/grpcpp.h>
#include <grpcpp/server_context.h>
#include <grpcpp/support/status.h>

#include "market_plant/market_plant.grpc.pb.h"
#include "market_plant/market_plant.pb.h"

namespace ms = market_plant::v1;

using grpc::Status;
using grpc::ServerContext;
using grpc::ServerWriter;

// Plant -> Subscriber API (TBD)
struct OrderBookDelta {
    
};

// handle all subscription and order
class MarketPlantServer final : public ms::MarketPlantService::Service {
public:
    MarketPlantServer();

    // Server-side streaming
    grpc::Status StreamUpdates(grpc::ServerContext* context, const ms::Subscription* request, ::grpc::ServerWriter< ms::StreamResponse>* writer);

    // Control-plane for modifying subscriptions
    grpc::Status UpdateSubscriptions(grpc::ServerContext* context, const ms::UpdateSubscriptionRequest* request, ::google::protobuf::Empty* response);
private:

};


// use std::move later
// class OrderBook {
// public:
//     OrderBook(const Depth depth_,  const InstrumentId instrument_id_);
    
// private:
//     InstrumentId instrument_id;
//     Depth depth;

//     Levels bids;
//     Levels asks;
// };
