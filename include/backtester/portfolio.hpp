#pragma once

#include "backtester/types.hpp"

#include <map>

class Portfolio {

public:
  explicit Portfolio(double feeTicks = 0.0);

  void apply(const StrategyEvent &event);

  double cash() const;

  long long position(const Symbol &symbol) const;

  const std::map<Symbol, long long> &positions() const;

private:
  double feeTicks_{0.0};

  double cash_{0.0};

  std::map<Symbol, long long> positions_;
};
