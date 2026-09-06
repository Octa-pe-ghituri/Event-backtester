#pragma once

#include "backtester/types.hpp"

#include <map>

class Portfolio {

public:
  void apply(const StrategyEvent &event);

  long long cash() const;

  long long position(const Symbol &symbol) const;

  const std::map<Symbol, long long> &positions() const;

private:
  long long cash_{0};

  std::map<Symbol, long long> positions_;
};
