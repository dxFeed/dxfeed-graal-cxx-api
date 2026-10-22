// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfeed_graal_cpp_api/api.hpp>

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <memory>
#include <mutex>
#include <set>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

using namespace dxfcpp;
using namespace std::literals;

/*
 * Builds an order book of a symbol and an order source with IndexedEventTxModel and prints its top levels every
 * second. Use the default DXFeed instance for that data feed address is defined by "dxfeed.system.properties" file.
 * The properties file is copied to the build output directory from the project directory.
 */

void printUsage() {
    const auto usageString = R"(
Usage:
SimpleOrderBookSample <symbol> <source>

Where:
    symbol - The security symbol to get events for (e.g. "AAPL").
    source - The order source (e.g. "ntv").

Example:
    SimpleOrderBookSample AAPL ntv)";

    std::cout << usageString << std::endl;
}

struct OrderBook {
    std::vector<std::shared_ptr<Order>> buyOrders;
    std::vector<std::shared_ptr<Order>> sellOrders;
};

class OrderListener {
    struct BuyOrderComparator {
        bool operator()(const std::shared_ptr<Order> &a, const std::shared_ptr<Order> &b) const {
            // The price in descending order, the index if the prices are equal.
            if (a->getPrice() > b->getPrice()) {
                return true;
            }

            if (a->getPrice() < b->getPrice()) {
                return false;
            }

            return a->getIndex() < b->getIndex();
        }
    };

    struct SellOrderComparator {
        bool operator()(const std::shared_ptr<Order> &a, const std::shared_ptr<Order> &b) const {
            // The price in ascending order, the index if the prices are equal.
            if (a->getPrice() < b->getPrice()) {
                return true;
            }

            if (a->getPrice() > b->getPrice()) {
                return false;
            }

            return a->getIndex() < b->getIndex();
        }
    };

    mutable std::mutex mutex_{};
    std::unordered_map<std::int64_t, std::shared_ptr<Order>> ordersByIndex_{};
    std::set<std::shared_ptr<Order>, BuyOrderComparator> buyOrders_{};
    std::set<std::shared_ptr<Order>, SellOrderComparator> sellOrders_{};

    void remove(const std::shared_ptr<Order> &order) {
        if (order->getOrderSide() == Side::BUY) {
            buyOrders_.erase(order);
        } else {
            sellOrders_.erase(order);
        }
    }

    void add(const std::shared_ptr<Order> &order) {
        if (order->getOrderSide() == Side::BUY) {
            buyOrders_.insert(order);
        } else {
            sellOrders_.insert(order);
        }
    }

    public:
    void eventsReceived(const std::vector<std::shared_ptr<Order>> &events, bool isSnapshot) {
        std::lock_guard lock{mutex_};

        if (isSnapshot) { // if this is a snapshot, clear the order book
            ordersByIndex_.clear();
            buyOrders_.clear();
            sellOrders_.clear();
        }

        for (const auto &order : events) {
            // Remove the old order if it exists.
            if (const auto found = ordersByIndex_.find(order->getIndex()); found != ordersByIndex_.end()) {
                remove(found->second);
                ordersByIndex_.erase(found);
            }

            if (IndexedEvent::REMOVE_EVENT.in(order->getEventFlagsMask())) { // skip removed events
                continue;
            }

            ordersByIndex_[order->getIndex()] = order;

            if (order->hasSize()) { // add the order to the appropriate side if needed
                add(order);
            }
        }
    }

    OrderBook getBook() const {
        std::lock_guard lock{mutex_};

        return {{buyOrders_.begin(), buyOrders_.end()}, {sellOrders_.begin(), sellOrders_.end()}};
    }
};

std::string getOrderInfo(const std::vector<std::shared_ptr<Order>> &orders, std::size_t i, const Side &side) {
    std::ostringstream info{};

    info << side.getName();

    if (i < orders.size()) {
        const auto &order = orders[i];

        info << " [Source: " << order->getSource().toString() << ", Size: " << std::fixed << std::setprecision(4)
             << std::setw(8) << order->getSize() << ", Price: " << std::setprecision(2) << std::setw(8)
             << order->getPrice() << "]";
    } else {
        info << " [None]";
    }

    return info.str();
}

void printBook(const OrderBook &book, std::size_t depthLimit) {
    if (book.buyOrders.empty() && book.sellOrders.empty()) {
        return;
    }

    const auto limit = std::min(std::max(book.buyOrders.size(), book.sellOrders.size()), depthLimit);

    for (std::size_t i = 0; i < limit; i++) {
        std::cout << getOrderInfo(book.buyOrders, i, Side::BUY) << "\t" << getOrderInfo(book.sellOrders, i, Side::SELL)
                  << "\n";
    }

    std::cout << std::endl;
}

int main(int argc, char *argv[]) {
    try {
        if (argc < 3) {
            printUsage();

            return 0;
        }

        const std::string symbol = argv[1];
        const auto source = OrderSource::valueOf(argv[2]);
        const auto listener = std::make_shared<OrderListener>();

        // The endpoint address to use is stored in the "dxfeed.system.properties" file.
        const auto model = IndexedEventTxModel<Order>::newBuilder()
                               ->withFeed(DXFeed::getInstance())
                               ->withSymbol(symbol)
                               ->withSource(source)
                               ->withListener([listener](const auto &events, bool isSnapshot) {
                                   listener->eventsReceived(events, isSnapshot);
                               })
                               ->build();

        constexpr std::size_t depthLimit = 5;

        while (true) {
            printBook(listener->getBook(), depthLimit); // prints an order book every second
            std::this_thread::sleep_for(1s);
        }
    } catch (const RuntimeException &e) {
        std::cerr << e << '\n';

        return 1;
    }
}
