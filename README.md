# Event Backtester

An event-driven backtesting engine written in C++17.

The project simulates a limit order book, processes historical market events, executes strategy-generated orders through the same matching engine, and tracks portfolio positions and PnL.

## Quick Start

### Requirements

- CMake 3.20+
- A C++17-compatible compiler such as GCC or Clang

### Clone

```bash
git clone https://github.com/Octa-pe-ghituri/Event-backtester.git
cd Event-backtester
```

### Build

```bash
cmake -S . -B build
cmake --build build
```

### Run

From the project root:

```bash
./build/event_backtester
```

The current example runs the included fair-price strategy on the provided simulation dataset.

---

## What It Supports

- Event-driven simulation
- Limit order book
- Price-time priority / FIFO within a price level
- Limit orders
- Market orders
- IOC orders
- Cancel and modify operations
- Multiple symbols
- Custom strategy interface
- Configurable strategy latency
- Portfolio and inventory tracking
- Trading fees
- Mark-to-market PnL
- Deterministic event ordering

## How It Works

The main flow is:

```text
Historical Events
       |
       v
   Event Queue
       |
       v
   Order Book
       |
       v
    Strategy
       |
       v
  Order Command
       |
       v
 Matching Engine
       |
       v
     Fills
       |
       v
   Portfolio
```

The strategy only observes the current market and portfolio state.

It does not directly modify the order book or position. Instead, it returns an `OrderCommand`, which is processed by the backtesting engine.

## Input Data

Historical events use the following format:

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

The fields are:

```text
time       simulation timestamp
SYMBOL     instrument identifier
TYPE       order event type
order_id   order identifier
owner_id   participant identifier
SIDE       BUY or SELL
quantity   order quantity
price      integer price in ticks
```

## Running Another Dataset

The dataset used by the backtest is currently configured in `main.cpp`.

Example:

```cpp
Backtest backtest(
    "data/lesson07_simulation_events_adapted.txt",
    std::move(strategy),
    359,
    0,
    1,
    0.05
);
```

The arguments are:

```text
events file
strategy
maximum simulation time
strategy latency
strategy owner ID
fee per executed unit
```

To use another dataset, change the file path and simulation parameters in `main.cpp`, then rebuild:

```bash
cmake --build build
./build/event_backtester
```

## Fair Price Strategy

The included strategy estimates fair value using top-of-book imbalance.

```text
imbalance =
    (bidQuantity - askQuantity)
    /
    (bidQuantity + askQuantity)

mid =
    (bestBid + bestAsk) / 2

fairPrice =
    mid + bias + imbalanceWeight * imbalance
```

It then compares fair value with the current best bid and ask.

```text
buyEdge  = fairPrice - bestAsk
sellEdge = bestBid - fairPrice
```

The required edge also depends on the current inventory, making it harder to keep increasing an already large position.

The current strategy trades one unit at a time using marketable limit orders.

## Validation

The included simulation scenario currently produces:

```text
strategy fills = 18
final position = 0
gross PnL = 300.00 ticks
trading fees = 0.90 ticks
final PnL = 299.10 ticks
```

Configuration:

```text
symbol = AAPL
strategy latency = 0
fee = 0.05 ticks per executed unit
max position = 6
```

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

## Adding Another Strategy

New strategies can inherit from the `Strategy` interface and implement:

```cpp
std::optional<OrderCommand> onTimeMove(
    int now,
    const std::map<Symbol, Book> &books,
    const Portfolio &portfolio,
    const std::vector<StrategyEvent> &recentEvents
);
```

The new strategy can then be instantiated in `main.cpp` and passed to `Backtest`.

## Sanitizers

The project supports AddressSanitizer and UndefinedBehaviorSanitizer with GCC or Clang.

```bash
cmake -S . -B build-asan \
    -DCMAKE_BUILD_TYPE=Debug \
    -DENABLE_SANITIZERS=ON

cmake --build build-asan
./build-asan/event_backtester
```

## Current Limitations

The project is still under development.

Planned improvements include:

- automated tests
- pending-order-aware risk limits
- richer performance statistics
- additional strategies
- improved runtime configuration
- more realistic execution and latency models

## Purpose

This project is an educational and experimental implementation of an event-driven trading backtester and limit order book.

The main goal is to better understand order matching, market microstructure, execution, inventory risk, and backtesting architecture.
