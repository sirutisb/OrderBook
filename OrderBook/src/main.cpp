#include <iostream>
#include <vector>

#include "orderBook.h"

#include <thread>
#include <random>
#include <chrono>
#include <memory>

void printOrderBook(const OrderBook& orderBook, size_t levels = 10, size_t barWidth = 50) {
    auto asks = orderBook.getAskDepth(levels);
    auto bids = orderBook.getBidDepth(levels);

    std::string buffer;

    //std::cout << "\033[?25l"; // hide cursor
    //buffer += "\033[H"; // Move to top-left



    Quantity maxVolume = 0;
    for (const auto& level : asks) {
        maxVolume = std::max(maxVolume, level.volume);
    }
    for (const auto& level : bids) {
        maxVolume = std::max(maxVolume, level.volume);
    }

    if (maxVolume == 0) {
        std::cout << "Order book is empty\n";
        return;
    }

    buffer += "\n" + std::string(80, '=') + "\n";
    buffer += "ORDER BOOK DEPTH\n";
    buffer += std::string(80, '=') + "\n";

    buffer += "\nASKS (Sell Orders):\n";
    buffer += std::string(80, '-') + "\n";

    for (auto it = asks.rbegin(); it != asks.rend(); ++it) {
        const auto& level = *it;
        int barLength = static_cast<int>((level.volume * barWidth) / maxVolume);

        //std::cout << std::fixed << std::setprecision(2);
        buffer +=
            std::to_string(level.price) + " | "
            + std::to_string(level.volume) + " | "
            + std::string(barLength, '#') + "\n";
            //+ std::string(barLength, '█') + "\n";


    }

    // Handle optional return values
    auto bestAsk = orderBook.getBestAsk();
    auto bestBid = orderBook.getBestBid();
    auto spread = orderBook.getSpread();

    if (bestAsk && bestBid && spread) {
        Price midPrice = (*bestAsk + *bestBid) / 2;

        buffer += std::string(80, '-') + "\n";
        buffer += "SPREAD: " + std::to_string(*spread)
            + " | MID: " + std::to_string(midPrice) + "\n";
        buffer += std::string(80, '-') + "\n";
    }

    buffer+= "\nBIDS (Buy Orders):\n";
    buffer+= std::string(80, '-') + "\n";

    for (const auto& level : bids) {
        int barLength = static_cast<int>((level.volume * barWidth) / maxVolume);

        buffer
            += std::to_string(level.price) + " | "
            += std::to_string(level.volume) + " | "
            += std::string(barLength, '#') + "\n";
    }

    buffer += std::string(80, '=') + "\n";
    buffer += "Total Orders: " + std::to_string(orderBook.getOrderCount()) + "\n";
    buffer += std::string(80, '=') + "\n\n";


    system("cls");
    std::cout << buffer << std::flush;
}





#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <chrono>

