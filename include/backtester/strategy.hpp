#pragma once

#include "backtester/book.hpp"
#include "backtester/portfolio.hpp"
#include "backtester/types.hpp"

#include <map>
#include <optional>
#include <vector>

struct OrderCommand {

        Symbol symbol;

        OrderType order_type = OrderType::Add;

        int order_id = 0;

        Side side = Side::Buy;

        int quantity = 0;

        int price = 0;
};

class Strategy {

    public:
        virtual ~Strategy() = default;

        virtual std::optional<OrderCommand> onTimeMove(int now, const std::map<Symbol, Book> &books,
                                                       const Portfolio &portfolio,
                                                       const std::vector<StrategyEvent> &recentEvents) = 0;
};

class DoNothingStrategy : public Strategy {

    public:
        std::optional<OrderCommand> onTimeMove(int now, const std::map<Symbol, Book> &books, const Portfolio &portfolio,
                                               const std::vector<StrategyEvent> &recentEvents) override {

            (void)now;
            (void)books;
            (void)portfolio;
            (void)recentEvents;

            return std::nullopt;
        }
};
