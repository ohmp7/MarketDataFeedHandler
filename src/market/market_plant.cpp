#include "endian.h"
#include "event.h"
#include "market_plant.h"
#include "moldudp64.h"
#include "market_cli.h"
#include <iomanip>

#include <arpa/inet.h>
#include <stdexcept>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <thread>
#include <unordered_map>
#include <stdexcept>
#include <string>

constexpr std::uint16_t market_port = 9001;
std::string market_ip = "127.0.0.1";

class OrderBook {
public:
    OrderBook(const Depth depth) : depth_(depth) {}
    
    void AddOrder(Side side, Price price, Quantity quantity) {
        if (side == Side::BID) {
            UpdateLevel(bids_, price, quantity);
        } else {
            UpdateLevel(asks_, price, quantity);
        }
    }
        
    void RemoveOrder(Side side, Price price, Quantity quantity) {
        if (side == Side::BID) {
            auto it = bids_.find(price);
            if (it == bids_.end()) return;
            ModifyLevel(bids_, it, quantity);
        } else {
            auto it = asks_.find(price);
            if (it == asks_.end()) return;
            ModifyLevel(asks_, it, quantity);
        }
    }

    void Snapshot() {
        std::cout << "\033[2J\033[H";

        std::cout << "-----------------------------+-----------------------------\n";
        std::cout << "   BIDS (Price | Qty)        |   ASKS (Price | Qty)\n";
        std::cout << "-----------------------------+-----------------------------\n";

        auto bid_it = bids_.begin();
        auto ask_it = asks_.begin();

        for (Depth i = 0; i < depth_; ++i) {
            // left side: bids
            if (bid_it != bids_.end()) {
                std::cout << std::setw(8) << bid_it->first << " | "
                        << std::setw(8) << bid_it->second;
                ++bid_it;
            } else {
                std::cout << std::setw(8) << "-" << " | " << std::setw(8) << "-";
            }

            std::cout << "        |   ";

            // right side: asks
            if (ask_it != asks_.end()) {
                std::cout << std::setw(8) << ask_it->first << " | "
                        << std::setw(8) << ask_it->second;
                ++ask_it;
            } else {
                std::cout << std::setw(8) << "-" << " | " << std::setw(8) << "-";
            }

            std::cout << "\n";
        }

        std::cout << "-----------------------------+-----------------------------\n";
        std::cout.flush();
    }
    

private:
    template <class Levels>
    static void UpdateLevel(Levels &levels, Price price, Quantity quantity) {
        auto [it, added] = levels.try_emplace(price, quantity);
        if (!added) it->second += quantity;
    }
    
    template <class Levels>
    static void ModifyLevel(Levels &levels, typename Levels::iterator it, Quantity quantity) {
        if (quantity >= it->second) {
            levels.erase(it);
        } else {
            it->second -= quantity;
        }
    }

    std::map<Price, Quantity, std::greater<Price>> bids_;
    std::map<Price, Quantity, std::less<Price>> asks_;

    Depth depth_;
};


class BookManager {
public:
    explicit BookManager(const InstrumentConfig& instruments) {
        books_.reserve(instruments.size());
        
        for (const auto& instrument : instruments) {
            books_.emplace(instrument.id, instrument.depth);
        }
    }

    OrderBook& book(InstrumentId id) { return books_.at(id); }

    const OrderBook& book(InstrumentId id) const { return books_.at(id); }

private:
    std::unordered_map<InstrumentId, OrderBook> books_;
};


// handle all subscription and order
class MarketPlantServer {
public:

private:
    // store subscribers via unique_ptr

};

class ExchangeFeed {
public:
    ExchangeFeed(const Exchange& exchange, const InstrumentConfig& instruments)
        : sockfd_(socket(AF_INET, SOCK_DGRAM, 0)),
          protocol_(0, sockfd_, exchange.ip, exchange.port), 
          orderbooks_(instruments) {
        
        if (sockfd_ < 0) throw std::runtime_error("Error: socket creation to exchange failed.");
        
        // MARKET
        sockaddr_in plantaddr = construct_ipv4(market_ip, market_port);
        if (bind(sockfd_, reinterpret_cast<sockaddr*>(&plantaddr), sizeof(plantaddr)) < 0) {
            throw std::runtime_error("Error: bind failed.");
        }

        // EXCHANGE
        sockaddr_in exaddr = construct_ipv4(exchange.ip, exchange.port);
        if (connect(sockfd_, reinterpret_cast<sockaddr*>(&exaddr), sizeof(exaddr)) < 0) {
            throw std::runtime_error("udp connect failed");
        }
    }
    
    ~ExchangeFeed() {
        if (sockfd_ >= 0) close(sockfd_);
    }

    void ConnectToExchange() {
        std::uint8_t buf[512];

        while (true) {
            ssize_t n = recvfrom(sockfd_, buf, sizeof(buf), 0, nullptr, nullptr);
            if (n <= 0) continue;
            
            try {
                if (protocol_.handle_packet(buf, static_cast<Bytes>(n))) {
                    auto message = protocol_.message_view();
                    handle_event(message);
                }
            } catch (const PacketTruncatedError& e) {
                std::cerr << e.what() << "\n";
            }
        }
    }

private:
    void handle_event(const MessageView &message) {
        MarketEvent e = parse_event(message);

        if (e.event == LevelEvent::AddLevel) {
            orderbooks_.book(e.instrument_id).AddOrder(e.side, e.price, e.quantity);
        } else {
            orderbooks_.book(e.instrument_id).RemoveOrder(e.side, e.price, e.quantity);
        }

        orderbooks_.book(e.instrument_id).Snapshot();
    }

    MarketEvent parse_event(const MessageView &message) {
        const std::uint8_t* p = message.data;
        Bytes off = 0;

        MarketEvent m_event{};

        m_event.instrument_id = read_big_endian<InstrumentId>(p, off); off += sizeof(InstrumentId);
        m_event.side = static_cast<Side>(p[off++]);
        m_event.event = static_cast<LevelEvent>(p[off++]);
        m_event.price = read_big_endian<Price>(p, off); off += sizeof(Price);
        m_event.quantity = read_big_endian<Quantity>(p, off); off += sizeof(Quantity);
        m_event.exchange_ts = read_big_endian<Timestamp>(p, off); off += sizeof(Timestamp);

        return m_event;
    }

    sockaddr_in construct_ipv4(const std::string& ip, std::uint16_t port) {
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

    int sockfd_{-1};

    MoldUDP64 protocol_;
    BookManager orderbooks_;
};

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
    ExchangeFeed feed(conf.exchange, conf.instruments);
    std::thread exchange_feed([&]{ feed.ConnectToExchange(); });
    exchange_feed.detach();

    for (;;) {}  // placeholder
    return 0;
}