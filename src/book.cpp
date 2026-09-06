#include "backtester/book.hpp"

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <stdexcept>

Book::~Book() {

  clearLevels(bids);
  clearLevels(asks);

  location.clear();
}

void Book::clearLevels(std::map<int, Location_Rest> &levels) {

  for (auto &entry : levels) {

    LevelNode *node = entry.second.first;

    while (node != nullptr) {

      LevelNode *next = node->nxt;

      delete node;

      node = next;
    }
  }

  levels.clear();
}

std::vector<OrderEvent> Book::ProcessOrder(const BackTestEvent &Event) {

  if (Event.order_type == OrderType::Add)
    return ProcessAdd(Event);

  if (Event.order_type == OrderType::Cancel)
    return ProcessCancel(Event);

  if (Event.order_type == OrderType::Mod)
    return ProcessMod(Event);

  if (Event.order_type == OrderType::IOC)
    return ProcessIOC(Event);

  if (Event.order_type == OrderType::Market)
    return ProcessMarket(Event);

  return {};
}

std::vector<OrderEvent> Book::ProcessAdd(const BackTestEvent &Event) {

  if (location.find(Event.order_id) != location.end()) {

    return {{Event.side, ResponseEvents::OrderRejected, Event.order_id,
             Event.owner_id, Event.quantity, Event.price, Event.time}};
  }

  std::vector<OrderEvent> responses = {
      {Event.side, ResponseEvents::OrderAccepted, Event.order_id,
       Event.owner_id, Event.quantity, Event.price, Event.time}};

  Order order_data = {Event.order_id, Event.owner_id, Event.side,
                      Event.quantity, Event.price,    Event.time};

  if (order_data.side == Side::Buy) {

    matchBuy(order_data, responses, Event.time);

  } else {

    matchSell(order_data, responses, Event.time);
  }

  if (order_data.quantity > 0)
    rest_order(order_data);

  return responses;
}

std::vector<OrderEvent> Book::ProcessCancel(const BackTestEvent &Event) {

  std::optional<Order> removed = try_remove(Event.order_id, Event.owner_id);

  if (!removed.has_value()) {

    return {{Event.side, ResponseEvents::OrderCancelFailed, Event.order_id,
             Event.owner_id, Event.quantity, Event.price, Event.time}};
  }

  return {{removed->side, ResponseEvents::OrderCancelled, removed->order_id,
           removed->owner_id, removed->quantity, removed->price, Event.time}};
}

std::vector<OrderEvent> Book::ProcessMod(const BackTestEvent &Event) {

  if (Event.quantity <= 0 || Event.price <= 0) {

    return {{Event.side, ResponseEvents::OrderModifyFailed, Event.order_id,
             Event.owner_id, Event.quantity, Event.price, Event.time}};
  }

  auto locationIt = location.find(Event.order_id);

  if (locationIt == location.end() ||
      locationIt->second.owner_id != Event.owner_id ||
      locationIt->second.side != Event.side) {

    return {{Event.side, ResponseEvents::OrderModifyFailed, Event.order_id,
             Event.owner_id, Event.quantity, Event.price, Event.time}};
  }

  std::optional<Order> removed = try_remove(Event.order_id, Event.owner_id);

  if (!removed.has_value()) {

    return {{Event.side, ResponseEvents::OrderModifyFailed, Event.order_id,
             Event.owner_id, Event.quantity, Event.price, Event.time}};
  }

  std::vector<OrderEvent> responses = {
      {removed->side, ResponseEvents::OrderModified, removed->order_id,
       removed->owner_id, Event.quantity, Event.price, Event.time}};

  Order order_data = {removed->order_id, removed->owner_id, removed->side,
                      Event.quantity,    Event.price,       Event.time};

  if (order_data.side == Side::Buy) {

    matchBuy(order_data, responses, Event.time);

  } else {

    matchSell(order_data, responses, Event.time);
  }

  if (order_data.quantity > 0)
    rest_order(order_data);

  return responses;
}

std::vector<OrderEvent> Book::ProcessIOC(const BackTestEvent &Event) {

  if (Event.quantity <= 0 || Event.price <= 0 ||
      location.find(Event.order_id) != location.end()) {

    return {{Event.side, ResponseEvents::OrderRejected, Event.order_id,
             Event.owner_id, Event.quantity, Event.price, Event.time}};
  }

  std::vector<OrderEvent> responses = {
      {Event.side, ResponseEvents::OrderAccepted, Event.order_id,
       Event.owner_id, Event.quantity, Event.price, Event.time}};

  Order order_data = {Event.order_id, Event.owner_id, Event.side,
                      Event.quantity, Event.price,    Event.time};

  if (order_data.side == Side::Buy) {

    matchBuy(order_data, responses, Event.time);

  } else {

    matchSell(order_data, responses, Event.time);
  }

  if (order_data.quantity > 0) {

    responses.push_back({Event.side, ResponseEvents::OrderExpired,
                         Event.order_id, Event.owner_id, order_data.quantity,
                         Event.price, Event.time});
  }

  return responses;
}

