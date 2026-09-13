#include "backtester/backtest.hpp"
#include "backtester/config.hpp"
#include "backtester/fair_price_strategy.hpp"

#include <string>

int main(int argc, char *argv[]) {

    const std::string configPath = argc > 1 ? argv[1] : "config/backtest.cfg";

    const BacktestConfig backtestConfig = LoadBacktestConfigFromFile(configPath);

    FairPriceStrategyConfig strategyConfig{"AAPL", -0.482072524654, 8.509332824319, 6.0, 1.0, 6};

    auto strategy = std::make_unique<FairPriceStrategy>(strategyConfig);

    Backtest backtest(

        "data/lesson07_simulation_events_adapted.txt",

        std::move(strategy),

        backtestConfig);

    backtest.run();

    backtest.printSummary();
}
