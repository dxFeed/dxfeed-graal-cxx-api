// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include <doctest.h>

using namespace std::literals;
using namespace dxfcpp;

class IndexedTxModelFixture {
    std::shared_ptr<DXFeed> dxfeed;
    std::shared_ptr<IndexedTxModelListener<Order>> listener;

    public:
    IndexedTxModelFixture()
        : dxfeed(DXEndpoint::getInstance(DXEndpoint::Role::FEED)->connect("demo.dxfeed.com:7300")->getFeed()),
          listener(IndexedTxModelListener<Order>::create([](const auto &, const auto &events, bool isSnapshot) {
              if (isSnapshot) {
                  std::cout << "Snapshot:" << std::endl;
              } else {
                  std::cout << "Update:" << std::endl;
              }

              std::size_t i = 0;

              for (const auto &e : events) {
                  std::cout << "[" << e->getEventFlagsMask().toString() << "]:" << e << std::endl;

                  if (i++ == 10) {
                      std::cout << "..." << std::endl;

                      break;
                  }
              }

              std::cout << std::endl;
          })) {
    }

    protected:
    [[nodiscard]] std::shared_ptr<DXFeed> getDxFeed() const {
        return dxfeed;
    }

    [[nodiscard]] std::shared_ptr<IndexedTxModelListener<Order>> getListener() const {
        return listener;
    }
};

TEST_CASE_FIXTURE(IndexedTxModelFixture, "The model must subscribe to AAPL#NTV orders") {
    auto model = IndexedTxModel<Order>::newBuilder()
                     ->withFeed(getDxFeed())
                     ->withBatchProcessing(true)
                     ->withSnapshotProcessing(true)
                     ->withSources({OrderSource::NTV})
                     ->withListener(getListener())
                     ->withSymbol("AAPL")
                     ->build();

    std::this_thread::sleep_for(10s);
}

TEST_CASE_FIXTURE(IndexedTxModelFixture, "The model must subscribe to IBM#NTV and IBM#DEX orders simultaneously") {
    auto model = IndexedTxModel<Order>::newBuilder()
                     ->withFeed(getDxFeed())
                     ->withBatchProcessing(true)
                     ->withSnapshotProcessing(true)
                     ->withSources({OrderSource::NTV, OrderSource::DEX})
                     ->withListener(getListener())
                     ->withSymbol("IBM")
                     ->build();

    std::this_thread::sleep_for(10s);
}