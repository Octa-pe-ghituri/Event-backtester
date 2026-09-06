// FORMAT FISIER:
//
// time SYMBOL TYPE order_id owner_id SIDE quantity price
//
// exemplu:
// 0 AAPL ADD 1001 2 BUY 10 10000
//

#pragma once

#include "backtester/types.hpp"

#include <string>
#include <vector>

OrderType ParseOrderType(const std::string &value);

Side ParseSide(const std::string &value);

std::vector<BackTestEvent> LoadEventsFromFile(const std::string &path);
