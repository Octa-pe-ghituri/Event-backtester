#include "backtester/price_level_treap.hpp"

#include <algorithm>
#include <limits>
#include <stdexcept>

PriceLevelTreap::PriceLevelTreap(
    const Side side,
    const std::uint64_t seed
)
    : side_(side),
      randomEngine_(seed) {
}

void PriceLevelTreap::reserve(
    const std::size_t expectedLevels
) {
    if (expectedLevels >
        static_cast<std::size_t>(std::numeric_limits<int>::max())) {

        throw std::length_error("too many Treap price levels");
    }

    nodes_.reserve(expectedLevels);
    freeIndices_.reserve(expectedLevels);
}

Side PriceLevelTreap::side() const noexcept {
    return side_;
}

bool PriceLevelTreap::empty() const noexcept {
    return root_ == nullIndex;
}

std::int64_t PriceLevelTreap::subtreeQuantity(
    const int nodeIndex
) const noexcept {
    if (nodeIndex == nullIndex) {
        return 0;
    }

    return nodes_[static_cast<std::size_t>(nodeIndex)]
        .subtreeQuantity;
}

std::size_t PriceLevelTreap::subtreeLevels(
    const int nodeIndex
) const noexcept {
    if (nodeIndex == nullIndex) {
        return 0;
    }

    return nodes_[static_cast<std::size_t>(nodeIndex)]
        .subtreeLevels;
}

void PriceLevelTreap::updateAggregates(
    const int nodeIndex
) noexcept {
    Node &node =
        nodes_[static_cast<std::size_t>(nodeIndex)];

    node.subtreeQuantity =
        subtreeQuantity(node.left) +
        node.quantity +
        subtreeQuantity(node.right);

    node.subtreeLevels =
        subtreeLevels(node.left) +
        1 +
        subtreeLevels(node.right);
}

bool PriceLevelTreap::hasHigherPriority(
    const int firstIndex,
    const int secondIndex
) const noexcept {
    const Node &first =
        nodes_[static_cast<std::size_t>(firstIndex)];

    const Node &second =
        nodes_[static_cast<std::size_t>(secondIndex)];

    if (first.priority != second.priority) {
        return first.priority > second.priority;
    }

    // Deterministic tie-breaker for the extremely unlikely
    // case where two random priorities are equal.
    return first.price > second.price;
}

int PriceLevelTreap::findNode(
    const int price
) const noexcept {
    int current = root_;

    while (current != nullIndex) {
        const Node &node =
            nodes_[static_cast<std::size_t>(current)];

        if (price == node.price) {
            return current;
        }

        current =
            price < node.price
                ? node.left
                : node.right;
    }

    return nullIndex;
}

int PriceLevelTreap::createNode(
    const int price,
    const std::int64_t quantity
) {
    Node newNode{
        price,
        quantity,
        quantity,
        1,
        randomEngine_(),
        nullIndex,
        nullIndex
    };

    if (!freeIndices_.empty()) {
        const int nodeIndex = freeIndices_.back();

        freeIndices_.pop_back();

        nodes_[static_cast<std::size_t>(nodeIndex)] =
            newNode;

        return nodeIndex;
    }

    if (nodes_.size() >=
        static_cast<std::size_t>(
            std::numeric_limits<int>::max()
        )) {

        throw std::length_error(
            "Treap node index capacity exceeded"
        );
    }

    // Ensure releaseNode() can return an index to the
    // free list without allocating memory.
    if (freeIndices_.capacity() < nodes_.size() + 1) {
        const std::size_t doubledCapacity =
            std::max<std::size_t>(
                1,
                freeIndices_.capacity() * 2
            );

        freeIndices_.reserve(
            std::max(
                nodes_.size() + 1,
                doubledCapacity
            )
        );
    }

    const int nodeIndex =
        static_cast<int>(nodes_.size());

    nodes_.push_back(newNode);

    return nodeIndex;
}

void PriceLevelTreap::releaseNode(
    const int nodeIndex
) noexcept {
    nodes_[static_cast<std::size_t>(nodeIndex)] =
        Node{};

    freeIndices_.push_back(nodeIndex);
}

