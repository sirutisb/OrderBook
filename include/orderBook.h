#pragma once
#include <map>
#include <unordered_map>
#include <list>
#include <vector>
#include <stdexcept>
#include <format>
#include <optional>

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
    Order(OrderId id, Side side, OrderType type, Price price, Quantity qty, TimeInForce tif)
        : id(id)
        , side(side)
        , type(type)
        , price(price)
        , quantity(qty)
        , filledQuantity(0)
        , tif(tif)
    {
    }

    void fill(Quantity amount) { filledQuantity += amount; }
    bool isFilled() const { return filledQuantity == quantity; }
    Quantity getRemainingQuantity() const { return quantity - filledQuantity; }

    OrderId id;
    Side side;
    OrderType type;
    Price price;
    Quantity quantity;
    Quantity filledQuantity;
    TimeInForce tif;

};

struct Trade {
    OrderId buyOrderId;
    OrderId sellOrderId;
    Price price;
    Quantity quantity;
};

using Orders = std::list<Order>;

struct OrderEntry {
    //Order order_;
    Orders::iterator location_;
};

struct PriceLevel {
    PriceLevel() : totalVolume_(0) {}

    void addOrder(const Order& order) {
        orders_.push_back(order);
        totalVolume_ += order.getRemainingQuantity();
    }

    void removeOrder(const Orders::iterator& location) {
        totalVolume_ -= location->getRemainingQuantity();
        orders_.erase(location);
    }

    void removeFrontOrder() {
        totalVolume_ -= orders_.front().getRemainingQuantity();
        orders_.pop_front();
    }

    Quantity getTotalVolume() const { return totalVolume_; }

    bool isEmpty() const { return orders_.empty(); }

    //Price price_;
    Quantity totalVolume_;
    Orders orders_;
};

struct OrderModify {
    OrderId id_;
    Price price_;
    Quantity quantity_;

    Order toOrder(Side side, OrderType type, TimeInForce tif) const {
        return Order(id_, side, type, price_, quantity_, tif);
    }
};

class OrderBook {
public:
    OrderBook()
    {
    }

    ~OrderBook() = default;

    OrderBook(const OrderBook&) = delete;
    OrderBook& operator=(const OrderBook&) = delete;

    std::vector<Trade> addOrder(Order& order) {
        std::vector<Trade> trades;

        if (order.type == OrderType::LIMIT) {
            if (order.tif == TimeInForce::FOK && !canFullyMatch(order)) {
                return trades;
            }

            trades = matchLimitOrder(order);

            if (!order.isFilled() && order.tif == TimeInForce::GTC) {
                PriceLevel& level = (order.side == Side::BUY) ? bids_[order.price] : asks_[order.price];
                level.addOrder(order);
                orderLookup_.emplace(order.id, OrderEntry{ std::prev(level.orders_.end()) });
            }
        } else {
            trades = matchMarketOrder(order);
        }

        return trades;
    }

    bool cancelOrder(OrderId orderId) {
        auto orderIt = orderLookup_.find(orderId);
        if (orderIt == orderLookup_.end()) return false;

        const auto& entry = orderIt->second;
        const Order& order = *entry.location_;

        auto removeFromBook = [&](auto& book) {
            auto levelIt = book.find(order.price);
            if (levelIt == book.end()) throw std::logic_error("no level in book found!");
            PriceLevel& level = levelIt->second;
            level.removeOrder(entry.location_);
            if (level.isEmpty()) book.erase(levelIt);
        };

        if (order.side == Side::BUY) {
            removeFromBook(bids_);
        } else {
            removeFromBook(asks_);
        }

        orderLookup_.erase(orderIt);
        return true;
    }

    std::vector<Trade> modifyOrder(const OrderModify& order) {
        auto it = orderLookup_.find(order.id_);
        if (it == orderLookup_.end()) throw std::logic_error("order doesnt exist");

        const auto& standingOrder = *it->second.location_;
        Order updatedOrder = order.toOrder(standingOrder.side, standingOrder.type, standingOrder.tif);
        cancelOrder(order.id_);
        return addOrder(updatedOrder);
    }

    // Query methods - (Public interface)
    std::optional<Price> getBestBid() const {
        if (bids_.empty()) return std::nullopt;
        return bids_.begin()->first;
    }

    std::optional<Price> getBestAsk() const {
        if (asks_.empty()) return std::nullopt;
        return asks_.begin()->first;
    }

