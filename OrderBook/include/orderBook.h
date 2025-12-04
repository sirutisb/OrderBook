#pragma once
#include <map>
#include <unordered_map>
#include <list>
#include <vector>
#include <cstdint>
#include <memory>
#include <ranges>     // std::views::take
#include <stdexcept>
#include <format>
#include <optional>


#include <iostream>
#include <iomanip>

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

using OrderPointer = std::shared_ptr<Order>;
using OrderPointers = std::list<OrderPointer>;

struct OrderEntry {
    OrderPointer order_{ nullptr };
    OrderPointers::iterator location_;
};

struct PriceLevel {
    //PriceLevel(Price price)
    //    : price_(price)
    //{
    //}

    PriceLevel() = default;

    void addOrder(const OrderPointer& order) {
        orders_.push_back(order);
        totalVolume_ += order->getRemainingQuantity();
    }

    void removeOrder(const OrderPointers::iterator& location) {
        totalVolume_ -= (*location)->getRemainingQuantity();
        orders_.erase(location);
    }

    void removeFrontOrder() {
        totalVolume_ -= orders_.front()->getRemainingQuantity();
        orders_.pop_front();
    }

    Quantity getTotalVolume() const { return totalVolume_; }

    bool isEmpty() const { return orders_.empty(); }

    //Price price_;
    Quantity totalVolume_;
    OrderPointers orders_;
};

struct OrderModify {
    OrderId id_;
    Price price_;
    Quantity quantity_;

    OrderPointer toOrderPointer(Side side, OrderType type, TimeInForce tif) const {
        return std::make_shared<Order>(id_, side, type, price_, quantity_, tif);
    }
};

// Main OrderBook class
/*
TODO:
Do I need sharedpointers or can it be stack allocated.

*/

class OrderBook {
public:
    OrderBook()
        : nextOrderId_(0)
        , nextTradeId_(0)
    {
    }
    ~OrderBook() = default;

    OrderBook(const OrderBook&) = delete;
    OrderBook& operator=(const OrderBook&) = delete;

