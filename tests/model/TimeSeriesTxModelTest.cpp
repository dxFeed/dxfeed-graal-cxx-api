// Copyright (c) 2024 Devexperts LLC.
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

class TimeSeriesTxModelFixture {
    std::shared_ptr<DXFeed> dxfeed;
    std::shared_ptr<TxModelListener<Candle>> listener;

  public:
    TimeSeriesTxModelFixture()
        : dxfeed(DXEndpoint::getInstance(DXEndpoint::Role::FEED)->connect("demo.dxfeed.com:7300")->getFeed()),
          listener(TxModelListener<Candle>::create([](const auto &, const auto &events, bool isSnapshot) {
              if (isSnapshot) {
                  std::cout << "Snapshot:" << std::endl;
              } else {
                  std::cout << "Update:" << std::endl;
              }

              for (const auto &e : events) {
                  std::cout << "[" << e->getEventFlagsMask().toString() << "]:" << e << std::endl;
              }

              std::cout << std::endl;
          })) {
    }

  protected:
    [[nodiscard]] std::shared_ptr<DXFeed> getDxFeed() const {
        return dxfeed;
    }

    [[nodiscard]] std::shared_ptr<TxModelListener<Candle>> getListener() const {
        return listener;
    }
};

TEST_CASE_FIXTURE(TimeSeriesTxModelFixture, "The model must subscribe to AAPL&Q{=1m} candles") {
    auto model = TimeSeriesTxModel::newBuilder(Candle::TYPE)
                     ->withFeed(getDxFeed())
                     ->withBatchProcessing(true)
                     ->withSnapshotProcessing(true)
                     ->withFromTime(std::chrono::milliseconds(dxfcpp::now()) - std::chrono::days(3))
                     ->withListener(getListener())
                     ->withSymbol(CandleSymbol::valueOf("AAPL&Q{=1m}"))
                     //->withSymbol("AAPL&Q{=1m}")
                     ->build();

    std::this_thread::sleep_for(10s);
}