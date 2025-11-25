//#include "orderBook.h"
//
//Order::Order(OrderId id, Side side, OrderType type, Price price, Quantity qty, TimeInForce tif)
//	: id(id)
//	, side(side)
//	, type(type)
//	, price(price)
//	, quantity(qty)
//	, filledQuantity(0)
//	, tif(tif)
//{
//	timestamp = time(nullptr);
//}
//
//void Order::fill(Quantity amount)
//{
//	filledQuantity += amount;
//}
//
//bool Order::isFilled() const
//{
//	return filledQuantity == quantity;
//}
//
//Quantity Order::getRemainingQuantity() const
//{
//	return quantity - filledQuantity;
//}
//
//
//PriceLevel::PriceLevel(Price price)
//	: price_(price)
//	, totalVolume_(0)
//{
//}
//
//void PriceLevel::addOrder(const std::shared_ptr<Order>& order)
//{
//}
//
//void PriceLevel::removeOrder(OrderId orderId)
//{
//}
//
//Quantity PriceLevel::getTotalVolume() const
//{
//	return totalVolume_;
//}
//
//bool PriceLevel::isEmpty() const
//{
//	return orders_.empty();
//}
//
//std::list<std::shared_ptr<Order>>& PriceLevel::getOrders()
//{
//	return orders_;
//}
//
//OrderBook::OrderBook()
//	: nextOrderId_(1)
//	, nextTradeId_(1)
//{
//}
//
//OrderBook::~OrderBook()
//{
//}
//
//std::vector<Trade> OrderBook::addOrder(const Order& order)
//{
//	return std::vector<Trade>();
//}
//
//bool OrderBook::cancelOrder(OrderId orderId)
//{
//	return false;
//}
//
//bool OrderBook::modifyOrder(OrderId orderId, Quantity newQuantity)
//{
//	return false;
//}
//
//Price OrderBook::getBestBid() const
//{
//	return bids_.begin()->first;
//}
//
//Price OrderBook::getBestAsk() const
//{
//	return asks_.begin()->first;
//}
//
//Price OrderBook::getSpread() const
//{
//	return getBestAsk() - getBestBid();
//}
//
//Quantity OrderBook::getVolumeAtPrice(Price price, Side side) const
//{
//	if (side == Side::BUY) {
//		const PriceLevel& level = bids_.at(price);
//		return level.getTotalVolume();
//	} else {
//		const PriceLevel& level = asks_.at(price);
//		return level.getTotalVolume();
//	}
//}
//
//std::vector<OrderBook::BookLevel> OrderBook::getBidDepth(size_t levels) const
//{
//	std::vector<BookLevel> result;
//	result.reserve(levels);
//	for (auto it = bids_.begin(); it != std::next(bids_.begin(), levels); ++it) {
//		// price, volume
//		result.emplace_back(it->first, it->second.getTotalVolume());
//	}
//	return result;
//}
//
//std::vector<OrderBook::BookLevel> OrderBook::getAskDepth(size_t levels) const
//{
//	std::vector<BookLevel> result;
//	result.reserve(levels);
//	for (auto it = asks_.begin(); it != std::next(asks_.begin(), levels); ++it) {
//		// price, volume
//		result.emplace_back(it->first, it->second.getTotalVolume());
//	}
//	return result;
//}
//
//size_t OrderBook::getOrderCount() const
//{
//	return orderLookup_.size();
//}
//
//bool OrderBook::isEmpty() const
//{
//	return orderLookup_.empty();
//}
//
//std::vector<Trade> OrderBook::matchOrder(const std::shared_ptr<Order>& order)
//{
//	if (order->type == OrderType::LIMIT) {
//		return matchLimitOrder(order);
//	} else {
//		return matchMarketOrder(order);
//	}
//}
//
//std::vector<Trade> OrderBook::matchLimitOrder(const std::shared_ptr<Order>& order)
//{
//	auto matchSide = [&](auto& oppositeSide) {
//		std::vector<Trade> trades;
//		while (!oppositeSide.empty() && !order->isFilled()) {
//			auto& [levelPrice, level] = *oppositeSide.begin();
//			if (order->side == Side::BUY && levelPrice > order->price) break;
//			if (order->side == Side::SELL && levelPrice < order->price) break;
//
//			auto& orders = level.getOrders();
//			while (!orders.empty() && !order->isFilled()) {
//				auto& restingOrder = orders.front();
//				Quantity fillQty = std::min(order->getRemainingQuantity(), restingOrder->getRemainingQuantity());
//				order->fill(fillQty);
//				restingOrder->fill(fillQty);
//
//				Trade trade{};
//
//				if (restingOrder->isFilled()) {
//					orderLookup_.erase(restingOrder->id);
//					orders.pop_front();
//					if (orders.empty()) {
//						oppositeSide.erase(levelPrice);
//						break;
//					}
//				}
//			}
//		}
//
//		if (!order->isFilled() && order->tif == TimeInForce::GTC) {
//			insertOrder(order); // TODO: insert side (without manual check)
//		}
//
//		return trades;
//	};
//
//	return (order->side == Side::BUY) ? matchSide(asks_) : matchSide(bids_);
//}
//
//std::vector<Trade> OrderBook::matchMarketOrder(const std::shared_ptr<Order>& order)
//{
//	std::vector<Trade> trades;
//
//	return trades;
//}
//
//void OrderBook::insertOrder(const std::shared_ptr<Order>& order)
//{
//	orderLookup_[order->id] = order;
//	if (order->side == Side::BUY) {
//		bids_[order->price].addOrder(order);
//	} else {
//		asks_[order->price].addOrder(order);
//	}
//}
//
//void OrderBook::removeOrderFromBook(const std::shared_ptr<Order>& order)
//{
//	orderLookup_.erase(order->id);
//	if (order->side == Side::BUY) {
//		bids_[order->price].removeOrder(order->id);
//	} else {
//		asks_[order->price].removeOrder(order->id);
//	}
//}
//
//Trade OrderBook::executeTrade(const std::shared_ptr<Order>& buyOrder, const std::shared_ptr<Order>& sellOrder, Quantity quantity)
//{
//	return Trade();
//}
