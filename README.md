# OrderBook Matching Engine

A lightweight C++ console limit order book implementation. It continuously generates random buy/sell orders, matches them against standing liquidity, and renders the current depth in the terminal so you can watch spreads, mid-prices, and volumes evolve in real time.

## Preview
<img width="746" height="689" alt="image" src="https://github.com/user-attachments/assets/daddc8a6-ce3a-4f3e-8baf-aadc94ad9b78" />


## Features
- Limit, market, and basic time-in-force handling (GTC/IOC/FOK) backed by a price-level order book via price-time priority.
- Order management operations: add, cancel, and modify existing orders.
- Simple depth visualization showing top-of-book prices, spread, mid-price, and relative volume bars.

## Complexity
The core engine utilizes a std::map for price levels and a std::unordered_map for order lookups.

| Operation | Complexity | Description |
| :--- | :--- | :--- |
| **Place Order** | $O(\log M)$ | Searching/Inserting the price level. |
| **Cancel Order** | $O(\log M)$ | Tree search to locate and remove liquidity. |
| **Match** | $O(T)$ | Linear to the number of trades ($T$) generated. |
| **Get Best Bid/Ask**| $O(1)$ | Direct access to the map's begin iterator. |

*(Where M is the number of active price levels)*

## Build
The code requires a C++20 compiler:
```bash
mkdir build
cd build
cmake ..
make
./bin/prog
```
