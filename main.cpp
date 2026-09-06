#include "backtester/backtest.hpp"
// #include "backtester/strategy.hpp"
#include "backtester/fair_price_strategy.hpp"

int main() {

  FairPriceStrategyConfig strategyConfig{
      "AAPL", -0.482072524654, 8.509332824319, 6.0, 1.0, 6};

  auto strategy = std::make_unique<FairPriceStrategy>(strategyConfig);

  Backtest backtest(

      "data/lesson07_simulation_events_adapted.txt",

      std::move(strategy),

      359,

      0,

      1, 0.05);

  backtest.run();

  backtest.printSummary();
}
