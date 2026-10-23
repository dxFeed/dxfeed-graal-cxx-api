// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

// Expected values recorded from the Java API (QD 3.356): OrderImbalance.
// Change the values only from a new run of the Java probe, not by hand.

#pragma once

#include <cstdint>
#include <limits>

namespace events_java_parity {

inline constexpr double NaN_ = std::numeric_limits<double>::quiet_NaN();
inline constexpr double INF_ = std::numeric_limits<double>::infinity();

// What the Java event class implements.
struct TypeFlags {
    const char *name;
    bool lasting;
    bool indexed;
    bool timeSeries;
    bool market;
};

inline const TypeFlags TYPE_FLAGS[] = {
    {"OrderImbalance", false, false, false, true},
};

inline constexpr std::int64_t ORDER_IMBALANCE_MAX_SEQUENCE = 4194303;

// The field values of the OrderImbalance cases and what Java toString() prints for them (with the time zone UTC).
struct OrderImbalanceCase {
    const char *name;
    const char *str;
    const char *symbol; // nullptr: the Java null symbol
    const char *source;
    std::int64_t eventTime;
    std::int64_t time;
    std::int32_t sequence;
    double refPrice;
    double pairedSize;
    double imbalanceSize;
    double nearPrice;
    double farPrice;
    const char *imbalanceSide;
    const char *auctionType;
};

inline const OrderImbalanceCase ORDER_IMBALANCE[] = {
    {"default",
     "OrderImbalance{null, eventTime=0, source=DEFAULT, time=0, sequence=0, refPrice=NaN, pairedSize=NaN, "
     "imbalanceSize=NaN, nearPrice=NaN, farPrice=NaN, imbalanceSide=UNDEFINED, auctionType=UNDEFINED}",
     nullptr, "DEFAULT", 0LL, 0LL, 0, NaN_, NaN_, NaN_, NaN_, NaN_, "UNDEFINED", "UNDEFINED"},
    {"defaultSym",
     "OrderImbalance{AAPL, eventTime=0, source=DEFAULT, time=0, sequence=0, refPrice=NaN, pairedSize=NaN, "
     "imbalanceSize=NaN, nearPrice=NaN, farPrice=NaN, imbalanceSide=UNDEFINED, auctionType=UNDEFINED}",
     "AAPL", "DEFAULT", 0LL, 0LL, 0, NaN_, NaN_, NaN_, NaN_, NaN_, "UNDEFINED", "UNDEFINED"},
    {"case0",
     "OrderImbalance{AAPL, eventTime=20231114-221321.456, source=NTV, time=20231114-221320.123, "
     "sequence=42, refPrice=150.25, pairedSize=1200.0, imbalanceSize=300.0, nearPrice=150.5, "
     "farPrice=149.75, imbalanceSide=BUY, auctionType=OPENING}",
     "AAPL", "NTV", 1700000001456LL, 1700000000123LL, 42, 150.25, 1200.0, 300.0, 150.5, 149.75, "BUY", "OPENING"},
    {"case1",
     "OrderImbalance{IBM, eventTime=20231114-221322.000, source=NUAM, time=20231114-221321.000, "
     "sequence=0, refPrice=1.0E7, pairedSize=1.0E-5, imbalanceSize=1.23456789E7, nearPrice=NaN, "
     "farPrice=-0.0, imbalanceSide=SELL, auctionType=CLOSING}",
     "IBM", "NUAM", 1700000002000LL, 1700000001000LL, 0, 10000000.0, 1e-05, 12345678.9, NaN_, -0.0, "SELL", "CLOSING"},
    {"case2",
     "OrderImbalance{MSFT, eventTime=0, source=ARCA, time=20231114-221323.999, sequence=4194303, "
     "refPrice=0.001, pairedSize=1.0E21, imbalanceSize=-2.5, nearPrice=1.0E-7, farPrice=Infinity, "
     "imbalanceSide=PAUSED, auctionType=OTHER}",
     "MSFT", "ARCA", 0LL, 1700000003999LL, 4194303, 0.001, 1e+21, -2.5, 1e-07, INF_, "PAUSED", "OTHER"},
    {"case3",
     "OrderImbalance{ES, eventTime=20231114-221324.001, source=DEFAULT, time=20231114-221324.000, "
     "sequence=7, refPrice=4500.25, pairedSize=10.0, imbalanceSize=5.0, nearPrice=4500.0, "
     "farPrice=4501.0, imbalanceSide=NO_IMBALANCE, auctionType=REOPENING}",
     "ES", "DEFAULT", 1700000004001LL, 1700000004000LL, 7, 4500.25, 10.0, 5.0, 4500.0, 4501.0, "NO_IMBALANCE",
     "REOPENING"},
    {"case4",
     "OrderImbalance{NQ, eventTime=20231114-221325.000, source=NTV, time=20231114-221325.000, sequence=1, "
     "refPrice=2.0, pairedSize=3.0, imbalanceSize=4.0, nearPrice=5.0, farPrice=6.0, "
     "imbalanceSide=INSUFFICIENT_ORDERS, auctionType=UNDEFINED}",
     "NQ", "NTV", 1700000005000LL, 1700000005000LL, 1, 2.0, 3.0, 4.0, 5.0, 6.0, "INSUFFICIENT_ORDERS", "UNDEFINED"},
    {"enum0",
     "OrderImbalance{ENUM, eventTime=20231114-221326.000, source=NTV, time=20231114-221326.000, "
     "sequence=0, refPrice=10.0, pairedSize=20.0, imbalanceSize=30.0, nearPrice=40.0, farPrice=50.0, "
     "imbalanceSide=UNDEFINED, auctionType=UNDEFINED}",
     "ENUM", "NTV", 1700000006000LL, 1700000006000LL, 0, 10.0, 20.0, 30.0, 40.0, 50.0, "UNDEFINED", "UNDEFINED"},
    {"enum1",
     "OrderImbalance{ENUM, eventTime=20231114-221326.000, source=NTV, time=20231114-221326.000, "
     "sequence=1, refPrice=11.0, pairedSize=21.0, imbalanceSize=31.0, nearPrice=41.0, farPrice=51.0, "
     "imbalanceSide=BUY, auctionType=OPENING}",
     "ENUM", "NTV", 1700000006000LL, 1700000006000LL, 1, 11.0, 21.0, 31.0, 41.0, 51.0, "BUY", "OPENING"},
    {"enum2",
     "OrderImbalance{ENUM, eventTime=20231114-221326.000, source=NTV, time=20231114-221326.000, "
     "sequence=2, refPrice=12.0, pairedSize=22.0, imbalanceSize=32.0, nearPrice=42.0, farPrice=52.0, "
     "imbalanceSide=SELL, auctionType=CLOSING}",
     "ENUM", "NTV", 1700000006000LL, 1700000006000LL, 2, 12.0, 22.0, 32.0, 42.0, 52.0, "SELL", "CLOSING"},
    {"enum3",
     "OrderImbalance{ENUM, eventTime=20231114-221326.000, source=NTV, time=20231114-221326.000, "
     "sequence=3, refPrice=13.0, pairedSize=23.0, imbalanceSize=33.0, nearPrice=43.0, farPrice=53.0, "
     "imbalanceSide=NO_IMBALANCE, auctionType=REOPENING}",
     "ENUM", "NTV", 1700000006000LL, 1700000006000LL, 3, 13.0, 23.0, 33.0, 43.0, 53.0, "NO_IMBALANCE", "REOPENING"},
    {"enum4",
     "OrderImbalance{ENUM, eventTime=20231114-221326.000, source=NTV, time=20231114-221326.000, "
     "sequence=4, refPrice=14.0, pairedSize=24.0, imbalanceSize=34.0, nearPrice=44.0, farPrice=54.0, "
     "imbalanceSide=INSUFFICIENT_ORDERS, auctionType=OTHER}",
     "ENUM", "NTV", 1700000006000LL, 1700000006000LL, 4, 14.0, 24.0, 34.0, 44.0, 54.0, "INSUFFICIENT_ORDERS", "OTHER"},
    {"enum5",
     "OrderImbalance{ENUM, eventTime=20231114-221326.000, source=NTV, time=20231114-221326.000, "
     "sequence=5, refPrice=15.0, pairedSize=25.0, imbalanceSize=35.0, nearPrice=45.0, farPrice=55.0, "
     "imbalanceSide=PAUSED, auctionType=UNDEFINED}",
     "ENUM", "NTV", 1700000006000LL, 1700000006000LL, 5, 15.0, 25.0, 35.0, 45.0, 55.0, "PAUSED", "UNDEFINED"},
};

} // namespace events_java_parity
