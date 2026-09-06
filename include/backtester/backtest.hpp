#pragma once

#include "backtester/book.hpp"
#include "backtester/event_queue.hpp"
#include "backtester/portfolio.hpp"
#include "backtester/strategy.hpp"
#include "backtester/types.hpp"

#include <map>
#include <memory>
#include <string>
#include <vector>

class Backtest {

public:
  Backtest(std::string eventsPath, std::unique_ptr<Strategy> strategy,
           int maxTime, int strategyLatency, int strategyOwnerId);

  void run();

  void printSummary() const;

  // pentru ca mai tarziu sa pot inspecta toate
  // book-urile din Strategy/tester.
  const std::map<Symbol, Book> &books() const;

private:
  void processEvent(const BackTestEvent &event);

  void scheduleCommand(const OrderCommand &command);

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
