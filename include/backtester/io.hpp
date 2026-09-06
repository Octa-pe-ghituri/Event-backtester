// FORMAT FISIER:
//
// time SYMBOL TYPE order_id owner_id SIDE quantity price
//
// exemplu:
// 0 AAPL ADD 1001 2 BUY 10 10000
//

// review: each column should have a small description of what it is, how time should be formatted for example
// p.s: maybe try to use csv instead of txt, easier to read and parse

#pragma once

#include "backtester/types.hpp"

#include <string>
#include <vector>

OrderType ParseOrderType(const std::string &value);

Side ParseSide(const std::string &value);

std::vector<BackTestEvent> LoadEventsFromFile(const std::string &path);