std::pair<int, int> PriceLevelTreap::splitLessThan(
    const int nodeIndex,
    const int price
) {
    if (nodeIndex == nullIndex) {
        return {nullIndex, nullIndex};
    }

    if (nodes_[static_cast<std::size_t>(nodeIndex)]
            .price < price) {

        const auto [smallerRight, greaterOrEqual] =
            splitLessThan(
                nodes_[static_cast<std::size_t>(nodeIndex)]
                    .right,
                price
            );

        nodes_[static_cast<std::size_t>(nodeIndex)]
            .right = smallerRight;

        updateAggregates(nodeIndex);

        return {nodeIndex, greaterOrEqual};
    }

    const auto [smaller, greaterOrEqualLeft] =
        splitLessThan(
            nodes_[static_cast<std::size_t>(nodeIndex)]
                .left,
            price
        );

    nodes_[static_cast<std::size_t>(nodeIndex)]
        .left = greaterOrEqualLeft;

    updateAggregates(nodeIndex);

    return {smaller, nodeIndex};
}

int PriceLevelTreap::merge(
    const int leftRoot,
    const int rightRoot
) {
    if (leftRoot == nullIndex) {
        return rightRoot;
    }

    if (rightRoot == nullIndex) {
        return leftRoot;
    }

    if (hasHigherPriority(leftRoot, rightRoot)) {
        nodes_[static_cast<std::size_t>(leftRoot)]
            .right =
            merge(
                nodes_[static_cast<std::size_t>(leftRoot)]
                    .right,
                rightRoot
            );

        updateAggregates(leftRoot);

        return leftRoot;
    }

    nodes_[static_cast<std::size_t>(rightRoot)]
        .left =
        merge(
            leftRoot,
            nodes_[static_cast<std::size_t>(rightRoot)]
                .left
        );

    updateAggregates(rightRoot);

    return rightRoot;
}

int PriceLevelTreap::insertNode(
    const int currentRoot,
    const int newNode
) {
    if (currentRoot == nullIndex) {
        return newNode;
    }

    if (hasHigherPriority(newNode, currentRoot)) {
        const auto [leftRoot, rightRoot] =
            splitLessThan(
                currentRoot,
                nodes_[static_cast<std::size_t>(newNode)]
                    .price
            );

        nodes_[static_cast<std::size_t>(newNode)]
            .left = leftRoot;

        nodes_[static_cast<std::size_t>(newNode)]
            .right = rightRoot;

        updateAggregates(newNode);

        return newNode;
    }

    if (nodes_[static_cast<std::size_t>(newNode)].price <
        nodes_[static_cast<std::size_t>(currentRoot)].price) {

        nodes_[static_cast<std::size_t>(currentRoot)]
            .left =
            insertNode(
                nodes_[static_cast<std::size_t>(currentRoot)]
                    .left,
                newNode
            );

    } else {
        nodes_[static_cast<std::size_t>(currentRoot)]
            .right =
            insertNode(
                nodes_[static_cast<std::size_t>(currentRoot)]
                    .right,
                newNode
            );
    }

    updateAggregates(currentRoot);

    return currentRoot;
}

int PriceLevelTreap::eraseNode(
    const int currentRoot,
    const int price
) {
    if (currentRoot == nullIndex) {
        return nullIndex;
    }

    const int currentPrice =
        nodes_[static_cast<std::size_t>(currentRoot)]
            .price;

    if (price < currentPrice) {
        nodes_[static_cast<std::size_t>(currentRoot)]
            .left =
            eraseNode(
                nodes_[static_cast<std::size_t>(currentRoot)]
                    .left,
                price
            );

        updateAggregates(currentRoot);

        return currentRoot;
    }

    if (price > currentPrice) {
        nodes_[static_cast<std::size_t>(currentRoot)]
            .right =
            eraseNode(
                nodes_[static_cast<std::size_t>(currentRoot)]
                    .right,
                price
            );

        updateAggregates(currentRoot);

        return currentRoot;
    }

    const int mergedChildren =
        merge(
            nodes_[static_cast<std::size_t>(currentRoot)]
                .left,
            nodes_[static_cast<std::size_t>(currentRoot)]
                .right
        );

    releaseNode(currentRoot);

    return mergedChildren;
}

void PriceLevelTreap::updateQuantity(
    const int currentRoot,
    const int price,
    const std::int64_t quantity
) noexcept {
    Node &node =
        nodes_[static_cast<std::size_t>(currentRoot)];

    if (price < node.price) {
        updateQuantity(
            node.left,
            price,
            quantity
        );

    } else if (price > node.price) {
        updateQuantity(
            node.right,
            price,
            quantity
        );

    } else {
        node.quantity = quantity;
    }

    updateAggregates(currentRoot);
}

