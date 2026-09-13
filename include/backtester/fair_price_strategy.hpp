#pragma once

#include "backtester/strategy.hpp"

struct FairPriceStrategyConfig {

        Symbol symbol;

        double bias = 0.0;

        double imbalanceWeight = 0.0;

        double baseEdge = 0.0;

        double inventoryPenalty = 0.0;

        long long maxPosition = 0;
};

class FairPriceStrategy : public Strategy {

    public:
        explicit FairPriceStrategy(FairPriceStrategyConfig config);

        std::optional<OrderCommand> onTimeMove(int now, const std::map<Symbol, Book> &books,
                                               const std::map<Symbol, MarketAnalytics> &analytics,
                                               const Portfolio &portfolio,
                                               const std::vector<StrategyEvent> &recentEvents) override;

    private:
        FairPriceStrategyConfig config_;
};
