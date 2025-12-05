# OrderBook Simulator

OrderBook Simulator is a lightweight C++20 console program that emulates a simple electronic limit order book. It continuously generates random buy/sell orders, matches them against standing liquidity, and renders the current depth in the terminal so you can watch spreads, mid-prices, and volumes evolve in real time.

## Features
- Limit, market, and basic time-in-force handling (GTC/IOC/FOK) backed by a price-level order book.
- Order management operations: add, cancel, and modify existing orders.
- Simple depth visualization showing top-of-book prices, spread, mid-price, and relative volume bars.
- Pluggable market simulation utilities (`MarketSimulator`) for automated order flow.

## Project structure
- `src/main.cpp` – standalone console driver that creates an `OrderBook`, injects random orders, and paints the book to the terminal each tick.
- `include/orderBook.h` – core order book data structures, matching logic, and depth/statistics helpers.
- `include/marketSimulator.h` – optional threaded simulator that can generate order flow against an `OrderBook` instance.
- `OrderBook.vcxproj*` – Visual Studio project files for Windows builds.

## Build
The code requires a C++20 compiler and POSIX threads. You can compile the console app with `g++`:

```bash
g++ -std=c++20 -Iinclude src/main.cpp -lpthread -o orderbook
```

On Windows, you can open `OrderBook.vcxproj` in Visual Studio and build the provided project configuration.

## Run
After building, launch the simulator from your terminal:

```bash
./orderbook
```

The program will start streaming randomized order activity and refresh the terminal with the current book depth every ~200ms. Use `Ctrl+C` to exit.

## Configuration hints
`src/main.cpp` includes a few parameters you can tweak before compiling:
- `centerPrice` / `spreadHalf`: control the typical mid-price and starting spread used for random order generation.
- Distribution ranges for buy/sell prices, quantities, and action mix (add/cancel/modify probabilities) influence how volatile the simulated book becomes.

For more advanced flows, consider the `MarketSimulator` helper in `include/marketSimulator.h`, which runs a background thread to feed orders into an `OrderBook` instance.
