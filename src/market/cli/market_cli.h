#pragma once

#include "market_plant.h"

#include <cstdint>
#include <string>
#include <vector>

struct Exchange {
    std::uint16_t port;
    std::string ip;
};

struct Instrument {
    InstrumentId id;
    Depth depth;
};

struct RuntimeConfig {
    uint16_t sub_ptool_size;
    uint16_t max_sub_limit;
};

using InstrumentConfig = std::vector<Instrument>;

struct Config {
    RuntimeConfig runtime;
    Exchange exchange;
    InstrumentConfig instruments;
};

void print_help();

// Returns false if user asked for help; otherwise parses config in `out` and returns true.
bool parse_args(int argc, char* argv[], Config& out);
