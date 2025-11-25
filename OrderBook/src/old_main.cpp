//#include <OrderBook.h>
//#include <iostream>
//#include <random>
//#include <chrono>
//#include <cstdlib>
//
//// --- Ingestion simulator / benchmark ---
//
//struct BenchmarkResult {
//	size_t ordersAdded = 0;
//	size_t tradesEmitted = 0;
//	double seconds = 0.0;
//};
//
//BenchmarkResult run_ingestion(OrderBook& book, size_t numOrders, size_t priceLevels = 1000, unsigned int seed = 42, double fokRatio = 0.01) {
//	std::mt19937_64 rng(seed);
//	std::uniform_int_distribution<int> sideDist(0, 1);
//	std::uniform_int_distribution<Price> priceDist(1, static_cast<Price>(priceLevels));
//	std::uniform_int_distribution<Quantity> qtyDist(1, 100);
//	std::bernoulli_distribution fokDist(fokRatio);
//
//	size_t tradesProduced = 0;
//
//	auto t0 = std::chrono::high_resolution_clock::now();
//
//	for (size_t i = 1; i <= numOrders; ++i) {
//		Side side = sideDist(rng) == 0 ? Side::Buy : Side::Sell;
//		OrderType type = fokDist(rng) ? OrderType::FillOrKill : OrderType::GoodTillCancel;
//		Price p = priceDist(rng);
//		Quantity q = qtyDist(rng);
//
//		auto orderPtr = std::make_shared<Order>(static_cast<OrderId>(i), side, type, p, q);
//		auto trades = book.addOrder(orderPtr);
//		tradesProduced += trades.size();
//		// optionally sample progress
//		if ((i % 100000) == 0) {
//			std::cout << "Added " << i << " orders...\n";
//		}
//	}
//
//	auto t1 = std::chrono::high_resolution_clock::now();
//	double seconds = std::chrono::duration<double>(t1 - t0).count();
//
//	return BenchmarkResult{ numOrders, tradesProduced, seconds };
//}
//
//int main(int argc, char** argv) {
//	size_t numOrders = 1000000; // default 1M
//	size_t priceLevels = 1000;
//	unsigned int seed = 42;
//	double fokRatio = 0.01;
//
//	if (argc > 1) numOrders = static_cast<size_t>(std::strtoull(argv[1], nullptr, 10));
//	if (argc > 2) priceLevels = static_cast<size_t>(std::strtoull(argv[2], nullptr, 10));
//	if (argc > 3) seed = static_cast<unsigned int>(std::strtoul(argv[3], nullptr, 10));
//	if (argc > 4) fokRatio = std::atof(argv[4]);
//
//	std::cout << "Order book ingestion benchmark\n";
//	std::cout << "Orders: " << numOrders << ", Price levels: " << priceLevels << ", seed: " << seed << ", FOK ratio: " << fokRatio << "\n";
//
//	OrderBook book;
//	auto result = run_ingestion(book, numOrders, priceLevels, seed, fokRatio);
//
//	std::cout << "Done.\n";
//	std::cout << "Orders added: " << result.ordersAdded << "\n";
//	std::cout << "Trades emitted: " << result.tradesEmitted << "\n";
//	std::cout << "Elapsed seconds: " << result.seconds << "\n";
//	std::cout << "Orders/sec: " << (result.ordersAdded / result.seconds) << "\n";
//	std::cout << "Trades/sec: " << (result.tradesEmitted / result.seconds) << "\n";
//
//	return 0;
//}