void PriceLevelTreap::setLevel(
    const int price,
    const std::int64_t quantity
) {
    if (quantity < 0) {
        throw std::invalid_argument(
            "Treap level quantity cannot be negative"
        );
    }

    const int existingNode = findNode(price);

    if (quantity == 0) {
        if (existingNode != nullIndex) {
            root_ = eraseNode(root_, price);
        }

        return;
    }

    if (existingNode != nullIndex) {
        const Node &node =
            nodes_[static_cast<std::size_t>(existingNode)];

        if (node.quantity != quantity) {
            updateQuantity(
                root_,
                price,
                quantity
            );
        }

        return;
    }

    const int newNode =
        createNode(price, quantity);

    root_ = insertNode(root_, newNode);
}

void PriceLevelTreap::clear() noexcept {
    root_ = nullIndex;

    nodes_.clear();
    freeIndices_.clear();
}

std::size_t PriceLevelTreap::levelCount() const noexcept {
    return subtreeLevels(root_);
}

std::int64_t PriceLevelTreap::totalQuantity() const noexcept {
    return subtreeQuantity(root_);
}

std::int64_t PriceLevelTreap::quantityAtPrice(
    const int price
) const noexcept {
    const int nodeIndex = findNode(price);

    if (nodeIndex == nullIndex) {
        return 0;
    }

    return nodes_[static_cast<std::size_t>(nodeIndex)]
        .quantity;
}

std::int64_t PriceLevelTreap::quantityStrictlyBelow(
    const int price
) const noexcept {
    std::int64_t quantity = 0;
    int current = root_;

    while (current != nullIndex) {
        const Node &node =
            nodes_[static_cast<std::size_t>(current)];

        if (price <= node.price) {
            current = node.left;

        } else {
            quantity +=
                subtreeQuantity(node.left) +
                node.quantity;

            current = node.right;
        }
    }

    return quantity;
}

std::int64_t PriceLevelTreap::quantityAtOrBelow(
    const int price
) const noexcept {
    std::int64_t quantity = 0;
    int current = root_;

    while (current != nullIndex) {
        const Node &node =
            nodes_[static_cast<std::size_t>(current)];

        if (price < node.price) {
            current = node.left;

        } else {
            quantity +=
                subtreeQuantity(node.left) +
                node.quantity;

            current = node.right;
        }
    }

    return quantity;
}

std::size_t PriceLevelTreap::levelsStrictlyBelow(
    const int price
) const noexcept {
    std::size_t levels = 0;
    int current = root_;

    while (current != nullIndex) {
        const Node &node =
            nodes_[static_cast<std::size_t>(current)];

        if (price <= node.price) {
            current = node.left;

        } else {
            levels +=
                subtreeLevels(node.left) + 1;

            current = node.right;
        }
    }

    return levels;
}

std::size_t PriceLevelTreap::levelsAtOrBelow(
    const int price
) const noexcept {
    std::size_t levels = 0;
    int current = root_;

    while (current != nullIndex) {
        const Node &node =
            nodes_[static_cast<std::size_t>(current)];

        if (price < node.price) {
            current = node.left;

        } else {
            levels +=
                subtreeLevels(node.left) + 1;

            current = node.right;
        }
    }

    return levels;
}

std::int64_t PriceLevelTreap::quantityBetween(
    const int lowestPrice,
    const int highestPrice
) const noexcept {
    if (lowestPrice > highestPrice) {
        return 0;
    }

    return
        quantityAtOrBelow(highestPrice) -
        quantityStrictlyBelow(lowestPrice);
}

std::size_t PriceLevelTreap::levelsBetween(
    const int lowestPrice,
    const int highestPrice
) const noexcept {
    if (lowestPrice > highestPrice) {
        return 0;
    }

    return
        levelsAtOrBelow(highestPrice) -
        levelsStrictlyBelow(lowestPrice);
}

std::optional<int>
PriceLevelTreap::lowestPrice() const noexcept {
    if (root_ == nullIndex) {
        return std::nullopt;
    }

    int current = root_;

    while (
        nodes_[static_cast<std::size_t>(current)]
            .left != nullIndex
    ) {
        current =
            nodes_[static_cast<std::size_t>(current)]
                .left;
    }

    return nodes_[static_cast<std::size_t>(current)]
        .price;
}

std::optional<int>
PriceLevelTreap::highestPrice() const noexcept {
    if (root_ == nullIndex) {
        return std::nullopt;
    }

    int current = root_;

    while (
        nodes_[static_cast<std::size_t>(current)]
            .right != nullIndex
    ) {
        current =
            nodes_[static_cast<std::size_t>(current)]
                .right;
    }

    return nodes_[static_cast<std::size_t>(current)]
        .price;
}

