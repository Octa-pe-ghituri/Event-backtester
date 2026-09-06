#include "backtester/portfolio.hpp"

void Portfolio::apply(const OrderEvent &event) {

  if (event.type != ResponseEvents::OrderFilled &&
      event.type != ResponseEvents::OrderPartialFilled)
    return;

  if (event.side == Side::Buy) {

    cash_ -= event.quantity * event.price;
    position_ += event.quantity;

  } else {

    cash_ += event.quantity * event.price;
    position_ -= event.quantity;
  }

  exposure_ = position_ * event.price;
}

long long Portfolio::cash() const { return cash_; }

long long Portfolio::position() const { return position_; }

long long Portfolio::exposure() const { return exposure_; }
