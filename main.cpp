#include "backtester/backtest.hpp"
#include "backtester/strategy.hpp"
#include <cassert>
#include <memory>
#include <string>
#include <utility>

int main() {

  auto strategy = std::make_unique<DoNothingStrategy>();

  Backtest backtest(

      "data/historical_events.txt",

      std::move(strategy),

      10,

      1,

      1);

  backtest.run();

  backtest.printSummary();
}
