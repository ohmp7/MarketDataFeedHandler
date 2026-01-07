// market_cli.h
#pragma once

#include "event.h"

#include <string>
#include <vector>

struct Instrument {
    InstrumentId id;
    Depth depth;
};

using InstrumentConfig = std::vector<Instrument>;

struct MarketPlantCliConfig {
    InstrumentConfig instruments;
    int cpu_core = -1;
};

void PrintHelp();

// Returns false if user asked for help; otherwise parses config in `out` and returns true.
bool ParseArgs(int argc, char* argv[], MarketPlantCliConfig& out);
