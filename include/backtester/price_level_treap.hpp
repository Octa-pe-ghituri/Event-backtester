#pragma once

#include "backtester/types.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <random>
#include <utility>
#include <vector>

class PriceLevelTreap {

    public:
        static constexpr std::uint64_t defaultSeed =
            0x9E3779B97F4A7C15ULL;

        struct Level {

                int price = 0;
                std::int64_t quantity = 0;
        };

        explicit PriceLevelTreap(
            Side side,
            std::uint64_t seed = defaultSeed
        );

        PriceLevelTreap(const PriceLevelTreap &) = delete;
        PriceLevelTreap &operator=(const PriceLevelTreap &) = delete;

        PriceLevelTreap(PriceLevelTreap &&) noexcept = default;
        PriceLevelTreap &operator=(PriceLevelTreap &&) noexcept = default;

        void reserve(std::size_t expectedLevels);

        // Sets the absolute quantity at one price.
        // A quantity of zero removes that price level.
        void setLevel(int price, std::int64_t quantity);

        [[nodiscard]] Side side() const noexcept;

        [[nodiscard]] bool empty() const noexcept;

        [[nodiscard]] std::size_t levelCount() const noexcept;

        [[nodiscard]] std::int64_t totalQuantity() const noexcept;

        [[nodiscard]] std::int64_t
        quantityAtPrice(int price) const noexcept;

        // Both boundaries are included.
        [[nodiscard]] std::int64_t
        quantityBetween(int lowestPrice, int highestPrice) const noexcept;

        // Both boundaries are included.
        [[nodiscard]] std::size_t
        levelsBetween(int lowestPrice, int highestPrice) const noexcept;

        [[nodiscard]] std::optional<int>
        lowestPrice() const noexcept;

        [[nodiscard]] std::optional<int>
        highestPrice() const noexcept;

        // Highest bid or lowest ask, depending on this Treap's side.
        [[nodiscard]] std::optional<int>
        bestPrice() const noexcept;

        // Includes the best price and all prices within the given distance.
        [[nodiscard]] std::int64_t
        quantityWithinTicks(int ticks) const;

        // Adds quantities from the best price outward.
        [[nodiscard]] std::int64_t
        quantityWithinBestLevels(std::size_t numberOfLevels) const noexcept;

        // Finds the price containing the requested cumulative quantity,
        // measured from the best price outward.
        [[nodiscard]] std::optional<int>
        priceAtDepth(std::int64_t cumulativeQuantity) const noexcept;

        // Returns levels ordered from best to worse.
        [[nodiscard]] std::vector<Level>
        topLevels(std::size_t maximumLevels) const;

        void clear() noexcept;

        // Used by tests and debugging to verify all Treap invariants.
        [[nodiscard]] bool validate() const noexcept;

        [[nodiscard]] std::size_t height() const noexcept;

    private:
        static constexpr int nullIndex = -1;

        struct Node {

                int price = 0;
                std::int64_t quantity = 0;

                std::int64_t subtreeQuantity = 0;
                std::size_t subtreeLevels = 0;

                std::uint64_t priority = 0;

                int left = nullIndex;
                int right = nullIndex;
        };

        Side side_;

        int root_ = nullIndex;

        std::vector<Node> nodes_;
        std::vector<int> freeIndices_;

        std::mt19937_64 randomEngine_;

        [[nodiscard]] std::int64_t
        subtreeQuantity(int nodeIndex) const noexcept;

        [[nodiscard]] std::size_t
        subtreeLevels(int nodeIndex) const noexcept;

        void updateAggregates(int nodeIndex) noexcept;

        [[nodiscard]] bool
        hasHigherPriority(int firstIndex, int secondIndex) const noexcept;

        [[nodiscard]] int
        findNode(int price) const noexcept;

        [[nodiscard]] int
        createNode(int price, std::int64_t quantity);

        void releaseNode(int nodeIndex) noexcept;

        [[nodiscard]] std::pair<int, int>
        splitLessThan(int nodeIndex, int price);

        [[nodiscard]] int
        merge(int leftRoot, int rightRoot);

        [[nodiscard]] int
        insertNode(int currentRoot, int newNode);

        [[nodiscard]] int
        eraseNode(int currentRoot, int price);

        void updateQuantity(
            int currentRoot,
            int price,
            std::int64_t quantity
        ) noexcept;

        [[nodiscard]] std::int64_t
        quantityStrictlyBelow(int price) const noexcept;

        [[nodiscard]] std::int64_t
        quantityAtOrBelow(int price) const noexcept;

        [[nodiscard]] std::size_t
        levelsStrictlyBelow(int price) const noexcept;

        [[nodiscard]] std::size_t
        levelsAtOrBelow(int price) const noexcept;

        [[nodiscard]] std::int64_t
        quantityWithinBestLevels(
            int nodeIndex,
            std::size_t numberOfLevels
        ) const noexcept;

        [[nodiscard]] std::optional<int>
        priceAtDepth(
            int nodeIndex,
            std::int64_t cumulativeQuantity
        ) const noexcept;

        void collectTopLevels(
            int nodeIndex,
            std::size_t maximumLevels,
            std::vector<Level> &result
        ) const;

        [[nodiscard]] bool validateNode(
            int nodeIndex,
            std::optional<int> lowerBound,
            std::optional<int> upperBound,
            std::int64_t &calculatedQuantity,
            std::size_t &calculatedLevels,
            std::size_t &calculatedHeight
        ) const noexcept;
};
