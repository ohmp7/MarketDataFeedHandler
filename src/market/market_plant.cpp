#include "market_plant.h"
#include "moldudp64_client.h"
#include "market_cli.h"

#include <fstream>
#include <iostream>
#include <thread>
#include <stdexcept>
#include <string>

OrderBook::OrderBook(const Depth depth_,  const InstrumentId instrument_id_)
    : instrument_id(instrument_id_), depth(depth_) {}


// handle all subscription and order
class MarketPlantServer {
public:
    // Takes in config class, which sets up all classes below

private:
    // OrderBookManager (manages all instances of OrderBook class)
    
};

class ExchangeFeed {
public:
    ExchangeFeed() {}

private:
    MoldUDP64 protocol();
};


// Exchange Side:
// IP, Port
// Establish connection with exchange
// Ingesting new packets of data recvfrom()
// Parsing MoldUDP64::handle_packet()
// updating the state of the orderbook

// Subscriber Side
// Manage subscribers


int main(int argc, char* argv[]) {
    Config conf{};

    try {
        if (!parse_args(argc, argv, conf)) return 0;
    } catch (const std::runtime_error& e) {
        std::cerr << "Error: " << e.what() << "\n\n";
        print_help();
        return 1;
    }

    std::cout << "max threads: " << conf.runtime.sub_ptool_size << "\n";

    return 0;
}
