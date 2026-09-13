#include "backtester/config.hpp"

#include <cmath>
#include <fstream>
#include <stdexcept>
#include <string>
#include <unordered_set>

namespace {

std::string Trim(const std::string &value) {
    const std::size_t first = value.find_first_not_of(" \t\r\n");

    if (first == std::string::npos) {
        return "";
    }

    const std::size_t last = value.find_last_not_of(" \t\r\n");

    return value.substr(first, last - first + 1);
}

std::string ConfigLocation(const std::string &path, int lineNumber) {
    return path + ":" + std::to_string(lineNumber);
}

int ParseInt(const std::string &value, const std::string &key, const std::string &location) {
    std::size_t parsedCharacters = 0;

    try {
        const int result = std::stoi(value, &parsedCharacters);

        if (parsedCharacters != value.size()) {
            throw std::runtime_error("invalid integer");
        }

        return result;
    } catch (const std::exception &) {
        throw std::runtime_error(location + ": invalid integer for " + key + ": " + value);
    }
}

double ParseDouble(const std::string &value, const std::string &key, const std::string &location) {
    std::size_t parsedCharacters = 0;

    try {
        const double result = std::stod(value, &parsedCharacters);

        if (parsedCharacters != value.size() || !std::isfinite(result)) {
            throw std::runtime_error("invalid number");
        }

        return result;
    } catch (const std::exception &) {
        throw std::runtime_error(location + ": invalid number for " + key + ": " + value);
    }
}

} // namespace

// FILE FORMAT:
//
// key = value
//
// Empty lines and lines beginning with # are ignored.
BacktestConfig LoadBacktestConfigFromFile(const std::string &path) {
    std::ifstream input(path);

    if (!input) {
        throw std::runtime_error("could not open backtest config file: " + path);
    }

    BacktestConfig config;
    std::unordered_set<std::string> parsedKeys;

    std::string line;
    int lineNumber = 0;

    while (std::getline(input, line)) {
        lineNumber++;

        const std::string trimmedLine = Trim(line);

        if (trimmedLine.empty() || trimmedLine.front() == '#') {
            continue;
        }

        const std::size_t separator = trimmedLine.find('=');
        const std::string location = ConfigLocation(path, lineNumber);

        if (separator == std::string::npos || trimmedLine.find('=', separator + 1) != std::string::npos) {
            throw std::runtime_error(location + ": expected key = value");
        }

        const std::string key = Trim(trimmedLine.substr(0, separator));
        const std::string value = Trim(trimmedLine.substr(separator + 1));

        if (key.empty() || value.empty()) {
            throw std::runtime_error(location + ": expected non-empty key and value");
        }

        if (!parsedKeys.insert(key).second) {
            throw std::runtime_error(location + ": duplicate config key: " + key);
        }

        if (key == "max_time") {
            config.maxTime = ParseInt(value, key, location);
        } else if (key == "strategy_owner_id") {
            config.strategyOwnerId = ParseInt(value, key, location);
        } else if (key == "fee_ticks") {
            config.feeTicks = ParseDouble(value, key, location);
        } else if (key == "market_communication_latency_ticks") {
            config.latency.marketCommunicationLatencyTicks = ParseInt(value, key, location);
        } else if (key == "prediction_generation_latency_ticks") {
            config.latency.predictionGenerationLatencyTicks = ParseInt(value, key, location);
        } else if (key == "prediction_transfer_latency_ticks") {
            config.latency.predictionTransferLatencyTicks = ParseInt(value, key, location);
        } else {
            throw std::runtime_error(location + ": unknown config key: " + key);
        }
    }

    if (!input.eof()) {
        throw std::runtime_error("could not read backtest config file: " + path);
    }

    return config;
}
