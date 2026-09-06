#pragma once

#include "backtester/types.hpp"

#include <string>
#include <vector>

OrderType ParseOrderType(const std::string &value);

Side ParseSide(const std::string &value);

std::vector<BackTestEvent> LoadEventsFromFile(const std::string &path);
