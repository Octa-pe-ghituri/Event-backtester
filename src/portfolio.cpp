#include "backtester/portfolio.hpp"

void Portfolio::apply(const StrategyEvent &strategyEvent) {

  const OrderEvent &event = strategyEvent.event;

  if (event.type != ResponseEvents::OrderFilled &&
      event.type != ResponseEvents::OrderPartialFilled) {

    return;
  }

  const long long notional = 1LL * event.quantity * event.price;

  if (event.side == Side::Buy) {

    cash_ -= notional;

    positions_[strategyEvent.symbol] += event.quantity;

  } else {

    cash_ += notional;

    positions_[strategyEvent.symbol] -= event.quantity;
  }
}

long long Portfolio::cash() const { return cash_; }

long long Portfolio::position(const Symbol &symbol) const {

  auto it = positions_.find(symbol);

  if (it == positions_.end())
    return 0;

  return it->second;
}

const std::map<Symbol, long long> &Portfolio::positions() const {

  return positions_;
}
