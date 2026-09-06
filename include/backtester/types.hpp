#pragma once

#include <string>

using Symbol = std::string;

enum class Side { Buy, Sell };

enum class OrderType { Add, Cancel, Mod, Market, IOC };

enum class ResponseEvents {
  OrderAccepted,
  OrderFilled,
  OrderPartialFilled,
  OrderCancelled,
  OrderCancelFailed,
  OrderModified,
  OrderModifyFailed,
  OrderMarketFilled,
  OrderMarketPartialFill,
  OrderMarketFailed
};

struct Order { /// used by book
  const int order_id = 0;
  const int owner_id = 0;
  const Side side = Side::Buy;

  int quantity = 0;
  int price = 0;

  const int time = 0;
};

struct BackTestEvent { /// used by the back tester
  Symbol symbol;
  Side side;
  OrderType order_type;

  int order_id;
  int owner_id;
  int quantity;
  int price;
  int time;
};

struct OrderEvent { /// used to return FROM THE BOOK the events that took place
                    /// while processing a order
  Side side;
  ResponseEvents type;

  const int order_id;
  const int owner_id;
  int quantity;
  int price;
  const int time;
};

struct Fill {
  Symbol symbol;
  Side side;

  const int id;
  const int quantity;
  const int price;
  const int time;
};
