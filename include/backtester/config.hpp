#pragma once

#include <string>

struct LatencyConfig {

        int marketCommunicationLatencyTicks = 0;

        int predictionGenerationLatencyTicks = 0;

        int predictionTransferLatencyTicks = 0;
};

struct BacktestConfig {

        int maxTime = 0;

        int strategyOwnerId = 1;

        double feeTicks = 0.0;

        LatencyConfig latency;
};

BacktestConfig LoadBacktestConfigFromFile(const std::string &path);
