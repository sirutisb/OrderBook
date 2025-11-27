#include <iostream>
#include <vector>

#include "orderBook.h"
#include "marketSimulator.h"

void printOrderBook(const OrderBook& orderBook, size_t levels = 10, size_t barWidth = 50) {
    auto asks = orderBook.getAskDepth(levels);
    auto bids = orderBook.getBidDepth(levels);

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

    std::cout << "\n" << std::string(80, '=') << "\n";
    std::cout << "ORDER BOOK DEPTH\n";
    std::cout << std::string(80, '=') << "\n";

    std::cout << "\nASKS (Sell Orders):\n";
    std::cout << std::string(80, '-') << "\n";

    for (auto it = asks.rbegin(); it != asks.rend(); ++it) {
        const auto& level = *it;
        int barLength = static_cast<int>((level.volume * barWidth) / maxVolume);

        std::cout << std::fixed << std::setprecision(2)
            << std::setw(10) << level.price << " | "
            << std::setw(8) << level.volume << " | "
            << std::string(barLength, '█') << "\n";
    }

    // Handle optional return values
    auto bestAsk = orderBook.getBestAsk();
    auto bestBid = orderBook.getBestBid();
    auto spread = orderBook.getSpread();

    if (bestAsk && bestBid && spread) {
        Price midPrice = (*bestAsk + *bestBid) / 2;

        std::cout << std::string(80, '-') << "\n";
        std::cout << "SPREAD: " << std::fixed << std::setprecision(2) << *spread
            << " | MID: " << midPrice << "\n";
        std::cout << std::string(80, '-') << "\n";
    }

    std::cout << "\nBIDS (Buy Orders):\n";
    std::cout << std::string(80, '-') << "\n";

    for (const auto& level : bids) {
        int barLength = static_cast<int>((level.volume * barWidth) / maxVolume);

        std::cout << std::fixed << std::setprecision(2)
            << std::setw(10) << level.price << " | "
            << std::setw(8) << level.volume << " | "
            << std::string(barLength, '█') << "\n";
    }

    std::cout << std::string(80, '=') << "\n";
    std::cout << "Total Orders: " << orderBook.getOrderCount() << "\n";
    std::cout << std::string(80, '=') << "\n\n";
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



    fastPrint();




    return 0;



	OrderBook orderBook;
    MarketSimulator simulator(orderBook, 10000, 50); // center=10000, spread=50

    simulator.start();
    // ... let it run ...
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // Print order book periodically
    for (int i = 0; i < 500000; ++i) {
        system("cls");
        printOrderBook(orderBook, 10, 40);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    //orderBook.printOrderBook();
    simulator.stop();

    std::cout << "ended\n";

    return 0;
}