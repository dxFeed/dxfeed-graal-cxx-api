// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfeed_graal_cpp_api/api.hpp>

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <memory>
#include <mutex>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

using namespace dxfcpp;
using namespace std::literals;

/*
 * Builds an order book of a symbol and an order source with IndexedEventTxModel and prints its top levels with the
 * scope of each order every second. Without an address it uses the default DXFeed instance, whose data feed address
 * is defined by the "dxfeed.system.properties" file. The properties file is copied to the build output directory from
 * the project directory.
 */

void printUsage() {
    const auto usageString = R"(
Usage:
SimpleOrderBookSample <symbol> <source> [<address>] [scope=<scope>] [--duration <seconds>]

Where:
    symbol   - The security symbol to get events for (e.g. "AAPL").
    source   - The order source (e.g. "ntv").
    address  - The address to connect to (e.g. "demo.dxfeed.com:7300"). The default is the address of the
               "dxfeed.system.properties" file.
    scope    - Shows only the orders of the scope: "composite", "regional", "aggregate" or "order".
    duration - Run for the given number of seconds and exit: the exit code is 0 if the book had orders, 1 otherwise.
               Without it the sample runs until it is stopped.

Examples:
    SimpleOrderBookSample AAPL ntv
    SimpleOrderBookSample AAPL ntv demo.dxfeed.com:7300 scope=aggregate --duration 10)";

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

    const Scope *scope_{}; // Shows only the orders of this scope, all the orders if null.
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
    explicit OrderListener(const Scope *scope) : scope_{scope} {
    }

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

            // Add the order to the appropriate side if needed.
            if (order->hasSize() && (scope_ == nullptr || order->getScope() == *scope_)) {
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

        info << " [Source: " << order->getSource().toString() << ", Scope: " << order->getScope().getName()
             << ", Size: " << std::fixed << std::setprecision(4) << std::setw(8) << order->getSize()
             << ", Price: " << std::setprecision(2) << std::setw(8) << order->getPrice() << "]";
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

// Finds a scope by its name, ignoring the case: "order" is Scope::ORDER.
const Scope *findScope(std::string name) {
    std::ranges::transform(name, name.begin(), [](char c) {
        return static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    });

    for (const auto &scope : Scope::ALL) {
        if (scope.get().getName() == name) {
            return &scope.get();
        }
    }

    return nullptr;
}

int main(int argc, char *argv[]) {
    try {
        if (argc < 3) {
            printUsage();

            return 0;
        }

        const std::string symbol = argv[1];
        const auto source = OrderSource::valueOf(argv[2]);
        std::optional<std::string> address{};
        const Scope *scope{};
        std::optional<std::chrono::duration<double>> duration{};

        for (int i = 3; i < argc; i++) {
            const std::string arg = argv[i];

            if (arg == "--duration") {
                if (i + 1 == argc) {
                    std::cerr << "No value of --duration\n";
                    printUsage();

                    return 1;
                }

                duration = std::chrono::duration<double>(std::stod(argv[++i]));
            } else if (arg.starts_with("scope=")) {
                scope = findScope(arg.substr(6));

                if (scope == nullptr) {
                    std::cerr << "Unknown scope: " << arg.substr(6) << '\n';
                    printUsage();

                    return 1;
                }
            } else if (!address) {
                address = arg;
            } else {
                std::cerr << "Unexpected argument: " << arg << '\n';
                printUsage();

                return 1;
            }
        }

        const auto listener = std::make_shared<OrderListener>(scope);

        // Without an address the endpoint address to use is stored in the "dxfeed.system.properties" file.
        const auto endpoint = address ? DXEndpoint::create()->connect(*address) : nullptr;
        const auto feed = endpoint ? endpoint->getFeed() : DXFeed::getInstance();

        const auto model = IndexedEventTxModel<Order>::newBuilder()
                               ->withFeed(feed)
                               ->withSymbol(symbol)
                               ->withSource(source)
                               ->withListener([listener](const auto &events, bool isSnapshot) {
                                   listener->eventsReceived(events, isSnapshot);
                               })
                               ->build();

        constexpr std::size_t depthLimit = 5;
        const auto deadline =
            std::chrono::steady_clock::now() + std::chrono::duration_cast<std::chrono::steady_clock::duration>(
                                                   duration.value_or(std::chrono::duration<double>::zero()));
        bool hadOrders = false;

        while (!duration || std::chrono::steady_clock::now() < deadline) {
            std::this_thread::sleep_for(1s);

            // Prints an order book every second; the last read is after the last second of the run.
            const auto book = listener->getBook();

            hadOrders = hadOrders || !book.buyOrders.empty() || !book.sellOrders.empty();
            printBook(book, depthLimit);
        }

        if (!hadOrders) {
            std::cout << "No orders" << std::endl;
        }

        model->close();

        if (endpoint) {
            endpoint->closeAndAwaitTermination();
        }

        return hadOrders ? 0 : 1;
    } catch (const RuntimeException &e) {
        std::cerr << e << '\n';

        return 1;
    } catch (const std::exception &e) {
        // For example, std::stod of an invalid number.
        std::cerr << e.what() << '\n';

        return 1;
    }
}
