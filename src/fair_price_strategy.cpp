#include "backtester/fair_price_strategy.hpp"

FairPriceStrategy::FairPriceStrategy(FairPriceStrategyConfig config)
    : config_(config) {}

std::optional<OrderCommand>
FairPriceStrategy::onTimeMove(int now, const std::map<Symbol, Book> &books,
                              const Portfolio &portfolio,
                              const std::vector<StrategyEvent> &recentEvents) {

  (void)now;
  (void)recentEvents;

  auto bookIt = books.find(config_.symbol);

  if (bookIt == books.end()) {

    return std::nullopt;
  }

  std::optional<TopOfBook> top = bookIt->second.topOfBook();

  if (!top.has_value()) {

    return std::nullopt;
  }

  long long totalQuantity = top->best_bid_quantity + top->best_ask_quantity;

  if (totalQuantity <= 0) {

    return std::nullopt;
  }

  double imbalance =
      static_cast<double>(top->best_bid_quantity - top->best_ask_quantity) /
      static_cast<double>(totalQuantity);

  double mid = (static_cast<double>(top->best_bid) +
                static_cast<double>(top->best_ask)) /
               2.0;

  double fairPrice = mid + config_.bias + config_.imbalanceWeight * imbalance;

  double buyEdge = fairPrice - static_cast<double>(top->best_ask);

  double sellEdge = static_cast<double>(top->best_bid) - fairPrice;

  long long position = portfolio.position(config_.symbol);

  double requiredBuyEdge =
      config_.baseEdge + position * config_.inventoryPenalty;

  double requiredSellEdge =
      config_.baseEdge - position * config_.inventoryPenalty;

  if (position < config_.maxPosition && buyEdge >= requiredBuyEdge) {

    return OrderCommand{config_.symbol, OrderType::Add, 0, Side::Buy, 1,
                        top->best_ask};
  }

  if (position > -config_.maxPosition && sellEdge >= requiredSellEdge) {

    return OrderCommand{config_.symbol, OrderType::Add, 0, Side::Sell, 1,
                        top->best_bid};
  }

  return std::nullopt;
}