std::optional<int>
PriceLevelTreap::bestPrice() const noexcept {
    if (side_ == Side::Buy) {
        return highestPrice();
    }

    return lowestPrice();
}

std::int64_t PriceLevelTreap::quantityWithinTicks(
    const int ticks
) const {
    if (ticks < 0) {
        throw std::invalid_argument(
            "tick distance cannot be negative"
        );
    }

    const std::optional<int> best = bestPrice();

    if (!best.has_value()) {
        return 0;
    }

    if (side_ == Side::Buy) {
        const std::int64_t lowest =
            static_cast<std::int64_t>(*best) -
            ticks;

        const int lowestPriceInRange =
            static_cast<int>(
                std::max<std::int64_t>(
                    lowest,
                    std::numeric_limits<int>::min()
                )
            );

        return quantityBetween(
            lowestPriceInRange,
            *best
        );
    }

    const std::int64_t highest =
        static_cast<std::int64_t>(*best) +
        ticks;

    const int highestPriceInRange =
        static_cast<int>(
            std::min<std::int64_t>(
                highest,
                std::numeric_limits<int>::max()
            )
        );

    return quantityBetween(
        *best,
        highestPriceInRange
    );
}

std::int64_t PriceLevelTreap::quantityWithinBestLevels(
    const std::size_t numberOfLevels
) const noexcept {
    return quantityWithinBestLevels(
        root_,
        numberOfLevels
    );
}

std::int64_t PriceLevelTreap::quantityWithinBestLevels(
    const int nodeIndex,
    const std::size_t numberOfLevels
) const noexcept {
    if (
        nodeIndex == nullIndex ||
        numberOfLevels == 0
    ) {
        return 0;
    }

    const Node &node =
        nodes_[static_cast<std::size_t>(nodeIndex)];

    const int betterChild =
        side_ == Side::Buy
            ? node.right
            : node.left;

    const int worseChild =
        side_ == Side::Buy
            ? node.left
            : node.right;

    const std::size_t betterLevels =
        subtreeLevels(betterChild);

    if (numberOfLevels <= betterLevels) {
        return quantityWithinBestLevels(
            betterChild,
            numberOfLevels
        );
    }

    std::int64_t quantity =
        subtreeQuantity(betterChild) +
        node.quantity;

    if (numberOfLevels == betterLevels + 1) {
        return quantity;
    }

    quantity += quantityWithinBestLevels(
        worseChild,
        numberOfLevels - betterLevels - 1
    );

    return quantity;
}

std::optional<int> PriceLevelTreap::priceAtDepth(
    const std::int64_t cumulativeQuantity
) const noexcept {
    if (
        cumulativeQuantity <= 0 ||
        cumulativeQuantity > totalQuantity()
    ) {
        return std::nullopt;
    }

    return priceAtDepth(
        root_,
        cumulativeQuantity
    );
}

std::optional<int> PriceLevelTreap::priceAtDepth(
    const int nodeIndex,
    const std::int64_t cumulativeQuantity
) const noexcept {
    if (nodeIndex == nullIndex) {
        return std::nullopt;
    }

    const Node &node =
        nodes_[static_cast<std::size_t>(nodeIndex)];

    const int betterChild =
        side_ == Side::Buy
            ? node.right
            : node.left;

    const int worseChild =
        side_ == Side::Buy
            ? node.left
            : node.right;

    const std::int64_t betterQuantity =
        subtreeQuantity(betterChild);

    if (cumulativeQuantity <= betterQuantity) {
        return priceAtDepth(
            betterChild,
            cumulativeQuantity
        );
    }

    if (
        cumulativeQuantity <=
        betterQuantity + node.quantity
    ) {
        return node.price;
    }

    return priceAtDepth(
        worseChild,
        cumulativeQuantity -
            betterQuantity -
            node.quantity
    );
}

std::vector<PriceLevelTreap::Level>
PriceLevelTreap::topLevels(
    const std::size_t maximumLevels
) const {
    std::vector<Level> result;

    result.reserve(
        std::min(
            maximumLevels,
            levelCount()
        )
    );

    collectTopLevels(
        root_,
        maximumLevels,
        result
    );

    return result;
}

