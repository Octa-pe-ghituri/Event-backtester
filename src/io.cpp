#include "backtester/io.hpp"

#include <fstream>
#include <stdexcept>

// Review: Oare comentariul asta este ok sa ramana in prod?

// luat de la Vlad - echivalentul parseEventType.
// Adaptat la OrderType-ul meu.
OrderType ParseOrderType(const std::string &value) {

    if (value == "ADD")
        return OrderType::Add;

    if (value == "CANCEL")
        return OrderType::Cancel;

    if (value == "MODIFY" || value == "MOD")
        return OrderType::Mod;

    if (value == "IOC")
        return OrderType::IOC;

    if (value == "MARKET")
        return OrderType::Market;

    throw std::invalid_argument("unknown order type: " + value);
}

// Review: Ce parere ai de asta?
Side ParseSide(const std::string &value) {

    if (value == "BUY")
        return Side::Buy;

    if (value == "SELL")
        return Side::Sell;

    throw std::invalid_argument("unknown side: " + value);
}

// luat de la Vlad - loadEventsFromFile.
// Adaptat pentru multiple simboluri.
//
// FORMAT FISIER:
//
// time SYMBOL TYPE order_id owner_id SIDE quantity price
//
// exemplu:
// 0 AAPL ADD 1001 2 BUY 10 10000
//
std::vector<BackTestEvent> LoadEventsFromFile(const std::string &path) {

    std::ifstream input(path);

    if (!input) {

        throw std::runtime_error("could not open events file: " + path);
    }

    std::vector<BackTestEvent> events;

    int time;
    Symbol symbol;
    std::string typeText;

    int orderId;
    int ownerId;

    std::string sideText;

    int quantity;
    int price;

    while (input >> time >> symbol >> typeText >> orderId >> ownerId >> sideText >> quantity >> price) {

        events.push_back(
            {symbol, ParseSide(sideText), ParseOrderType(typeText), orderId, ownerId, quantity, price, time});
    }

    if (!input.eof()) {

        throw std::runtime_error("malformed events file: " + path);
    }

    return events;
}
