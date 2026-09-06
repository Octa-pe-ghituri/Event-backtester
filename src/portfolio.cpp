#include "backtester/portfolio.hpp"

Portfolio::Portfolio(double feeTicks) : feeTicks_(feeTicks) {}

void Portfolio::apply(const StrategyEvent &strategyEvent) {

  const OrderEvent &event = strategyEvent.event;

  if (event.type != ResponseEvents::OrderFilled &&
      event.type != ResponseEvents::OrderPartialFilled) {

    return;
  }

  double notional =
      static_cast<double>(event.quantity) * static_cast<double>(event.price);

  double fee = feeTicks_ * static_cast<double>(event.quantity);

  if (event.side == Side::Buy) {

    cash_ -= notional + fee;

    positions_[strategyEvent.symbol] += event.quantity;

  } else {

    cash_ += notional - fee;

    positions_[strategyEvent.symbol] -= event.quantity;
  }
}

double Portfolio::cash() const { return cash_; }

long long Portfolio::position(const Symbol &symbol) const {

  auto it = positions_.find(symbol);

  if (it == positions_.end()) {

    return 0;
  }

  return it->second;
}

const std::map<Symbol, long long> &Portfolio::positions() const {

  return positions_;
}