void PriceLevelTreap::collectTopLevels(
    const int nodeIndex,
    const std::size_t maximumLevels,
    std::vector<Level> &result
) const {
    if (
        nodeIndex == nullIndex ||
        result.size() >= maximumLevels
    ) {
        return;
    }

    const Node &node =
        nodes_[static_cast<std::size_t>(nodeIndex)];

    const int betterChild =
        side_ == Side::Buy
            ? node.right
            : node.left;

    const int worseChild =
        side_ == Side::Buy
            ? node.left
            : node.right;

    collectTopLevels(
        betterChild,
        maximumLevels,
        result
    );

    if (result.size() < maximumLevels) {
        result.push_back(
            Level{
                node.price,
                node.quantity
            }
        );
    }

    collectTopLevels(
        worseChild,
        maximumLevels,
        result
    );
}

bool PriceLevelTreap::validate() const noexcept {
    std::int64_t calculatedQuantity = 0;
    std::size_t calculatedLevels = 0;
    std::size_t calculatedHeight = 0;

    if (!validateNode(
            root_,
            std::nullopt,
            std::nullopt,
            calculatedQuantity,
            calculatedLevels,
            calculatedHeight
        )) {

        return false;
    }

    if (
        calculatedLevels + freeIndices_.size() !=
        nodes_.size()
    ) {
        return false;
    }

    for (
        std::size_t first = 0;
        first < freeIndices_.size();
        first++
    ) {
        const int freeIndex = freeIndices_[first];

        if (
            freeIndex < 0 ||
            static_cast<std::size_t>(freeIndex) >=
                nodes_.size() ||
            freeIndex == root_
        ) {
            return false;
        }

        for (
            std::size_t second = first + 1;
            second < freeIndices_.size();
            second++
        ) {
            if (freeIndex == freeIndices_[second]) {
                return false;
            }
        }
    }

    return true;
}

std::size_t PriceLevelTreap::height() const noexcept {
    std::int64_t calculatedQuantity = 0;
    std::size_t calculatedLevels = 0;
    std::size_t calculatedHeight = 0;

    if (!validateNode(
            root_,
            std::nullopt,
            std::nullopt,
            calculatedQuantity,
            calculatedLevels,
            calculatedHeight
        )) {

        return 0;
    }

    return calculatedHeight;
}

bool PriceLevelTreap::validateNode(
    const int nodeIndex,
    const std::optional<int> lowerBound,
    const std::optional<int> upperBound,
    std::int64_t &calculatedQuantity,
    std::size_t &calculatedLevels,
    std::size_t &calculatedHeight
) const noexcept {
    if (nodeIndex == nullIndex) {
        calculatedQuantity = 0;
        calculatedLevels = 0;
        calculatedHeight = 0;

        return true;
    }

    if (
        nodeIndex < 0 ||
        static_cast<std::size_t>(nodeIndex) >=
            nodes_.size()
    ) {
        return false;
    }

    const Node &node =
        nodes_[static_cast<std::size_t>(nodeIndex)];

    if (
        node.quantity <= 0 ||
        (
            lowerBound.has_value() &&
            node.price <= *lowerBound
        ) ||
        (
            upperBound.has_value() &&
            node.price >= *upperBound
        )
    ) {
        return false;
    }

    const auto validChildIndex =
        [&](const int childIndex) {
            return
                childIndex == nullIndex ||
                (
                    childIndex >= 0 &&
                    static_cast<std::size_t>(
                        childIndex
                    ) < nodes_.size()
                );
        };

    if (
        !validChildIndex(node.left) ||
        !validChildIndex(node.right)
    ) {
        return false;
    }

    if (
        (
            node.left != nullIndex &&
            hasHigherPriority(
                node.left,
                nodeIndex
            )
        ) ||
        (
            node.right != nullIndex &&
            hasHigherPriority(
                node.right,
                nodeIndex
            )
        )
    ) {
        return false;
    }

    std::int64_t leftQuantity = 0;
    std::size_t leftLevels = 0;
    std::size_t leftHeight = 0;

    if (!validateNode(
            node.left,
            lowerBound,
            node.price,
            leftQuantity,
            leftLevels,
            leftHeight
        )) {

        return false;
    }

    std::int64_t rightQuantity = 0;
    std::size_t rightLevels = 0;
    std::size_t rightHeight = 0;

    if (!validateNode(
            node.right,
            node.price,
            upperBound,
            rightQuantity,
            rightLevels,
            rightHeight
        )) {

        return false;
    }

    calculatedQuantity =
        leftQuantity +
        node.quantity +
        rightQuantity;

    calculatedLevels =
        leftLevels +
        1 +
        rightLevels;

    calculatedHeight =
        1 +
        std::max(
            leftHeight,
            rightHeight
        );

    return
        node.subtreeQuantity ==
            calculatedQuantity &&
        node.subtreeLevels ==
            calculatedLevels;
}
