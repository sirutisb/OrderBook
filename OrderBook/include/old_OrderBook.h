//#pragma once
//
//#include <map>
//#include <unordered_map>
//#include <list>
//#include <vector>
//#include <format>
//#include <memory>
//#include <stdexcept>
//
//enum class Side { Buy, Sell };
//enum class OrderType { FillOrKill, GoodTillCancel };
//
//using OrderId = uint32_t;
//using Quantity = uint32_t;
//using Price = uint32_t;
//
//class Order {
//public:
//	Order(OrderId orderId, Side side, OrderType orderType, Price price, Quantity quantity)
//		: orderId_(orderId)
//		, side_(side)
//		, orderType_(orderType)
//		, price_(price)
//		, initialQuantity_(quantity)
//		, remainingQuantity_(quantity)
//	{
//	}
//
//
//	OrderId getOrderId() const { return orderId_; }
//	Side getSide() const { return side_; }
//	OrderType getOrderType() const { return orderType_; }
//	Price getPrice() const { return price_; }
//	Quantity getInitialQuantity() const { return initialQuantity_; }
//	Quantity getRemainingQuantity() const { return remainingQuantity_; }
//	Quantity getFilledQuantity() const { return getInitialQuantity() - getRemainingQuantity(); }
//	bool isFilled() const { return getRemainingQuantity() == 0; }
//
//	void fill(Quantity quantity) {
//		if (quantity > remainingQuantity_) {
//			throw std::logic_error(
//				std::format("Order ({}) cannot be filled for more than its remaining quantity", getOrderId())
//			);
//		}
//		remainingQuantity_ -= quantity;
//	}
//
//
//private:
//	OrderId orderId_;
//	Side side_;
//	OrderType orderType_;
//	Price price_;
//	Quantity initialQuantity_;
//	Quantity remainingQuantity_;
//};
//
//using OrderPointer = std::shared_ptr<Order>;
//using OrderPointers = std::list<OrderPointer>;
//
//class OrderModify {
//public:
//	OrderModify(OrderId orderId, Side side, Price price, Quantity quantity)
//		: orderId_(orderId)
//		, side_(side)
//		, price_(price)
//		, quantity_(quantity)
//	{
//	}
//
//	OrderId getOrderId() const { return orderId_; }
//	Side getSide() const { return side_; }
//	Price getPrice() const { return price_; }
//	Quantity getQuantity() const { return quantity_; }
//
//	OrderPointer toOrderPointer(OrderType type) const {
//		return std::make_shared<Order>(orderId_, side_, type, price_, quantity_);
//	}
//
//private:
//	OrderId orderId_;
//	Side side_;
//	Price price_;
//	Quantity quantity_;
//};
//
//struct TradeInfo {
//	OrderId orderId_;
//	Price price_;
//	Quantity quantity_;
//};
//
//class Trade {
//public:
//	Trade(const TradeInfo& bidTrade, const TradeInfo& askTrade)
//		: bidTrade_(bidTrade)
//		, askTrade_(askTrade)
//	{
//	}
//
//	const TradeInfo& getBidTrade() const { return bidTrade_; }
//	const TradeInfo& getAskTrade() const { return askTrade_; }
//private:
//	TradeInfo bidTrade_;
//	TradeInfo askTrade_;
//};
//
//using Trades = std::vector<Trade>;
//
//struct OrderEntry {
//	OrderPointer order_{ nullptr };
//	OrderPointers::iterator location_;
//};
//
//class OrderBook {
//public:
//	Trades addOrder(OrderPointer order) {
//		if (orders_.contains(order->getOrderId())) {
//			throw std::logic_error(
//				std::format("An order with id ({}) already exists in the order book", order->getOrderId())
//			);
//		}
//		if (order->getOrderType() == OrderType::FillOrKill && !canMatch(order->getSide(), order->getPrice()))
//			return {};
//
//		OrderPointers::iterator it;
//		if (order->getSide() == Side::Buy) {
//			auto& buyOrders = bids_[order->getPrice()];
//			buyOrders.push_back(order);
//			it = std::prev(buyOrders.end());
//		} else {
//			auto& sellOrders = asks_[order->getPrice()];
//			sellOrders.push_back(order);
//			it = std::prev(sellOrders.end());
//		}
//
//		orders_.emplace(order->getOrderId(), OrderEntry{ order, it });
//		return matchOrders();
//	}
//
//	void cancelOrder(OrderId orderId) {
//		auto mapIt = orders_.find(orderId);
//		if (mapIt == orders_.end()) return;
//
//		// copy the entry to avoid referencing into the map after erase
//		OrderEntry entry = mapIt->second;
//		auto order = entry.order_;
//		orders_.erase(mapIt);
//
//		if (order->getSide() == Side::Buy) {
//			Price price = order->getPrice();
//			auto bidsIt = bids_.find(price);
//			if (bidsIt != bids_.end()) {
//				bidsIt->second.erase(entry.location_);
//				if (bidsIt->second.empty()) bids_.erase(bidsIt);
//			}
//		} else {
//			Price price = order->getPrice();
//			auto asksIt = asks_.find(price);
//			if (asksIt != asks_.end()) {
//				asksIt->second.erase(entry.location_);
//				if (asksIt->second.empty()) asks_.erase(asksIt);
//			}
//		}
//	}
//
//	Trades modifyOrder(OrderModify order) {
//		auto it = orders_.find(order.getOrderId());
//		if (it == orders_.end()) return {};
//		// read existing order type before cancelling
//		OrderType existingType = it->second.order_->getOrderType();
//		cancelOrder(order.getOrderId());
//		return addOrder(order.toOrderPointer(existingType));
//	}
//
//private:
//	bool canMatch(Side side, Price price) const {
//		if (side == Side::Buy) {
//			if (asks_.empty()) return false;
//			const auto& [bestAsk, _] = *asks_.begin();
//			return price >= bestAsk;
//		} else {
//			if (bids_.empty()) return false;
//			const auto& [bestBid, _] = *bids_.begin();
//			return price <= bestBid;
//		}
//	}
//
//	Trades matchOrders() {
//		Trades trades;
//
//		while (!bids_.empty() && !asks_.empty()) {
//			auto bidIt = bids_.begin();
//			auto askIt = asks_.begin();
//			Price bidPrice = bidIt->first;
//			Price askPrice = askIt->first;
//			if (bidPrice < askPrice) break;
//
//			// always copy the front orders out so we don't hold references to map nodes
//			if (bidIt->second.empty() || askIt->second.empty()) break;
//			auto bidOrderPtr = bidIt->second.front();
//			auto askOrderPtr = askIt->second.front();
//			OrderId bidId = bidOrderPtr->getOrderId();
//			OrderId askId = askOrderPtr->getOrderId();
//
//			Quantity fillQuantity = std::min(bidOrderPtr->getRemainingQuantity(), askOrderPtr->getRemainingQuantity());
//			bidOrderPtr->fill(fillQuantity);
//			askOrderPtr->fill(fillQuantity);
//
//			if (bidOrderPtr->isFilled()) {
//				orders_.erase(bidId);
//				bidIt->second.pop_front();
//			}
//
//			if (askOrderPtr->isFilled()) {
//				orders_.erase(askId);
//				askIt->second.pop_front();
//			}
//
//			// erase price levels if empty (erase by iterator)
//			if (bidIt->second.empty()) bids_.erase(bidIt);
//			if (askIt->second.empty()) asks_.erase(askIt);
//
//			trades.emplace_back(
//				TradeInfo{ bidId, bidPrice, fillQuantity },
//				TradeInfo{ askId, askPrice, fillQuantity }
//			);
//
//			// after processing one fill, loop to re-evaluate top-of-book
//		}
//
//		// kill remaining fillOrKill orders at top of book
//		if (!bids_.empty()) {
//			auto topBidIt = bids_.begin();
//			if (!topBidIt->second.empty()) {
//				auto topOrder = topBidIt->second.front();
//				if (topOrder->getOrderType() == OrderType::FillOrKill) {
//					cancelOrder(topOrder->getOrderId());
//				}
//			}
//		}
//
//		if (!asks_.empty()) {
//			auto topAskIt = asks_.begin();
//			if (!topAskIt->second.empty()) {
//				auto topOrder = topAskIt->second.front();
//				if (topOrder->getOrderType() == OrderType::FillOrKill) {
//					cancelOrder(topOrder->getOrderId());
//				}
//			}
//		}
//
//		return trades;
//	}
//
//	// O(log n) access to list of orders via price
//	// O(1) access to the top orders
//	std::map<Price, OrderPointers, std::greater<>> bids_;
//	std::map<Price, OrderPointers, std::less<>> asks_;
//
//	// O(1) access to any order via its OrderId
//	std::unordered_map<OrderId, OrderEntry> orders_;
//};