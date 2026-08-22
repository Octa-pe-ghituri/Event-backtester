#include <algorithm> // luat de la Vlad; necesar pentru std::min
#include <cassert>
#include <cstddef>
#include <fstream>  // luat de la Vlad
#include <iostream> // luat de la Vlad
#include <map>
#include <memory> // luat de la Vlad
#include <optional>
#include <queue>
#include <stdexcept> // luat de la Vlad
#include <string>
#include <unordered_map>
#include <utility> // luat de la Vlad
#include <vector>

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

template <typename Event> class EventQueue {

private:
  struct EventInQueue {
    Event event;
    long long order;
  };

  struct Compare_Events {

    bool operator()(const EventInQueue &lhs, const EventInQueue &rhs) const {

      if (lhs.event.time == rhs.event.time) {
        return lhs.order > rhs.order;
      }

      return lhs.event.time > rhs.event.time;
    };
  };

  std ::priority_queue<EventInQueue, std::vector<EventInQueue>, Compare_Events>
      events;
  long long order_counter = 0;

public:
  void AddEvent(const Event &event) {

    events.push({event, order_counter});
    order_counter++;
  }

  bool IsEmpty() const { return events.empty(); }

  std ::vector<Event> GetEvents(const int &time) {

    std::vector<Event> result;

    while (!events.empty() && events.top().event.time <= time) {

      result.push_back(events.top().event);
      events.pop();
    }

    return result;
  }
};

