#include "market_plant.h"
#include "moldudp64_client.h"
#include "market_cli.h"

#include <arpa/inet.h>
#include <stdexcept>
#include <sys/socket.h>
#include <unistd.h>

#include <fstream>
#include <iostream>
#include <thread>
#include <stdexcept>
#include <string>

constexpr std::uint16_t listening_port = 9001;

class OrderBook {
public:
    OrderBook(const Depth depth_,  const InstrumentId instrument_id_)
    : instrument_id(instrument_id_), depth(depth_) {}
    
private:
    InstrumentId instrument_id;
    Depth depth;

    Levels bids;
    Levels asks;
};

// handle all subscription and order
class MarketPlantServer {
public:
    // Takes in config class, which sets up all classes below

private:
    // OrderBookManager (manages all instances of OrderBook class)
};

class ExchangeFeed {
public:
    ExchangeFeed(const Exchange& exchange)
        : sockfd_(socket(AF_INET, SOCK_DGRAM, 0)), protocol_(0, sockfd_, exchange.ip, exchange.port) {

        if (sockfd_ < 0) throw std::runtime_error("Error: socket creation to exchange failed.");
        
        memset(&exaddr, 0, sizeof(exaddr));
        exaddr.sin_family = AF_INET;
        exaddr.sin_port = htons(listening_port);
        exaddr.sin_addr.s_addr = htonl(INADDR_ANY);

        if (::bind(sockfd_, reinterpret_cast<sockaddr*>(&exaddr), sizeof(exaddr)) < 0) {
            close(sockfd_);
            throw std::runtime_error("Error: bind failed.");
        }
    }
    
    ~ExchangeFeed() {
        if (sockfd_ >= 0) close(sockfd_);
    }

    void connect() {
        std::uint8_t buf[2048];

        while (true) {
            ssize_t n = recvfrom(sockfd_, buf, sizeof(buf), 0, nullptr, nullptr);
            if (n <= 0) continue;
            
            try {
                if (protocol_.handle_packet(buf, static_cast<Bytes>(n))) {
                    auto msg = protocol_.message_view();

                    std::cout << "length: " << msg.len << "\n";
                    std::cout << "message: " << std::string(reinterpret_cast<const char*>(msg.data), msg.len) << "\n";
                }
            } catch (const PacketTruncatedError& e) {
                std::cerr << e.what() << "\n";
            }
        }
    }

private:
    int sockfd_{-1};
    sockaddr_in exaddr{};
    MoldUDP64 protocol_;
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

    // connect to exchange
    ExchangeFeed feed(conf.exchange);
    std::thread exchange_feed([&]{ feed.connect(); });
    exchange_feed.detach();

    for (;;) {}  // placeholder
    return 0;
}
