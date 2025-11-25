#pragma once
#include <map>
#include <unordered_map>
#include <list>
#include <vector>
#include <cstdint>
#include <memory>

// Type aliases
using Price = int64_t;
using Quantity = uint64_t;
using OrderId = uint64_t;

// Enums
enum class Side { BUY, SELL };
enum class OrderType { LIMIT, MARKET };
enum class TimeInForce { GTC, IOC, FOK };  // Good-Till-Cancel, Immediate-or-Cancel, Fill-or-Kill

// Structs
struct Order {
    OrderId id;
    Side side;
    OrderType type;
    Price price;
    Quantity quantity;
    Quantity filledQuantity;
    TimeInForce tif;
    uint64_t timestamp;

    Order(OrderId id, Side side, OrderType type, Price price, Quantity qty, TimeInForce tif);
    
    void fill(Quantity amount);
    bool isFilled() const;
    Quantity getRemainingQuantity() const;
};

struct Trade {
    OrderId buyOrderId;
    OrderId sellOrderId;
    Price price;
    Quantity quantity;
    uint64_t timestamp;
};

// Price level containing orders at same price
class PriceLevel {
public:
    PriceLevel(Price price);

    void addOrder(const std::shared_ptr<Order>& order);
    void removeOrder(OrderId orderId);
    Quantity getTotalVolume() const;
    bool isEmpty() const;
    std::list<std::shared_ptr<Order>>& getOrders();

private:
    Price price_;
    Quantity totalVolume_;
    std::list<std::shared_ptr<Order>> orders_;
};

// Main OrderBook class
class OrderBook {
public:
    OrderBook();
    ~OrderBook();

    // Order management
    std::vector<Trade> addOrder(const Order& order);
    bool cancelOrder(OrderId orderId);
    bool modifyOrder(OrderId orderId, Quantity newQuantity);

    // Query methods
    Price getBestBid() const;
    Price getBestAsk() const;
    Price getSpread() const;
    Quantity getVolumeAtPrice(Price price, Side side) const;

    // Market data
    struct BookLevel {
        Price price;
        Quantity volume;
    };

    std::vector<BookLevel> getBidDepth(size_t levels) const;
    std::vector<BookLevel> getAskDepth(size_t levels) const;

    // Statistics
    size_t getOrderCount() const;
    bool isEmpty() const;

private:
    // Price level maps (bid: descending, ask: ascending)
    std::map<Price, PriceLevel, std::greater<Price>> bids_;  // Descending
    std::map<Price, PriceLevel> asks_;                        // Ascending

    // Fast order lookup for cancellations
    std::unordered_map<OrderId, std::shared_ptr<Order>> orderLookup_;

    // Matching engine
    std::vector<Trade> matchOrder(const std::shared_ptr<Order>& order);
    std::vector<Trade> matchLimitOrder(const std::shared_ptr<Order>& order);
    std::vector<Trade> matchMarketOrder(const std::shared_ptr<Order>& order);

    // Helper methods
    void insertOrder(const std::shared_ptr<Order>& order);
    void removeOrderFromBook(const std::shared_ptr<Order>& order);
    Trade executeTrade(const std::shared_ptr<Order>& buyOrder,
        const std::shared_ptr<Order>& sellOrder,
        Quantity quantity);

    uint64_t nextOrderId_;
    uint64_t nextTradeId_;
};