void fastPrint() {
    // ANSI Escape Codes:
    // \033[?25l  -> Hide Cursor (makes it look smoother)
    // \033[H     -> Move Cursor to Home (0,0)
    std::cout << "\033[?25l";

    while (true) {
        // 1. Create a "buffer" in memory
        std::string buffer;

        // 2. Build your frame
        buffer += "\033[H"; // Move to top-left
        buffer += "--- ORDER BOOK ---\n";
        buffer += "ASKS:\n";
        buffer += "105.50  |  500\n";
        buffer += "105.00  |  200\n";
        buffer += "------------------\n";
        buffer += "BIDS:\n";
        buffer += "104.50  |  " + std::to_string(rand() % 1000) + "\n"; // Simulate update
        buffer += "104.00  |  300\n";

        // 3. Print the whole frame at once
        std::cout << buffer << std::flush;

        // Simulate some latency
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

int main() {
    // 1. Setup OrderBook and State
    OrderBook orderBook;
    std::vector<OrderId> activeOrderIds;
    OrderId nextOrderId = 1;

    // 2. Setup Randomness
    std::mt19937_64 rng(std::random_device{}());

    // Config
    Price centerPrice = 10000;
    Price spreadHalf = 50;

    // Distributions
    std::uniform_int_distribution<Price> priceDistBuy(centerPrice - spreadHalf - 100, centerPrice - spreadHalf);
    std::uniform_int_distribution<Price> priceDistSell(centerPrice + spreadHalf, centerPrice + spreadHalf + 100);
    std::uniform_int_distribution<Quantity> quantityDist(1, 100);
    std::uniform_int_distribution<int> sideDist(0, 1);
    std::uniform_int_distribution<int> actionDist(0, 99);

    // 3. Define Actions as Lambdas (Inline functions)

    auto addOrder = [&]() {
        Side side = (sideDist(rng) == 0) ? Side::BUY : Side::SELL;
        // 90% Limit, 10% Market
        OrderType type = (std::uniform_int_distribution<int>(0, 99)(rng) < 90) ? OrderType::LIMIT : OrderType::MARKET;
        TimeInForce tif = TimeInForce::GTC; // Keep it simple for demo

        Price price = (type == OrderType::MARKET) ? 0 : ((side == Side::BUY) ? priceDistBuy(rng) : priceDistSell(rng));
        Quantity qty = quantityDist(rng);
        OrderId id = nextOrderId++;

        Order order(id, side, type, price, qty, tif);

        // EXECUTE
        auto trades = orderBook.addOrder(order);

        // TRACKING (Only if GTC and not filled)
        if (type == OrderType::LIMIT && tif == TimeInForce::GTC && !order.isFilled()) {
            activeOrderIds.push_back(id);
        }

        std::cout << "[ADD] ID:" << id << " " << (side == Side::BUY ? "BUY" : "SELL")
            << " @" << price << " Q:" << qty << " -> Trades: " << trades.size() << "\n";
        };

    auto cancelOrder = [&]() {
        if (activeOrderIds.empty()) return;

        // Pick random index
        std::uniform_int_distribution<size_t> idxDist(0, activeOrderIds.size() - 1);
        size_t idx = idxDist(rng);
        OrderId id = activeOrderIds[idx];

        // EXECUTE
        bool success = orderBook.cancelOrder(id);

        // Remove from tracking
        activeOrderIds.erase(activeOrderIds.begin() + idx);

        if (success) std::cout << "[CANCEL] ID:" << id << " Success\n";
        };

    auto modifyOrder = [&]() {
        if (activeOrderIds.empty()) return;

        size_t idx = std::uniform_int_distribution<size_t>(0, activeOrderIds.size() - 1)(rng);
        OrderId id = activeOrderIds[idx];

        Quantity newQty = quantityDist(rng);
        Price newPrice = (sideDist(rng) == 0) ? priceDistBuy(rng) : priceDistSell(rng);

        OrderModify mod{ id, newPrice, newQty };

        try {
            // EXECUTE
            auto trades = orderBook.modifyOrder(mod);
            std::cout << "[MODIFY] ID:" << id << " NewPrice:" << newPrice << " NewQty:" << newQty << "\n";

            // If filled, remove from active list
            if (!trades.empty()) {
                activeOrderIds.erase(activeOrderIds.begin() + idx);
            }
        } catch (...) {
            // Order likely already filled or gone
            activeOrderIds.erase(activeOrderIds.begin() + idx);
        }
        };


    // 4. Main Event Loop
    std::cout << "Starting Simulation (Press Ctrl+C to stop)...\n";

    for (int i = 0;; ++i) {
        int action = actionDist(rng);

        // Simulation Step (The "Writer")
        if (action < 60) addOrder();       // 60% Add
        else if (action < 80) cancelOrder(); // 20% Cancel
        else modifyOrder();                // 20% Modify

        printOrderBook(orderBook);
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    std::cout << "Simulation Complete.\n";
    return 0;
}