class Book {

public:
  std ::vector<OrderEvent> ProcessOrder(const BackTestEvent &Event) {

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

private:
  struct LevelNode {

    LevelNode *nxt, *prev;
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

    LevelNode *first, *last;
  };

  std ::map<int, Location_Rest> bids;
  std ::map<int, Location_Rest> asks;
  std ::unordered_map<int, Location_ID> location;

  std ::vector<OrderEvent> ProcessAdd(const BackTestEvent &Event) {

    std ::vector<OrderEvent> responses = {
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

  std::vector<OrderEvent> ProcessCancel(const BackTestEvent &Event) {

    std::optional<Order> removed = try_remove(Event.order_id, Event.owner_id);

    if (!removed.has_value()) {

      return {{Event.side, ResponseEvents::OrderCancelFailed, Event.order_id,
               Event.owner_id, Event.quantity, Event.price, Event.time}};
    }

    return {{removed->side, ResponseEvents::OrderCancelled, removed->order_id,
             removed->owner_id, removed->quantity, removed->price, Event.time}};
  }

  std::vector<OrderEvent> ProcessMod(const BackTestEvent &Event) {

    std::optional<Order> removed = try_remove(Event.order_id, Event.owner_id);

    if (!removed.has_value()) {

      return {{Event.side, ResponseEvents::OrderModifyFailed, Event.order_id,
               Event.owner_id, Event.quantity, Event.price, Event.time}};
    }

    std::vector<OrderEvent> responses = {
        {removed->side, ResponseEvents::OrderModified, removed->order_id,
         removed->owner_id, Event.quantity, Event.price, Event.time}};

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

  std ::vector<OrderEvent> ProcessIOC(const BackTestEvent &Event) {

    std ::vector<OrderEvent> responses = {
        {Event.side, ResponseEvents::OrderAccepted, Event.order_id,
         Event.owner_id, Event.quantity, Event.price, Event.time}};

    Order order_data = {Event.order_id, Event.owner_id, Event.side,
                        Event.quantity, Event.price,    Event.time};

    if (order_data.side == Side::Buy) {
      matchBuy(order_data, responses, Event.time);
    } else {
      matchSell(order_data, responses, Event.time);
    }

    return responses;
  }

  std ::vector<OrderEvent> ProcessMarket(const BackTestEvent &Event) {

    /// the exact same logic, but we don't care about the price, only to get the
    /// quantity

    std ::vector<OrderEvent> responses = {
        {Event.side, ResponseEvents::OrderAccepted, Event.order_id,
         Event.owner_id, Event.quantity, Event.price, Event.time}};

    Order order_data = {Event.order_id, Event.owner_id, Event.side,
                        Event.quantity, Event.price,    Event.time};

    if (Event.side == Side::Buy) {

      marketBuy(order_data, responses, Event.time);
    } else { /// Sell

      marketSell(order_data, responses, Event.time);
    }

    if (order_data.quantity == 0) {

      responses.push_back({Event.side, ResponseEvents::OrderMarketFilled,
                           Event.order_id, Event.owner_id, Event.quantity,
                           Event.price, Event.time});
    }

    return responses;
  }

  void marketBuy(Order &order, std ::vector<OrderEvent> &responses,
                 const int &time) {

    while (!asks.empty() && /**asks.begin()->first <= order.price &&*/
           order.quantity > 0) {

      auto it = asks.begin()->second.first;

      int price = asks.begin()->first; /// we use the price of the SELL order

      while (order.quantity > 0 && it != NULL) {

        int delta = std ::min(order.quantity, it->quantity);

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

        /// we ran out of orders than we delete the level

        asks.erase(asks.begin());
      }
    }
  }

  void marketSell(Order &order, std ::vector<OrderEvent> &responses,
                  const int &time) {

    while (!bids.empty() && /**bids.rbegin()->first >= order.price &&*/
           order.quantity > 0) {

      auto it = bids.rbegin()->second.first;

      int price = bids.rbegin()->first; /// we use the price of the SELL order

      while (order.quantity > 0 && it != NULL) {

        int delta = std ::min(order.quantity, it->quantity);

        order.quantity -= delta;
        it->quantity -= delta;

        responses.push_back(Fill(order.order_id, order.owner_id,
                                 order.side, /// SELL
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
        /// bids is 100% not empty, otherwise we would not have entered at while

        auto it = prev(bids.end());

        bids.erase(it);
      }
    }
  }

  std::optional<Order> try_remove(const int &ID_ToBeRemoved,
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

  std::optional<Order> removeFromSide(std::map<int, Location_Rest> &Level,
                                      const int &price, const int &owner_id,
                                      const int &order_id, const Side &side) {

    auto levelIt = Level.find(price);

    if (levelIt == Level.end())
      return std::nullopt;

    auto locationIt = location.find(order_id);

    if (locationIt == location.end())
      return std::nullopt;

    LevelNode *node = locationIt->second.location;

    if (node == NULL)
      return std::nullopt;

    // The order exists, but it does not belong to the owner that requested the
    // cancellation.
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

  void rest_order(const Order &order) {

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

  void init_level(const Order &order, std ::map<int, Location_Rest> &Levels) {

    LevelNode *location_Ptr = new LevelNode{NULL, NULL, order.order_id,
                                            order.owner_id, order.quantity};

    Levels.emplace(order.price, Location_Rest{location_Ptr, location_Ptr});

    location.emplace(order.order_id, Location_ID{location_Ptr, order.side,
                                                 order.owner_id, order.price});
  }

  void add_order(const Order &order, std ::map<int, Location_Rest> &Levels) {

    LevelNode *location_Ptr =
        new LevelNode{NULL, Levels[order.price].last, order.order_id,
                      order.owner_id, order.quantity};

    Levels[order.price].last->nxt = location_Ptr;
    Levels[order.price].last = location_Ptr;

    location.emplace(order.order_id, Location_ID{location_Ptr, order.side,
                                                 order.owner_id, order.price});
  }

  void matchBuy(Order &order, std ::vector<OrderEvent> &responses,
                const int &time) {

    while (!asks.empty() && asks.begin()->first <= order.price &&
           order.quantity > 0) {

      auto it = asks.begin()->second.first;

      int price = asks.begin()->first; /// we use the price of the SELL order

      while (order.quantity > 0 && it != NULL) {

        int delta = std ::min(order.quantity, it->quantity);

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

        /// we ran out of orders than we delete the level

        asks.erase(asks.begin());
      }
    }
  }

  void matchSell(Order &order, std ::vector<OrderEvent> &responses,
                 const int &time) {

    while (!bids.empty() && bids.rbegin()->first >= order.price &&
           order.quantity > 0) {

      auto it = bids.rbegin()->second.first;

      int price = bids.rbegin()->first; /// we use the price of the SELL order

      while (order.quantity > 0 && it != NULL) {

        int delta = std ::min(order.quantity, it->quantity);

        order.quantity -= delta;
        it->quantity -= delta;

        responses.push_back(Fill(order.order_id, order.owner_id,
                                 order.side, /// SELL
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
        /// bids is 100% not empty, otherwise we would not have entered at while

        auto it = prev(bids.end());

        bids.erase(it);
      }
    }
  }

  OrderEvent Fill(const int &order_id, const int &owner_id, const Side &side,
                  const int &remaining_quantity, const int &traded_quantity,
                  const int &price, const int &time) {

    ResponseEvents type;

    if (remaining_quantity > 0)
      type = ResponseEvents::OrderPartialFilled;
    else
      type = ResponseEvents::OrderFilled;

    return {side, type, order_id, owner_id, traded_quantity, price, time};
  }

  OrderEvent FillMarket(const int &order_id, const int &owner_id,
                        const Side &side, const int &remaining_quantity,
                        const int &traded_quantity, const int &price,
                        const int &time) {

    ResponseEvents type;

    if (remaining_quantity > 0)
      type = ResponseEvents::OrderMarketPartialFill;
    else
      type = ResponseEvents::OrderMarketFilled;

    return {side, type, order_id, owner_id, traded_quantity, price, time};
  }
};

class Portfolio {
public:
  void apply(const OrderEvent &event) {
    if (event.type != ResponseEvents::OrderFilled &&
        event.type != ResponseEvents::OrderPartialFilled) {
      return;
    }

    if (event.side == Side::Buy) {
      cash_ -= event.quantity * event.price;
      position_ += event.quantity;
    } else {
      cash_ += event.quantity * event.price;
      position_ -= event.quantity;
    }

    exposure_ = position_ * event.price;
  }

  long long cash() const { return cash_; }
  long long position() const { return position_; }
  long long exposure() const { return exposure_; }

private:
  long long cash_{0};
  long long position_{0};
  long long exposure_{0};
};

// luat de la Vlad - acelasi rol ca OrderCommand din codul lui Vlad.
// Adaptat doar pentru tipurile mele + symbol pentru multiple Book-uri.

struct OrderCommand {

  Symbol symbol;
  OrderType order_type = OrderType::Add;

  int order_id = 0;
  Side side = Side::Buy;
  int quantity = 0;
  int price = 0;
};

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

// luat de la Vlad
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

  while (input >> time >> symbol >> typeText >> orderId >> ownerId >>
         sideText >> quantity >> price) {

    events.push_back({symbol, ParseSide(sideText), ParseOrderType(typeText),
                      orderId, ownerId, quantity, price, time});
  }

  return events;
}

// luat de la Vlad.
// eu am:
//
//      const std::map<Symbol, Book>& books
//
// fiindca Backtest-ul suporta mai multe simboluri.
class Strategy {

public:
  virtual ~Strategy() = default;

  virtual std::optional<OrderCommand> onTimeMove(

      int now,

      const std::map<Symbol, Book> &books,

      const Portfolio &portfolio,

      const std::vector<OrderEvent> &recentEvents

      ) = 0;
};

// luat de la Vlad aproape 1 la 1.
// Singura diferenta este parametrul books.
class DoNothingStrategy : public Strategy {

public:
  std::optional<OrderCommand> onTimeMove(

      int now,

      const std::map<Symbol, Book> &books,

      const Portfolio &portfolio,

      const std::vector<OrderEvent> &recentEvents

      ) override {

    (void)now;
    (void)books;
    (void)portfolio;
    (void)recentEvents;

    return std::nullopt;
  }
};

// luat de la Vlad.
//
//      std::map<Symbol, Book> books_;
//
class Backtest {

public:
  Backtest(

      std::string eventsPath,

      std::unique_ptr<Strategy> strategy,

      int maxTime,

      int strategyLatency,

      int strategyOwnerId

      )

      : maxTime_(maxTime),

        strategyLatency_(strategyLatency),

        strategyOwnerId_(strategyOwnerId),

        strategy_(std::move(strategy)) {

    // luat de la Vlad
    if (strategy_ == nullptr) {

      throw std::invalid_argument("strategy must not be null");
    }

    // luat de la Vlad
    // adaptat doar la numele tau EventQueue::AddEvent.
    for (const BackTestEvent &event : LoadEventsFromFile(eventsPath)) {

      eventQueue_.AddEvent(event);
    }
  }

  // luat de la Vlad
  void run() {

    while (now_ <= maxTime_ && !eventQueue_.IsEmpty()) {

      std::vector<BackTestEvent> dueEvents = eventQueue_.GetEvents(now_);

      for (const BackTestEvent &event : dueEvents) {

        processEvent(event);
      }

      // luat de la Vlad
      std::optional<OrderCommand> command = strategy_->onTimeMove(

          now_,

          books_,

          portfolio_,

          strategyEventsThisTick_);

      if (command.has_value()) {

        scheduleCommand(*command);
      }

      // luat de la Vlad
      strategyEventsThisTick_.clear();

      now_++;
    }
  }

  // luat de la Vlad
  void printSummary() const {

    std::cout

        << "cash=" << portfolio_.cash()

        << " position=" << portfolio_.position()

        << " exposure=" << portfolio_.exposure()

        << "\n";
  }

  // pentru ca mai tarziu sa pot inspecta toate
  // book-urile din Strategy/tester.
  const std::map<Symbol, Book> &books() const { return books_; }

private:
  void processEvent(const BackTestEvent &event) {

    // Daca symbol-ul nu exista:
    //
    //      books_[event.symbol]
    //
    // construieste automat un Book gol.
    //
    // Daca exista:
    // il foloseste pe cel existent.
    Book &book = books_[event.symbol];

    std::vector<OrderEvent> responses = book.ProcessOrder(event);

    for (const OrderEvent &response : responses) {

      std::cout

          << "t=" << now_

          << " symbol=" << event.symbol

          << " order=" << response.order_id

          << " owner=" << response.owner_id

          << " event=" << static_cast<int>(response.type)

          << " qty=" << response.quantity

          << " price=" << response.price

          << "\n";

      // luat de la Vlad
      if (response.owner_id == strategyOwnerId_) {

        strategyEventsThisTick_.push_back(response);

        portfolio_.apply(response);
      }
    }
  }

  void scheduleCommand(const OrderCommand &command) {

    BackTestEvent event{

        command.symbol,

        command.side,

        command.order_type,

        command.order_id,

        strategyOwnerId_,

        command.quantity,

        command.price,

        now_ + strategyLatency_};

    eventQueue_.AddEvent(event);
  }

  // luat de la Vlad
  int now_{0};

  int maxTime_{0};

  int strategyLatency_{1};

  int strategyOwnerId_{1};

  std::map<Symbol, Book> books_;

  // luat de la Vlad
  Portfolio portfolio_;

  EventQueue<BackTestEvent> eventQueue_;

  // luat de la Vlad
  std::vector<OrderEvent> strategyEventsThisTick_;

  // luat de la Vlad
  std::unique_ptr<Strategy> strategy_;
};

int main() {

  // luat de la Vlad
  auto strategy = std::make_unique<DoNothingStrategy>();

  // luat de la Vlad
  Backtest backtest(

      "data/historical_events.txt",

      std::move(strategy),

      10,

      1,

      1);

  // luat de la Vlad
  backtest.run();

  backtest.printSummary();
}