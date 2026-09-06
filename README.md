# Event Backtester

An event-driven backtesting engine written in C++17, built around a limit order book, strategy interface, portfolio accounting, configurable execution latency, trading fees, and mark-to-market PnL.

The project simulates how trading strategies interact with a market through order events rather than directly modifying positions or prices.

## Features

- Event-driven backtesting engine
- Limit order book with price-time priority
- Limit orders
- Market orders
- IOC orders
- Cancel and modify operations
- Multi-symbol market support
- Strategy interface for implementing custom strategies
- Configurable strategy latency
- Portfolio and inventory tracking
- Trading fees
- Mark-to-market equity and PnL
- Deterministic event ordering
- CMake build system
- AddressSanitizer and UndefinedBehaviorSanitizer support

## Architecture

The project is split into several independent components:

- `Book` manages the limit order book and matching engine.
- `EventQueue` processes market and strategy events in chronological order.
- `Backtest` coordinates the simulation.
- `Strategy` defines the interface used by trading strategies.
- `FairPriceStrategy` implements the current trading strategy.
- `Portfolio` tracks cash, positions, fees, and fills.
- `IO` parses historical market events from files.

A strategy does not directly modify the order book or portfolio.

Instead, the flow is:

```text
Historical Events
       |
       v
   EventQueue
       |
       v
      Book
       |
       v
 Top of Book
       |
       v
    Strategy
       |
       v
 OrderCommand
       |
       v
   EventQueue
       |
       v
 Matching Engine
       |
       v
     Fills
       |
       v
   Portfolio
       |
       v
 Equity / PnL
```

## Fair Price Strategy

The current strategy uses top-of-book order imbalance to estimate a fair price.

The order book imbalance is:

```text
imbalance =
    (bestBidQuantity - bestAskQuantity)
    /
    (bestBidQuantity + bestAskQuantity)
```

The mid price is:

```text
mid =
    (bestBid + bestAsk) / 2
```

The predicted fair price is:

```text
fairPrice =
    mid
    + bias
    + imbalanceWeight * imbalance
```

The current model parameters are:

```text
bias = -0.482072524654
imbalanceWeight = 8.509332824319
```

The strategy then calculates:

```text
buyEdge =
    fairPrice - bestAsk

sellEdge =
    bestBid - fairPrice
```

Orders are only sent when the estimated edge is large enough.

## Inventory-Aware Risk

The strategy adjusts its required edge based on the current position.

```text
requiredBuyEdge =
    baseEdge
    + position * inventoryPenalty

requiredSellEdge =
    baseEdge
    - position * inventoryPenalty
```

Current parameters:

```text
baseEdge = 6.0 ticks
inventoryPenalty = 1.0
maxPosition = 6
```

This means that as the strategy becomes more long, additional BUY orders become harder to justify.

Similarly, as the strategy becomes more short, additional SELL orders require a larger edge.

The strategy currently trades one unit at a time using marketable limit orders:

```text
BUY  1 @ bestAsk
SELL 1 @ bestBid
```

## Limit Order Book

Bid and ask price levels are stored using ordered maps.

```text
best bid -> highest bid price
best ask -> lowest ask price
```

Orders at the same price level are stored in FIFO order using linked nodes.

The engine also keeps an order ID lookup table, allowing orders to be located efficiently for cancellation and modification.

The matching engine supports:

```text
ADD
CANCEL
MODIFY
IOC
MARKET
```

## Multi-Symbol Support

The backtester stores a separate order book for each symbol:

```cpp
std::map<Symbol, Book>
```

Historical input therefore includes the symbol of every event.

The current example strategy trades only `AAPL`, but the backtesting engine itself supports multiple symbols.

## Input Format

Historical market events use the following format:

```text
time SYMBOL TYPE order_id owner_id SIDE quantity price
```

Example:

```text
0 AAPL ADD 1001 2 BUY 100 9951
0 AAPL ADD 1002 3 SELL 80 9953
1 AAPL CANCEL 1001 2 BUY 0 0
```

Supported order types:

```text
ADD
CANCEL
MODIFY
MOD
IOC
MARKET
```

Supported sides:

```text
BUY
SELL
```

## Event Ordering

Events are processed in chronological order.

When multiple events have the same timestamp, insertion order is used as a deterministic tie-breaker.

This makes repeated backtests reproducible.

Strategy orders can also be assigned a configurable latency before they become active in the market.

## Portfolio Accounting

The portfolio is updated only when strategy orders receive:

```text
OrderFilled
OrderPartialFilled
```

For a BUY:

```text
cash -= quantity * price
cash -= fee
position += quantity
```

For a SELL:

```text
cash += quantity * price
cash -= fee
position -= quantity
```

Trading fees are calculated as:

```text
fee =
    feeTicks * tradedQuantity
```

## Mark-to-Market PnL

Cash alone is not enough to measure performance when an open position remains.

The backtester therefore calculates:

```text
equity =
    cash
    + position * mark
```

The current mark is the mid price:

```text
mark =
    (bestBid + bestAsk) / 2
```

For multiple symbols, the value of each position is added to the portfolio equity.

## Lesson 7 Validation

The current implementation was validated using an adapted Lesson 7 simulation dataset.

Configuration:

```text
symbol = AAPL
strategy latency = 0
fee = 0.05 ticks per executed unit
max position = 6
```

Result:

```text
strategy fills = 18
final position = 0
gross PnL = 300.00 ticks
trading fees = 0.90 ticks
final PnL = 299.10 ticks
```

This reproduces the reference result for the strategy while running through the project's own event queue, matching engine, portfolio, and strategy architecture.

## Project Structure

```text
Event-backtester/
├── CMakeLists.txt
├── README.md
├── main.cpp
│
├── data/
│   └── lesson07_simulation_events_adapted.txt
│
├── include/
│   └── backtester/
│       ├── backtest.hpp
│       ├── book.hpp
│       ├── event_queue.hpp
│       ├── fair_price_strategy.hpp
│       ├── io.hpp
│       ├── portfolio.hpp
│       ├── strategy.hpp
│       └── types.hpp
│
└── src/
    ├── backtest.cpp
    ├── book.cpp
    ├── fair_price_strategy.cpp
    ├── io.cpp
    └── portfolio.cpp
```

## Building

Requirements:

- CMake 3.20+
- C++17 compatible compiler

Configure the project:

```bash
cmake -S . -B build
```

Build:

```bash
cmake --build build
```

Run:

```bash
./build/event_backtester
```

## Sanitizers

The project optionally supports AddressSanitizer and UndefinedBehaviorSanitizer when using GCC or Clang.

Configure:

```bash
cmake -S . -B build-asan \
    -DCMAKE_BUILD_TYPE=Debug \
    -DENABLE_SANITIZERS=ON
```

Build:

```bash
cmake --build build-asan
```

Run:

```bash
./build-asan/event_backtester
```

## Compiler Warnings

For GCC and Clang, the project enables:

```text
-Wall
-Wextra
-Wpedantic
-Wshadow
```

## Current Limitations

The project is still under development.

Some areas planned for future improvement include:

- automated unit and integration tests
- pending-order-aware risk limits
- cached quantity at each order book level
- richer performance statistics
- trade history and equity curve output
- additional strategy implementations
- more realistic latency and execution models
- improved configuration handling

## Purpose

This project is being developed as an educational and experimental implementation of an event-driven trading backtester and limit order book in modern C++.

The goal is to better understand market microstructure, order matching, execution, strategy design, inventory risk, and backtesting architecture.