std::vector<OrderEvent> Book::ProcessMarket(const BackTestEvent &Event) {

  if (Event.quantity <= 0 || location.find(Event.order_id) != location.end()) {

    return {{Event.side, ResponseEvents::OrderRejected, Event.order_id,
             Event.owner_id, Event.quantity, Event.price, Event.time}};
  }

  std::vector<OrderEvent> responses = {
      {Event.side, ResponseEvents::OrderAccepted, Event.order_id,
       Event.owner_id, Event.quantity, Event.price, Event.time}};

  Order order_data = {Event.order_id, Event.owner_id, Event.side,
                      Event.quantity, Event.price,    Event.time};

  if (Event.side == Side::Buy) {

    marketBuy(order_data, responses, Event.time);

  } else {

    marketSell(order_data, responses, Event.time);
  }

  if (order_data.quantity > 0) {

    responses.push_back({Event.side, ResponseEvents::OrderExpired,
                         Event.order_id, Event.owner_id, order_data.quantity,
                         Event.price, Event.time});
  }

  return responses;
}

void Book::marketBuy(Order &order, std::vector<OrderEvent> &responses,
                     const int &time) {

  while (!asks.empty() && order.quantity > 0) {

    auto it = asks.begin()->second.first;

    int price = asks.begin()->first; /// we use the price of the SELL order

    while (order.quantity > 0 && it != NULL) {

      int delta = std::min(order.quantity, it->quantity);

      order.quantity -= delta;
      it->quantity -= delta;

      responses.push_back(Fill(order.order_id, order.owner_id, order.side,
                               order.quantity, delta, price, time));

      responses.push_back(Fill(it->order_id, it->owner_id, Side::Sell,
                               it->quantity, delta, price, time));

      if (it->quantity == 0) {

        location.erase(it->order_id);

        auto aux = it->nxt;

        delete it;

        it = aux;
      }
    }

    /// change the first element
    if (it != NULL) {

      it->prev = NULL;

      asks.begin()->second.first = it;

    } else {

      /// we ran out of orders then we delete the level

      asks.erase(asks.begin());
    }
  }
}

void Book::marketSell(Order &order, std::vector<OrderEvent> &responses,
                      const int &time) {

  while (!bids.empty() && order.quantity > 0) {

    auto it = bids.rbegin()->second.first;

    int price = bids.rbegin()->first; /// we use the price of the SELL order

    while (order.quantity > 0 && it != NULL) {

      int delta = std::min(order.quantity, it->quantity);

      order.quantity -= delta;
      it->quantity -= delta;

      responses.push_back(Fill(order.order_id, order.owner_id, order.side,
                               order.quantity, delta, price, time));

      responses.push_back(Fill(it->order_id, it->owner_id, Side::Buy,
                               it->quantity, delta, price, time));

      if (it->quantity == 0) {

        location.erase(it->order_id);

        auto aux = it->nxt;

        delete it;

        it = aux;
      }
    }

    /// change the first element
    if (it != NULL) {

      it->prev = NULL;

      bids.rbegin()->second.first = it;

    } else {

      /// we ran out of orders : we delete the level
      /// bids is 100% not empty, otherwise we would not have entered
      /// the while

      auto lastLevelIt = std::prev(bids.end());

      bids.erase(lastLevelIt);
    }
  }
}

std::optional<Order> Book::try_remove(const int &ID_ToBeRemoved,
                                      const int &owner_id) {

  auto locationID = location.find(ID_ToBeRemoved);

  if (locationID == location.end())
    return std::nullopt;

  Side side = locationID->second.side;

  int price = locationID->second.price;

  if (side == Side::Buy) {

    return removeFromSide(bids, price, owner_id, ID_ToBeRemoved, side);
  }

  return removeFromSide(asks, price, owner_id, ID_ToBeRemoved, side);
}

std::optional<Order> Book::removeFromSide(std::map<int, Location_Rest> &Level,
                                          const int &price, const int &owner_id,
                                          const int &order_id,
                                          const Side &side) {

  auto levelIt = Level.find(price);

  if (levelIt == Level.end())
    return std::nullopt;

  auto locationIt = location.find(order_id);

  if (locationIt == location.end())
    return std::nullopt;

  LevelNode *node = locationIt->second.location;

  if (node == NULL)
    return std::nullopt;

  // The order exists, but it does not belong to the owner that
  // requested the cancellation.
  if (node->owner_id != owner_id)
    return std::nullopt;

  // Save everything BEFORE deleting the node.
  Order removed = {node->order_id, node->owner_id, side,
                   node->quantity, price,          0};

  // Remove node from doubly linked list.

  if (node->prev != NULL)
    node->prev->nxt = node->nxt;
  else
    levelIt->second.first = node->nxt;

  if (node->nxt != NULL)
    node->nxt->prev = node->prev;
  else
    levelIt->second.last = node->prev;

  // Remove ID lookup before deleting the actual node.
  location.erase(locationIt);

  delete node;

  // No orders remain at this price.
  if (levelIt->second.first == NULL)
    Level.erase(levelIt);

  return removed;
}

