#include <iostream>
#include <cstdint>

using Price = std::int32_t;
using Quantity = std::uint32_t;

enum class Side {
    BUY,
    SELL,
};

struct Level {
    Price level_price;
    Quantity level_quantity;
};

// use std::move later
class OrderBook {
public:
    OrderBook(const std::vector<Level>& bids_, const std::vector<Level>& asks_, const std::string& ticker_)
        : ticker(ticker_), bids(bids_), asks(asks_) {}
    
private:
    std::string ticker;
    std::vector<Level> bids;
    std::vector<Level> asks;
};

int main() {

}
