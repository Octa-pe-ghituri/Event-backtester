#include "backtester/backtest.hpp"
// #include "backtester/strategy.hpp"
#include "backtester/fair_price_strategy.hpp"

int main() {

  FairPriceStrategyConfig strategyConfig{
      "AAPL", -0.482072524654, 8.509332824319, 6.0, 1.0, 6};

  auto strategy = std::make_unique<FairPriceStrategy>(strategyConfig);

  Backtest backtest(

      "data/historical_events.txt",

      std::move(strategy),

      10,

      1,

      1);

  backtest.run();

  backtest.printSummary();
}
