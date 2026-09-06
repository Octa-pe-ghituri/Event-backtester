#include "backtester/backtest.hpp"

#include "backtester/io.hpp"
#include "backtester/types.hpp"

#include <iostream>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

Backtest::Backtest(std::string eventsPath, std::unique_ptr<Strategy> strategy,
                   int maxTime, int strategyLatency, int strategyOwnerId)
    : maxTime_(maxTime), strategyLatency_(strategyLatency),
      strategyOwnerId_(strategyOwnerId), strategy_(std::move(strategy)) {

  if (strategy_ == nullptr) {

    throw std::invalid_argument("strategy must not be null");
  }

  if (maxTime_ < 0) {

    throw std::invalid_argument("maxTime must not be negative");
  }

  if (strategyLatency_ < 0) {

    throw std::invalid_argument("strategyLatency must not be negative");
  }

  if (strategyOwnerId_ < 0) {

    throw std::invalid_argument("strategyOwnerId must not be negative");
  }

  for (const BackTestEvent &event : LoadEventsFromFile(eventsPath)) {

    eventQueue_.AddEvent(event);
  }
}

void Backtest::processDueEvents() {

  std::vector<BackTestEvent> dueEvents = eventQueue_.GetEvents(now_);

  for (const BackTestEvent &event : dueEvents) {

    processEvent(event);
  }
}

void Backtest::run() {

  while (now_ <= maxTime_) {

    processDueEvents();

    std::optional<OrderCommand> command = strategy_->onTimeMove(
        now_, books_, portfolio_, strategyEventsThisTick_);

    // Strategy tocmai a consumat evenimentele primite.
    strategyEventsThisTick_.clear();

    if (command.has_value()) {

      scheduleCommand(*command);

      // Daca latency este 0, ordinul trebuie sa devina
      // activ chiar la timpul now_.
      //
      // Strategy si-a luat deja decizia pentru acest tick,
      // deci fills-urile rezultate vor fi vazute la
      // urmatoarea apelare onTimeMove().
      processDueEvents();
    }

    now_++;
  }
}

void Backtest::printSummary() const {

  std::cout << "cash=" << portfolio_.cash() << "\n";

  for (const auto &entry : portfolio_.positions()) {

    std::cout << "position[" << entry.first << "]=" << entry.second << "\n";
  }
}

// pentru ca mai tarziu sa pot inspecta toate
// book-urile din Strategy/tester.
const std::map<Symbol, Book> &Backtest::books() const { return books_; }

void Backtest::processEvent(const BackTestEvent &event) {

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

    std::cout << "t=" << now_ << " symbol=" << event.symbol
              << " order=" << response.order_id
              << " owner=" << response.owner_id
              << " event=" << static_cast<int>(response.type)
              << " qty=" << response.quantity << " price=" << response.price
              << "\n";

    // luat de la Vlad
    if (response.owner_id == strategyOwnerId_) {

      StrategyEvent strategyEvent{event.symbol, response};

      strategyEventsThisTick_.push_back(strategyEvent);

      portfolio_.apply(strategyEvent);
    }
  }
}

void Backtest::scheduleCommand(const OrderCommand &command) {

  BackTestEvent event{command.symbol,     command.side,
                      command.order_type, command.order_id,
                      strategyOwnerId_,   command.quantity,
                      command.price,      now_ + strategyLatency_};

  eventQueue_.AddEvent(event);
}
