#pragma once

#include <string>

using Symbol = std::string;

enum class Side { Buy, Sell };

enum class OrderType { Add, Cancel, Mod, Market, IOC };

enum class ResponseEvents {
    OrderAccepted,
    OrderRejected,

    OrderFilled,
    OrderPartialFilled,

    OrderCancelled,
    OrderCancelFailed,

    OrderModified,
    OrderModifyFailed,

    OrderExpired
};

struct Order {
        int order_id = 0;
        int owner_id = 0;

        Side side = Side::Buy;

        int quantity = 0;
        int price = 0;

        int time = 0;
};

struct BackTestEvent {
        Symbol symbol;

        Side side = Side::Buy;
        OrderType order_type = OrderType::Add;

        int order_id = 0;
        int owner_id = 0;

        int quantity = 0;
        int price = 0;

        int time = 0;
};

struct OrderEvent {
        Side side = Side::Buy;
        ResponseEvents type = ResponseEvents::OrderAccepted;

        int order_id = 0;
        int owner_id = 0;

        int quantity = 0;
        int price = 0;

        int time = 0;
};

struct StrategyEvent {
        Symbol symbol;
        OrderEvent event;
};