void Book::rest_order(const Order &order) {

  if (location.find(order.order_id) != location.end()) {

    throw std::logic_error("attempted to rest duplicate active order id");
  }

  if (order.side == Side::Buy) {

    if (bids.find(order.price) == bids.end())
      init_level(order, bids);
    else
      add_order(order, bids);

  } else {

    if (asks.find(order.price) == asks.end())
      init_level(order, asks);
    else
      add_order(order, asks);
  }
}

void Book::init_level(const Order &order,
                      std::map<int, Location_Rest> &Levels) {

  LevelNode *location_Ptr =
      new LevelNode{NULL, NULL, order.order_id, order.owner_id, order.quantity};

  Levels.emplace(order.price, Location_Rest{location_Ptr, location_Ptr});

  location.emplace(order.order_id, Location_ID{location_Ptr, order.side,
                                               order.owner_id, order.price});
}

void Book::add_order(const Order &order, std::map<int, Location_Rest> &Levels) {

  LevelNode *location_Ptr =
      new LevelNode{NULL, Levels[order.price].last, order.order_id,
                    order.owner_id, order.quantity};

  Levels[order.price].last->nxt = location_Ptr;

  Levels[order.price].last = location_Ptr;

  location.emplace(order.order_id, Location_ID{location_Ptr, order.side,
                                               order.owner_id, order.price});
}

void Book::matchBuy(Order &order, std::vector<OrderEvent> &responses,
                    const int &time) {

  while (!asks.empty() && asks.begin()->first <= order.price &&
         order.quantity > 0) {

    auto it = asks.begin()->second.first;

    int price = asks.begin()->first; /// we use the price of the SELL order

    while (order.quantity > 0 && it != NULL) {

      int delta = std::min(order.quantity, it->quantity);

      order.quantity -= delta;
      it->quantity -= delta;

      responses.push_back(Fill(order.order_id, order.owner_id, order.side,
                               order.quantity, delta, price, time));

      responses.push_back(Fill(it->order_id, it->owner_id, Side::Sell,
                               it->quantity, delta, price, time));

      if (it->quantity == 0) {

        location.erase(it->order_id);

        auto aux = it->nxt;

        delete it;

        it = aux;
      }
    }

    /// change the first element
    if (it != NULL) {

      it->prev = NULL;

      asks.begin()->second.first = it;

    } else {

      /// we ran out of orders then we delete the level

      asks.erase(asks.begin());
    }
  }
}

void Book::matchSell(Order &order, std::vector<OrderEvent> &responses,
                     const int &time) {

  while (!bids.empty() && bids.rbegin()->first >= order.price &&
         order.quantity > 0) {

    auto it = bids.rbegin()->second.first;

    int price = bids.rbegin()->first; /// we use the price of the SELL order

    while (order.quantity > 0 && it != NULL) {

      int delta = std::min(order.quantity, it->quantity);

      order.quantity -= delta;
      it->quantity -= delta;

      responses.push_back(Fill(order.order_id, order.owner_id, order.side,
                               order.quantity, delta, price, time));

      responses.push_back(Fill(it->order_id, it->owner_id, Side::Buy,
                               it->quantity, delta, price, time));

      if (it->quantity == 0) {

        location.erase(it->order_id);

        auto aux = it->nxt;

        delete it;

        it = aux;
      }
    }

    /// change the first element
    if (it != NULL) {

      it->prev = NULL;

      bids.rbegin()->second.first = it;

    } else {

      /// we ran out of orders : we delete the level
      /// bids is 100% not empty, otherwise we would not have entered
      /// the while

      auto lastLevelIt = std::prev(bids.end());

      bids.erase(lastLevelIt);
    }
  }
}

OrderEvent Book::Fill(const int &order_id, const int &owner_id,
                      const Side &side, const int &remaining_quantity,
                      const int &traded_quantity, const int &price,
                      const int &time) {

  ResponseEvents type;

  if (remaining_quantity > 0)
    type = ResponseEvents::OrderPartialFilled;
  else
    type = ResponseEvents::OrderFilled;

  return {side, type, order_id, owner_id, traded_quantity, price, time};
}