    std::vector<Trade> addOrder(const OrderPointer& order) {
        std::vector<Trade> trades;
        if (order->type == OrderType::LIMIT) {
            if (order->tif == TimeInForce::FOK && !canFullyMatch(order)) {
                return trades;
            }
            trades = matchLimitOrder(order);
            if (!order->isFilled() && order->tif == TimeInForce::GTC) {
                PriceLevel& level = order->side == Side::BUY ? bids_[order->price] : asks_[order->price];
                level.addOrder(order);
                orderLookup_.emplace(order->id, OrderEntry{ order, std::prev(level.orders_.end()) });
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
        if (entry.order_->side == Side::BUY) {
            PriceLevel& level = bids_.at(entry.order_->price);
            level.removeOrder(entry.location_);
        }
        else {
            PriceLevel& level = asks_.at(entry.order_->price);
            level.removeOrder(entry.location_);
        }

        orderLookup_.erase(orderIt);
        return true;
    }

    std::vector<Trade> modifyOrder(const OrderModify& order) {
        auto it = orderLookup_.find(order.id_);
        if (it == orderLookup_.end()) throw std::logic_error("order doesnt exist");

        return {}; // no op
        
        const auto& standingOrder = it->second.order_;
        cancelOrder(order.id_);
        return addOrder(order.toOrderPointer(standingOrder->side, standingOrder->type, standingOrder->tif));
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
    };

    std::vector<BookLevel> getBidDepth(size_t levels) const {
        std::vector<BookLevel> depth;
        depth.reserve(std::min(levels, bids_.size()));
        for (auto it = bids_.begin(); levels > 0 && it != bids_.end(); --levels, ++it) {
            depth.emplace_back(it->first, it->second.getTotalVolume());
        }
        return depth;
    }

    std::vector<BookLevel> getAskDepth(size_t levels) const {
        std::vector<BookLevel> depth;
        depth.reserve(std::min(levels, bids_.size()));
        for (auto it = asks_.begin(); levels > 0 && it != asks_.end(); --levels, ++it) {
            depth.emplace_back(it->first, it->second.getTotalVolume());
        }
        return depth;
    }

    // Statistics
    size_t getOrderCount() const { return orderLookup_.size(); }
    bool isEmpty() const { return orderLookup_.empty(); }

private:
    std::map<Price, PriceLevel, std::greater<Price>> bids_;
    std::map<Price, PriceLevel> asks_;
    std::unordered_map<OrderId, OrderEntry> orderLookup_;
    bool canFullyMatch(const OrderPointer& order) const {
        Quantity vol = 0;
        if (order->side == Side::BUY) {
            if (asks_.empty()) return false;
            for (auto it = asks_.begin(); it != asks_.end() && it->first <= order->price; ++it) {
                vol += it->second.getTotalVolume();
                if (vol >= order->getRemainingQuantity()) return true;
            }
        } else {
            if (bids_.empty()) return false;
            for (auto it = bids_.begin(); it != bids_.end() && it->first >= order->price; ++it) {
                vol += it->second.getTotalVolume();
                if (vol >= order->getRemainingQuantity()) return true;
            }
        }
        return false;
    }

    // Matching engine
    std::vector<Trade> matchOrder(const OrderPointer& order) {
        if (order->type == OrderType::LIMIT) {
            return matchLimitOrder(order);
        } else {
            return matchMarketOrder(order);
        }
    }

    std::vector<Trade> matchLimitOrder(const OrderPointer& order) {
        std::vector<Trade> trades;

        if (order->side == Side::BUY) {
            while (!order->isFilled()) {
                if (asks_.empty()) break;

                auto bestAskIt = asks_.begin();
                Price askPrice = bestAskIt->first;
                PriceLevel& bestAsks = bestAskIt->second;

                if (askPrice > order->price) break;

                while (!bestAsks.orders_.empty() && !order->isFilled()) {
                    OrderPointer& standingOrder = bestAsks.orders_.front();
                    Quantity fillQty = std::min(order->getRemainingQuantity(), standingOrder->getRemainingQuantity());

                    order->fill(fillQty);
                    standingOrder->fill(fillQty);

                    trades.push_back(Trade{
                        order->id,
                        standingOrder->id,
                        askPrice,
                        fillQty
                    });

                    if (standingOrder->isFilled()) {
                        bestAsks.removeFrontOrder();
                        orderLookup_.erase(standingOrder->id);
                    }
                }

                if (bestAsks.orders_.empty()) {
                    asks_.erase(bestAskIt);
                }

            }
        } else {
            while (!order->isFilled()) {
                if (bids_.empty()) break;

                auto bestBidIt = bids_.begin();
                Price bidPrice = bestBidIt->first;
                auto& bestBids = bestBidIt->second;

                if (bidPrice < order->price) break;

                while (!bestBids.orders_.empty() && !order->isFilled()) {
                    OrderPointer& standingOrder = bestBids.orders_.front();
                    Quantity fillQty = std::min(order->getRemainingQuantity(), standingOrder->getRemainingQuantity());

                    order->fill(fillQty);
                    standingOrder->fill(fillQty);

                    trades.push_back(Trade{
                        order->id,
                        standingOrder->id,
                        bidPrice,
                        fillQty
                        });

                    if (standingOrder->isFilled()) {
                        bestBids.removeFrontOrder();
                        orderLookup_.erase(standingOrder->id);
                    }
                }

                if (bestBids.orders_.empty()) {
                    bids_.erase(bestBidIt);
                }
            }
        }

        return trades;
    }

    std::vector<Trade> matchMarketOrder(const OrderPointer& order) {
        std::vector<Trade> trades;

        if (order->side == Side::BUY) {
            while (!order->isFilled() && !asks_.empty()) {
                auto bestAskIt = asks_.begin();
                Price askPrice = bestAskIt->first;
                PriceLevel& bestAsks = bestAskIt->second;

                while (!bestAsks.orders_.empty() && !order->isFilled()) {
                    OrderPointer& standingOrder = bestAsks.orders_.front();
                    Quantity fillQty = std::min(order->getRemainingQuantity(), standingOrder->getRemainingQuantity());

                    order->fill(fillQty);
                    standingOrder->fill(fillQty);
                    trades.push_back(Trade{
                        order->id,
                        standingOrder->id,
                        askPrice,
                        fillQty
                    });

                    if (standingOrder->isFilled()) {
                        orderLookup_.erase(standingOrder->id); // check logic again (for errors, for some reason crash if swapped)
                        bestAsks.removeFrontOrder();
                    }
                }

                if (bestAsks.orders_.empty()) {
                    asks_.erase(bestAskIt);
                }

            }
        } else {
            while (!order->isFilled() && !bids_.empty()) {
                auto bestBidIt = bids_.begin();
                Price bidPrice = bestBidIt->first;
                auto& bestBids = bestBidIt->second;

                while (!bestBids.orders_.empty() && !order->isFilled()) {
                    OrderPointer& standingOrder = bestBids.orders_.front();
                    Quantity fillQty = std::min(order->getRemainingQuantity(), standingOrder->getRemainingQuantity());

                    order->fill(fillQty);
                    standingOrder->fill(fillQty);

                    trades.push_back(Trade{
                        order->id,
                        standingOrder->id,
                        bidPrice,
                        fillQty
                        });

                    if (standingOrder->isFilled()) {
                        orderLookup_.erase(standingOrder->id); // crash?
                        bestBids.removeFrontOrder();
                    }
                }

                if (bestBids.orders_.empty()) {
                    bids_.erase(bestBidIt);
                }
            }
        }
        return trades;
    }

    uint64_t nextOrderId_;
    uint64_t nextTradeId_;
};