    std::optional<Price> getSpread() const {
        auto bid = getBestBid();
        auto ask = getBestAsk();
        if (!bid || !ask) return std::nullopt;

        return *ask - *bid;
    }

    Quantity getVolumeAtPrice(Price price, Side side) const {
        auto getSideVolume = [&](auto& orderSide) -> Quantity {
            auto it = orderSide.find(price);
            return (it == orderSide.end()) ? 0 : it->second.getTotalVolume();
        };
        return side == Side::BUY ? getSideVolume(bids_) : getSideVolume(asks_);
    }

    // Market data
    struct BookLevel {
        Price price;
        Quantity volume;
        BookLevel(Price p, Quantity v) : price(p), volume(v) {}
    };

    std::vector<BookLevel> getBidDepth(size_t levels) const { return getDepthFrom(bids_, levels); }
    std::vector<BookLevel> getAskDepth(size_t levels) const { return getDepthFrom(asks_, levels); }

    // Statistics
    size_t getOrderCount() const { return orderLookup_.size(); }
    bool isEmpty() const { return orderLookup_.empty(); }

private:
    std::map<Price, PriceLevel, std::greater<Price>> bids_;
    std::map<Price, PriceLevel> asks_;
    std::unordered_map<OrderId, OrderEntry> orderLookup_;


    template <typename BookMap>
    std::vector<BookLevel> getDepthFrom(const BookMap& book, size_t levels) const {
        std::vector<BookLevel> depth;
        depth.reserve(std::min(levels, book.size()));
        for (auto it = book.begin(); levels > 0 && it != book.end(); --levels, ++it) {
            depth.emplace_back(it->first, it->second.getTotalVolume());
        }
        return depth;
    }

    bool canFullyMatch(const Order& order) const {
        Quantity requiredQty = order.getRemainingQuantity();
        Quantity availableQty = 0;

        if (order.side == Side::BUY) {
            // Match against Asks
            for (const auto& [price, level] : asks_) {
                if (price > order.price) break;
                availableQty += level.getTotalVolume();
                if (availableQty >= requiredQty) return true;
            }
        } else {
            // Match against Bids
            for (const auto& [price, level] : bids_) {
                if (price < order.price) break;
                availableQty += level.getTotalVolume();
                if (availableQty >= requiredQty) return true;
            }
        }
        return false;
    }

    // Matching engine
    std::vector<Trade> matchOrder(Order& order) {
        return order.type == OrderType::LIMIT
            ? matchLimitOrder(order)
            : matchMarketOrder(order);
    }

    template <typename BookType, typename Predicate>
    std::vector<Trade> executeMatching(Order& order, BookType& book, Predicate&& shouldMatchPrice) {
        std::vector<Trade> trades;

        while (!order.isFilled() && !book.empty()) {
            auto bestIt = book.begin();
            Price bestPrice = bestIt->first;

            if (!shouldMatchPrice(bestPrice)) break;

            PriceLevel& level = bestIt->second;

            // Match against all orders at this price level
            while (!level.orders_.empty() && !order.isFilled()) {
                Order& standingOrder = level.orders_.front();
                Quantity fillQty = std::min(order.getRemainingQuantity(), standingOrder.getRemainingQuantity());

                order.fill(fillQty);
                standingOrder.fill(fillQty);

                // Todo: method to reduce volume
                // remove will not reduce volume and will need to call reduce first in PriceLevel class.
                level.totalVolume_ -= fillQty;

                trades.push_back(Trade{
                    order.id,
                    standingOrder.id,
                    bestPrice,
                    fillQty
                });

                if (standingOrder.isFilled()) {
                    orderLookup_.erase(standingOrder.id);
                    level.removeFrontOrder();
                }
            }

            if (level.orders_.empty()) {
                book.erase(bestIt);
            }
        }

        return trades;
    }

    std::vector<Trade> matchLimitOrder(Order& order) {
        if (order.side == Side::BUY) {
            return executeMatching(order, asks_, [limit = order.price](Price ask) { return ask <= limit; });
        } else {
            return executeMatching(order, bids_, [limit = order.price](Price bid) { return bid >= limit; });
        }
    }

    std::vector<Trade> matchMarketOrder(Order& order) {
        if (order.side == Side::BUY) {
            return executeMatching(order, asks_, [](Price) { return true; });
        } else {
            return executeMatching(order, bids_, [](Price) { return true; });
        }
    }
};