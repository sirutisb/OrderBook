#pragma once
#include <thread>
#include <atomic>
#include <random>
#include <chrono>
#include <memory>
#include <vector>
#include <mutex>

using OrderPointer = std::shared_ptr<Order>;

class MarketSimulator {
private:
    OrderBook& orderBook;
    std::atomic<bool> running;
    std::thread workerThread;
    std::mutex mtx;

    // Random number generators
    std::mt19937_64 rng;
    std::uniform_int_distribution<Price> priceDistBuy;
    std::uniform_int_distribution<Price> priceDistSell;
    std::uniform_int_distribution<Quantity> quantityDist;
    std::uniform_int_distribution<int> sideDist;
    std::uniform_int_distribution<int> orderTypeDist;
    std::uniform_int_distribution<int> tifDist;
    std::uniform_int_distribution<int> actionDist;
    std::uniform_int_distribution<int> delayDist;

    OrderId nextOrderId;
    std::vector<OrderId> activeOrderIds;

    Price centerPrice;
    Price spreadHalf;

public:
    MarketSimulator(OrderBook& book, Price centerPrice = 10000, Price spreadHalf = 50)
        : orderBook(book)
        , running(false)
        , rng(std::random_device{}())
        , priceDistBuy(centerPrice - spreadHalf - 100, centerPrice - spreadHalf)
        , priceDistSell(centerPrice + spreadHalf, centerPrice + spreadHalf + 100)
        , quantityDist(1, 1000)
        , sideDist(0, 1)
        , orderTypeDist(0, 99)  // 90% limit, 10% market
        , tifDist(0, 99)  // 80% GTC, 15% IOC, 5% FOK
        , actionDist(0, 99)  // 70% add, 20% cancel, 10% modify
        , delayDist(10, 100)  // 10-100ms between orders
        , nextOrderId(1)
        , centerPrice(centerPrice)
        , spreadHalf(spreadHalf) {
    }

    ~MarketSimulator() {
        stop();
    }

    void start() {
        if (running.exchange(true)) {
            return;  // Already running
        }

        workerThread = std::thread(&MarketSimulator::simulationLoop, this);
    }

    void stop() {
        if (!running.exchange(false)) {
            return;  // Already stopped
        }

        if (workerThread.joinable()) {
            workerThread.join();
        }
    }

