#pragma once

#include "backtester/types.hpp"

#include <map>
#include <optional>
#include <unordered_map>
#include <vector>

class Book {

public:
  std::vector<OrderEvent> ProcessOrder(const BackTestEvent &Event);

private:
  struct LevelNode {

    LevelNode *nxt;
    LevelNode *prev;

    const int order_id;
    const int owner_id;

    int quantity;
  };

  struct Location_ID {

    LevelNode *location;

    Side side;

    const int owner_id;

    int price;
  };

  struct Location_Rest {

    LevelNode *first;
    LevelNode *last;
  };

  std::map<int, Location_Rest> bids;
  std::map<int, Location_Rest> asks;

  std::unordered_map<int, Location_ID> location;

  std::vector<OrderEvent> ProcessAdd(const BackTestEvent &Event);

  std::vector<OrderEvent> ProcessCancel(const BackTestEvent &Event);

  std::vector<OrderEvent> ProcessMod(const BackTestEvent &Event);

  std::vector<OrderEvent> ProcessIOC(const BackTestEvent &Event);

  std::vector<OrderEvent> ProcessMarket(const BackTestEvent &Event);

  void marketBuy(Order &order, std::vector<OrderEvent> &responses,
                 const int &time);

  void marketSell(Order &order, std::vector<OrderEvent> &responses,
                  const int &time);

  std::optional<Order> try_remove(const int &ID_ToBeRemoved,
                                  const int &owner_id);

  std::optional<Order> removeFromSide(std::map<int, Location_Rest> &Level,
                                      const int &price, const int &owner_id,
                                      const int &order_id, const Side &side);

  void rest_order(const Order &order);

  void init_level(const Order &order, std::map<int, Location_Rest> &Levels);

  void add_order(const Order &order, std::map<int, Location_Rest> &Levels);

  void matchBuy(Order &order, std::vector<OrderEvent> &responses,
                const int &time);

  void matchSell(Order &order, std::vector<OrderEvent> &responses,
                 const int &time);

  OrderEvent Fill(const int &order_id, const int &owner_id, const Side &side,
                  const int &remaining_quantity, const int &traded_quantity,
                  const int &price, const int &time);

  OrderEvent FillMarket(const int &order_id, const int &owner_id,
                        const Side &side, const int &remaining_quantity,
                        const int &traded_quantity, const int &price,
                        const int &time);
};
