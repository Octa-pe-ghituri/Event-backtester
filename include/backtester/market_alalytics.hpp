#pragma once

#include "backtester/price_level_treap.hpp"
#include "backtester/types.hpp"

#include <cstdint>

class MarketAnalytics {

    public:
        MarketAnalytics()
            : bids_(
                  Side::Buy,
                  PriceLevelTreap::defaultSeed
              ),
              asks_(
                  Side::Sell,
                  PriceLevelTreap::defaultSeed + 1
              ) {
        }

        void setLevel(
            const Side side,
            const int price,
            const std::int64_t quantity
        ) {
            if (side == Side::Buy) {
                bids_.setLevel(price, quantity);
            } else {
                asks_.setLevel(price, quantity);
            }
        }

        [[nodiscard]] const PriceLevelTreap &
        bids() const noexcept {
            return bids_;
        }

        [[nodiscard]] const PriceLevelTreap &
        asks() const noexcept {
            return asks_;
        }

        [[nodiscard]] bool validate() const noexcept {
            return
                bids_.validate() &&
                asks_.validate();
        }

    private:
        PriceLevelTreap bids_;
        PriceLevelTreap asks_;
};
