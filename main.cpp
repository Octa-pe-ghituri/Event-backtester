#include "backtester/book.hpp"
#include "backtester/event_queue.hpp"
#include "backtester/portfolio.hpp"
#include "backtester/types.hpp"
#include <cassert>
#include <fstream>  // luat de la Vlad
#include <iostream> // luat de la Vlad
#include <map>
#include <memory> // luat de la Vlad
#include <optional>
#include <stdexcept> // luat de la Vlad
#include <string>
#include <utility> // luat de la Vlad
#include <vector>
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
