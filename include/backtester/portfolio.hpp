#pragma once

#include "backtester/types.hpp"

class Portfolio {

public:
  void apply(const OrderEvent &event);

  long long cash() const;
  long long position() const;
  long long exposure() const;

private:
  long long cash_{0};
  long long position_{0};
  long long exposure_{0};
};
