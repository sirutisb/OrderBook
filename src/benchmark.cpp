#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <random>
#include <vector>

#include "orderBook.h"

struct BenchmarkResult {
    double seconds = 0.0;
    std::uint64_t ops = 0;
    std::uint64_t adds = 0;
    std::uint64_t cancels = 0;
    std::uint64_t modifies = 0;
    std::uint64_t trades = 0;
    std::size_t finalRestingOrders = 0;
    std::size_t trackedActiveIds = 0;
};

static BenchmarkResult runBenchmark() {
    using Clock = std::chrono::steady_clock;

    constexpr std::uint64_t kNumOps = 10'000'000ULL;
    constexpr std::uint64_t kSeed = 0xC0FFEEULL;
    constexpr std::size_t kMaxActiveIds = 200000;

    // Action mix (percent)
    constexpr int kAddPct = 70;
    constexpr int kCancelPct = 15;
    constexpr int kModifyPct = 15;
    static_assert(kAddPct + kCancelPct + kModifyPct == 100);

    // Order mix
    constexpr int kMarketPct = 5; // small % to exercise matching

    OrderBook orderBook;
    std::vector<OrderId> activeOrderIds;
    activeOrderIds.reserve(std::min<std::size_t>(kMaxActiveIds, 100000));
    OrderId nextOrderId = 1;

    std::mt19937_64 rng(kSeed);

    Price centerPrice = 10000;
    Price spreadHalf = 50;

    std::uniform_int_distribution<Price> priceDistBuy(centerPrice - spreadHalf - 100, centerPrice - spreadHalf);
    std::uniform_int_distribution<Price> priceDistSell(centerPrice + spreadHalf, centerPrice + spreadHalf + 100);
    std::uniform_int_distribution<Price> priceDistAny(centerPrice - 300, centerPrice + 300);
    std::uniform_int_distribution<Quantity> quantityDist(1, 100);
    std::uniform_int_distribution<int> sideDist(0, 1);
    std::uniform_int_distribution<int> actionDist(0, 99);
    std::uniform_int_distribution<int> pctDist(0, 99);

    BenchmarkResult result;
    const auto start = Clock::now();

    auto addOrder = [&]() {
        const Side side = (sideDist(rng) == 0) ? Side::BUY : Side::SELL;
        const bool isMarket = (pctDist(rng) < kMarketPct);
        const OrderType type = isMarket ? OrderType::MARKET : OrderType::LIMIT;
        const TimeInForce tif = isMarket ? TimeInForce::IOC : TimeInForce::GTC;

        const Price price = isMarket ? 0 : ((side == Side::BUY) ? priceDistBuy(rng) : priceDistSell(rng));
        const Quantity qty = quantityDist(rng);
        const OrderId id = nextOrderId++;

        Order order(id, side, type, price, qty, tif);
        const auto trades = orderBook.addOrder(order);

        result.ops++;
        result.adds++;
        result.trades += static_cast<std::uint64_t>(trades.size());

        if (type == OrderType::LIMIT && tif == TimeInForce::GTC && !order.isFilled()) {
            activeOrderIds.push_back(id);
        }
    };

    auto cancelOrder = [&]() {
        if (activeOrderIds.empty()) {
            return;
        }

        const std::size_t idx = std::uniform_int_distribution<std::size_t>(0, activeOrderIds.size() - 1)(rng);
        const OrderId id = activeOrderIds[idx];

        (void)orderBook.cancelOrder(id);
        activeOrderIds[idx] = activeOrderIds.back();
        activeOrderIds.pop_back();

        result.ops++;
        result.cancels++;
    };

    auto modifyOrder = [&]() {
        if (activeOrderIds.empty()) {
            return;
        }

        const std::size_t idx = std::uniform_int_distribution<std::size_t>(0, activeOrderIds.size() - 1)(rng);
        const OrderId id = activeOrderIds[idx];

        const Quantity newQty = quantityDist(rng);
        const Price newPrice = priceDistAny(rng);
        const OrderModify mod{ id, newPrice, newQty };

        try {
            const auto trades = orderBook.modifyOrder(mod);
            result.trades += static_cast<std::uint64_t>(trades.size());
            // If it traded, the order may have been fully filled; stop tracking to reduce stale IDs.
            if (!trades.empty()) {
                activeOrderIds[idx] = activeOrderIds.back();
                activeOrderIds.pop_back();
            }
        } catch (...) {
            // Order likely already gone; drop it from tracking.
            activeOrderIds[idx] = activeOrderIds.back();
            activeOrderIds.pop_back();
        }

        result.ops++;
        result.modifies++;
    };

    for (std::uint64_t i = 0; i < kNumOps; ++i) {
        // Keep tracking bounded so we don't benchmark vector growth.
        if (activeOrderIds.size() > kMaxActiveIds) {
            cancelOrder();
            continue;
        }

        const int action = actionDist(rng);
        if (action < kAddPct) {
            addOrder();
        } else if (action < kAddPct + kCancelPct) {
            cancelOrder();
        } else {
            modifyOrder();
        }
    }

    const auto end = Clock::now();
    const std::chrono::duration<double> elapsed = end - start;
    result.seconds = elapsed.count();
    result.finalRestingOrders = orderBook.getOrderCount();
    result.trackedActiveIds = activeOrderIds.size();
    return result;
}

int main() {
    const BenchmarkResult result = runBenchmark();
    const double opsPerSec = (result.seconds > 0.0) ? (static_cast<double>(result.ops) / result.seconds) : 0.0;

    std::cout
        << "BENCHMARK\n"
        << "Seconds: " << result.seconds << "\n"
        << "Ops: " << result.ops << "\n"
        << "Ops/sec: " << opsPerSec << "\n"
        << "Adds: " << result.adds << " Cancels: " << result.cancels << " Modifies: " << result.modifies << "\n"
        << "Trades: " << result.trades << "\n"
        << "Final resting orders: " << result.finalRestingOrders << "\n"
        << "Tracked active IDs: " << result.trackedActiveIds << "\n";

    return 0;
}