    bool isRunning() const {
        return running.load();
    }

private:
    void simulationLoop() {
        while (running.load()) {
            int action = actionDist(rng);

            if (action < 70 || activeOrderIds.empty()) {
                // 70% add new order (or 100% if no orders exist)
                addRandomOrder();
            } else if (action < 90) {
                // 20% cancel random order
                cancelRandomOrder();
            } else {
                // 10% modify random order
                modifyRandomOrder();
            }

            // Random delay between actions
            int delayMs = delayDist(rng);
            std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));
        }
    }

    void addRandomOrder() {
        Side side = (sideDist(rng) == 0) ? Side::BUY : Side::SELL;

        // Determine order type (90% limit, 10% market)
        OrderType type = (orderTypeDist(rng) < 90) ? OrderType::LIMIT : OrderType::MARKET;

        // Determine time in force (80% GTC, 15% IOC, 5% FOK)
        TimeInForce tif;
        int tifRoll = tifDist(rng);
        if (tifRoll < 80) {
            tif = TimeInForce::GTC;
        } else if (tifRoll < 95) {
            tif = TimeInForce::IOC;
        } else {
            tif = TimeInForce::FOK;
        }

        // Generate price based on side and type
        Price price;
        if (type == OrderType::MARKET) {
            price = 0;  // Market orders don't use price for matching
        } else {
            price = (side == Side::BUY) ? priceDistBuy(rng) : priceDistSell(rng);
        }

        Quantity quantity = quantityDist(rng);

        OrderId orderId;
        {
            std::lock_guard<std::mutex> lock(mtx);
            orderId = nextOrderId++;
        }

        Order order(
            orderId, side, type, price, quantity, tif
        );

        // Add to order book
        auto trades = orderBook.addOrder(order);

        // Track GTC limit orders that weren't fully filled
        if (type == OrderType::LIMIT &&
            tif == TimeInForce::GTC &&
            !order.isFilled()) {
            std::lock_guard<std::mutex> lock(mtx);
            activeOrderIds.push_back(orderId);
        }

        // Log the action
        logOrder("ADD", order, trades.size());
    }

    void cancelRandomOrder() {
        OrderId orderId;
        {
            std::lock_guard<std::mutex> lock(mtx);
            if (activeOrderIds.empty()) return;

            std::uniform_int_distribution<size_t> indexDist(0, activeOrderIds.size() - 1);
            size_t index = indexDist(rng);
            orderId = activeOrderIds[index];
            activeOrderIds.erase(activeOrderIds.begin() + index);
        }

        bool success = orderBook.cancelOrder(orderId);
        logAction("CANCEL", orderId, success);
    }

    void modifyRandomOrder() {
        OrderId orderId;
        {
            std::lock_guard<std::mutex> lock(mtx);
            if (activeOrderIds.empty()) return;

            std::uniform_int_distribution<size_t> indexDist(0, activeOrderIds.size() - 1);
            size_t index = indexDist(rng);
            orderId = activeOrderIds[index];
            // Don't remove from activeOrderIds yet - modify might fail
        }

        Quantity newQuantity = quantityDist(rng);

        // Create OrderModify struct (need to get price from existing order)
        // Since we can't easily get the price, we'll use a simpler approach
        // Just generate a new price for the modification
        Price newPrice = (sideDist(rng) == 0) ? priceDistBuy(rng) : priceDistSell(rng);

        OrderModify modify{ orderId, newPrice, newQuantity };

        try {
            auto trades = orderBook.modifyOrder(modify);
            logModify(orderId, newQuantity, true, trades.size());

            // If modify succeeded, the old order was cancelled and new one added
            // Track the new order if it wasn't filled
            if (trades.empty() || newQuantity > 0) {
                // Order still exists in book
                // Keep it in activeOrderIds (orderId stays the same)
            } else {
                // Order was fully filled, remove from tracking
                std::lock_guard<std::mutex> lock(mtx);
                auto it = std::find(activeOrderIds.begin(), activeOrderIds.end(), orderId);
                if (it != activeOrderIds.end()) {
                    activeOrderIds.erase(it);
                }
            }
        } catch (const std::exception& e) {
            logModify(orderId, newQuantity, false, 0);
            // Remove from tracking since modify failed (order doesn't exist)
            std::lock_guard<std::mutex> lock(mtx);
            auto it = std::find(activeOrderIds.begin(), activeOrderIds.end(), orderId);
            if (it != activeOrderIds.end()) {
                activeOrderIds.erase(it);
            }
        }
    }

    // Logging helpers
    void logOrder(const std::string& action, const Order& order, size_t tradeCount) {
        return;

        std::cout << "[" << action << "] "
            << "ID: " << order.id
            << " | " << (order.side == Side::BUY ? "BUY " : "SELL")
            << " | " << (order.type == OrderType::LIMIT ? "LIMIT " : "MARKET")
            << " | Price: " << order.price
            << " | Qty: " << order.quantity;

        if (tradeCount > 0) {
            std::cout << " | Trades: " << tradeCount;
        }

        std::cout << std::endl;
    }

    void logAction(const std::string& action, OrderId orderId, bool success) {
        return;

        std::cout << "[" << action << "] "
            << "ID: " << orderId
            << " | " << (success ? "SUCCESS" : "FAILED")
            << std::endl;
    }

    void logModify(OrderId orderId, Quantity newQty, bool success, size_t tradeCount) {
        return;

        std::cout << "[MODIFY] "
            << "ID: " << orderId
            << " | New Qty: " << newQty
            << " | " << (success ? "SUCCESS" : "FAILED");

        if (success && tradeCount > 0) {
            std::cout << " | Trades: " << tradeCount;
        }

        std::cout << std::endl;
    }
};

// Example usage:
/*
int main() {
    OrderBook orderBook;
    MarketSimulator simulator(orderBook, 10000, 50);

    simulator.start();

    // Let it run and print periodically
    for (int i = 0; i < 10; ++i) {
        std::this_thread::sleep_for(std::chrono::seconds(2));
        orderBook.printOrderBook(10, 40);
    }

    simulator.stop();

    return 0;
}
*/