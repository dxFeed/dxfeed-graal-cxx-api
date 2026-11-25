// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

// Expected values recorded from the Java API (QD 3.356): MarketMaker, NuamOrder, NuamTimeAndSale, NuamTrade,
// OrderImbalance, TextConfiguration.
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
    {"OrderImbalance", false, false, false, true}, {"NuamOrder", false, true, false, true},
    {"NuamTrade", true, false, false, true},       {"NuamTimeAndSale", false, true, true, true},
    {"MarketMaker", false, true, false, true},     {"TextConfiguration", true, false, false, false},
};

inline constexpr std::int64_t ORDER_IMBALANCE_MAX_SEQUENCE = 4194303;
inline constexpr std::int64_t TEXT_CONFIGURATION_MAX_SEQUENCE = 4194303;

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

// The field values of the NuamOrder cases and what Java toString() prints for them (with the time zone UTC).
struct NuamOrderCase {
    const char *name;
    const char *str;
    const char *symbol; // nullptr: the Java null symbol
    const char *source;
    std::int64_t eventTime;
    std::int32_t eventFlags;
    std::int64_t index;
    std::int64_t time;
    std::int32_t sequence;
    std::int32_t timeNanoPart;
    const char *action;
    std::int64_t actionTime;
    std::int64_t orderId;
    std::int64_t auxOrderId;
    double price;
    double size;
    double executedSize;
    std::int64_t count;
    std::int32_t exchangeCode;
    const char *orderSide;
    const char *scope;
    std::int64_t tradeId;
    double tradePrice;
    double tradeSize;
    const char *marketMaker; // nullptr: the Java null
    std::int32_t actorId;
    std::int32_t participantId;
    std::int32_t submitterId;
    std::int32_t onBehalfOfSubmitterId;
    const char *clientOrderId; // nullptr: the Java null
    const char *customerAccount; // nullptr: the Java null
    const char *customerInfo; // nullptr: the Java null
    const char *exchangeInfo; // nullptr: the Java null
    const char *timeInForce;
    std::int32_t timeInForceData;
    std::int32_t triggerOrderBookId;
    double triggerPrice;
    std::int32_t triggerSessionType;
    const char *orderType;
    double orderQuantity;
    double displayQuantity;
    double refreshQuantity;
    double leavesQuantity;
    double matchedQuantity;
};

inline const NuamOrderCase NUAM_ORDER[] = {
    {"default",
     "NuamOrder{null, eventTime=0, source=DEFAULT, eventFlags=0x0, index=0x0, time=0, sequence=0, "
     "timeNanoPart=0, action=UNDEFINED, actionTime=0, orderId=0, auxOrderId=0, price=NaN, size=NaN, "
     "executedSize=NaN, count=0, exchange=\\0, side=UNDEFINED, scope=COMPOSITE, tradeId=0, tradePrice=NaN, "
     "tradeSize=NaN, marketMaker='null', actorId=0, participantId=0, submitterId=0, "
     "onBehalfOfSubmitterId=0, clientOrderId='null', customerAccount='null', customerInfo='null', "
     "timeInForce=UNDEFINED, timeInForceData=0, triggerOrderBookId=0, triggerPrice=NaN, "
     "triggerSessionType=0, orderType=UNDEFINED, orderQuantity=NaN, displayQuantity=NaN, "
     "refreshQuantity=NaN, leavesQuantity=NaN, matchedQuantity=NaN}",
     nullptr, "DEFAULT", 0LL, 0, 0LL, 0LL, 0, 0, "UNDEFINED", 0LL, 0LL, 0LL, NaN_, NaN_, NaN_, 0LL, 0, "UNDEFINED",
     "COMPOSITE", 0LL, NaN_, NaN_, nullptr, 0, 0, 0, 0, nullptr, nullptr, nullptr, nullptr, "UNDEFINED", 0, 0, NaN_, 0,
     "UNDEFINED", NaN_, NaN_, NaN_, NaN_, NaN_},
    {"full",
     "NuamOrder{AAPL, eventTime=20231114-221321.456, source=NUAM, eventFlags=0x3, "
     "index=0x4e55414d0000004d, time=20231114-221320.123, sequence=42, timeNanoPart=123456, action=NEW, "
     "actionTime=20231114-221320.999, orderId=1234567890123, auxOrderId=987654321, price=150.25, "
     "size=100.5, executedSize=25.75, count=7, exchange=N, side=BUY, scope=ORDER, tradeId=5551212, "
     "tradePrice=150.5, tradeSize=10.5, marketMaker='MM01', actorId=11, participantId=22, submitterId=33, "
     "onBehalfOfSubmitterId=44, clientOrderId='CL-1_a', customerAccount='ACC-9', customerInfo='info:x', "
     "timeInForce=GTC, timeInForceData=3, triggerOrderBookId=555, triggerPrice=149.5, "
     "triggerSessionType=2, orderType=LIMIT, orderQuantity=1000.5, displayQuantity=100.25, "
     "refreshQuantity=50.5, leavesQuantity=850.0, matchedQuantity=150.5}",
     "AAPL", "NUAM", 1700000001456LL, 3, 77LL, 1700000000123LL, 42, 123456, "NEW", 1700000000999LL, 1234567890123LL,
     987654321LL, 150.25, 100.5, 25.75, 7LL, 78, "BUY", "ORDER", 5551212LL, 150.5, 10.5, "MM01", 11, 22, 33, 44,
     "CL-1_a", "ACC-9", "info:x", "XI-77", "GTC", 3, 555, 149.5, 2, "LIMIT", 1000.5, 100.25, 50.5, 850.0, 150.5},
    {"lowerSource",
     "NuamOrder{IBM, eventTime=20231114-221322.000, source=nuam, eventFlags=0x1, "
     "index=0x6e75616dffffffff, time=20231114-221321.000, sequence=1, timeNanoPart=0, action=REPLACE, "
     "actionTime=0, orderId=1, auxOrderId=0, price=0.001, size=9999999.5, executedSize=NaN, count=1, "
     "exchange=A, side=SELL, scope=AGGREGATE, tradeId=0, tradePrice=NaN, tradeSize=NaN, "
     "marketMaker='nuam', actorId=1, participantId=0, submitterId=0, onBehalfOfSubmitterId=0, "
     "clientOrderId='lower', customerAccount='null', customerInfo='null', timeInForce=REST_OF_DAY, "
     "timeInForceData=0, triggerOrderBookId=0, triggerPrice=NaN, triggerSessionType=0, orderType=MARKET, "
     "orderQuantity=2.5, displayQuantity=NaN, refreshQuantity=NaN, leavesQuantity=NaN, "
     "matchedQuantity=NaN}",
     "IBM", "nuam", 1700000002000LL, 1, 4294967295LL, 1700000001000LL, 1, 0, "REPLACE", 0LL, 1LL, 0LL, 0.001,
     9999999.5, NaN_, 1LL, 65, "SELL", "AGGREGATE", 0LL, NaN_, NaN_, "nuam", 1, 0, 0, 0, "lower", nullptr, nullptr,
     nullptr, "REST_OF_DAY", 0, 0, NaN_, 0, "MARKET", 2.5, NaN_, NaN_, NaN_, NaN_},
    {"nanNull",
     "NuamOrder{MSFT, eventTime=20231114-221323.000, source=NUAM, eventFlags=0x0, "
     "index=0x4e55414d00000000, time=0, sequence=0, timeNanoPart=0, action=UNDEFINED, actionTime=0, "
     "orderId=0, auxOrderId=0, price=NaN, size=NaN, executedSize=NaN, count=0, exchange=\\0, side=BUY, "
     "scope=REGIONAL, tradeId=0, tradePrice=NaN, tradeSize=NaN, marketMaker='null', actorId=0, "
     "participantId=0, submitterId=0, onBehalfOfSubmitterId=0, clientOrderId='null', "
     "customerAccount='null', customerInfo='null', timeInForce=UNDEFINED, timeInForceData=0, "
     "triggerOrderBookId=0, triggerPrice=NaN, triggerSessionType=0, orderType=UNDEFINED, "
     "orderQuantity=NaN, displayQuantity=NaN, refreshQuantity=NaN, leavesQuantity=NaN, "
     "matchedQuantity=NaN}",
     "MSFT", "NUAM", 1700000003000LL, 0, 0LL, 0LL, 0, 0, "UNDEFINED", 0LL, 0LL, 0LL, NaN_, NaN_, NaN_, 0LL, 0, "BUY",
     "REGIONAL", 0LL, NaN_, NaN_, nullptr, 0, 0, 0, 0, nullptr, nullptr, nullptr, nullptr, "UNDEFINED", 0, 0, NaN_, 0,
     "UNDEFINED", NaN_, NaN_, NaN_, NaN_, NaN_},
    {"outsideEmpty",
     "NuamOrder{ES, eventTime=0, source=NUAM, eventFlags=0x0, index=0x4e55414d00000000, time=0, "
     "sequence=0, timeNanoPart=0, action=UNDEFINED, actionTime=0, orderId=0, auxOrderId=0, price=100.5, "
     "size=3.5, executedSize=NaN, count=0, exchange=\\0, side=SELL, scope=COMPOSITE, tradeId=0, "
     "tradePrice=NaN, tradeSize=NaN, marketMaker='', actorId=0, participantId=0, submitterId=0, "
     "onBehalfOfSubmitterId=0, clientOrderId='', customerAccount='', customerInfo='', "
     "timeInForce=UNDEFINED, timeInForceData=0, triggerOrderBookId=0, triggerPrice=1.0E7, "
     "triggerSessionType=0, orderType=UNDEFINED, orderQuantity=1.0E-5, displayQuantity=1.0E21, "
     "refreshQuantity=-0.0, leavesQuantity=-1.5E10, matchedQuantity=Infinity}",
     "ES", "NUAM", 0LL, 0, 0LL, 0LL, 0, 0, "UNDEFINED", 0LL, 0LL, 0LL, 100.5, 3.5, NaN_, 0LL, 0, "SELL", "COMPOSITE",
     0LL, NaN_, NaN_, "", 0, 0, 0, 0, "", "", "", "", "UNDEFINED", 0, 0, 10000000.0, 0, "UNDEFINED", 1e-05, 1e+21,
     -0.0, -15000000000.0, INF_},
    {"extremeInts",
     "NuamOrder{NQ, eventTime=21000101-000000.123, source=nuam, eventFlags=0xffffffff, "
     "index=0x6e75616d7fffffff, time=20380119-031407.123, sequence=4194303, timeNanoPart=999999, "
     "action=UNDEFINED, actionTime=21000101-000000.456, orderId=9223372036854775807, "
     "auxOrderId=-9223372036854775808, price=0.001, size=1.5, executedSize=2.5, "
     "count=9223372036854775807, exchange=\\u007f, side=BUY, scope=COMPOSITE, "
     "tradeId=-9223372036854775808, tradePrice=3.5, tradeSize=4.5, marketMaker='ASCII', "
     "actorId=2147483647, participantId=-2147483648, submitterId=-1, onBehalfOfSubmitterId=2147483647, "
     "clientOrderId='a'b=c', customerAccount=',;=#', customerInfo='back\\slash', timeInForce=UNDEFINED, "
     "timeInForceData=-2147483648, triggerOrderBookId=2147483647, triggerPrice=-Infinity, "
     "triggerSessionType=-2147483648, orderType=UNDEFINED, orderQuantity=NaN, displayQuantity=NaN, "
     "refreshQuantity=NaN, leavesQuantity=NaN, matchedQuantity=-Infinity}",
     "NQ", "nuam", 4102444800123LL, -1, 2147483647LL, 4102444800123LL, 4194303, 999999, "UNDEFINED", 4102444800456LL,
     9223372036854775807LL, (-9223372036854775807LL - 1), 0.001, 1.5, 2.5, 9223372036854775807LL, 127, "BUY",
     "COMPOSITE", (-9223372036854775807LL - 1), 3.5, 4.5, "ASCII", 2147483647, (-2147483647 - 1), -1, 2147483647,
     "a'b=c", ",;=#", "back\\slash", "\"quoted\"", "UNDEFINED", (-2147483647 - 1), 2147483647, -INF_,
     (-2147483647 - 1), "UNDEFINED", NaN_, NaN_, NaN_, NaN_, -INF_},
    {"enum0",
     "NuamOrder{ENUM0, eventTime=20231114-221326.000, source=NUAM, eventFlags=0x0, "
     "index=0x4e55414d00000001, time=20231114-221326.000, sequence=0, timeNanoPart=0, action=UNDEFINED, "
     "actionTime=0, orderId=0, auxOrderId=0, price=10.5, size=20.0, executedSize=NaN, count=0, "
     "exchange=A, side=BUY, scope=COMPOSITE, tradeId=0, tradePrice=NaN, tradeSize=NaN, marketMaker='M0', "
     "actorId=0, participantId=0, submitterId=0, onBehalfOfSubmitterId=0, clientOrderId='C0', "
     "customerAccount='null', customerInfo='null', timeInForce=UNDEFINED, timeInForceData=100, "
     "triggerOrderBookId=0, triggerPrice=NaN, triggerSessionType=0, orderType=UNDEFINED, "
     "orderQuantity=1.25, displayQuantity=NaN, refreshQuantity=NaN, leavesQuantity=NaN, "
     "matchedQuantity=NaN}",
     "ENUM0", "NUAM", 1700000006000LL, 0, 1LL, 1700000006000LL, 0, 0, "UNDEFINED", 0LL, 0LL, 0LL, 10.5, 20.0, NaN_,
     0LL, 65, "BUY", "COMPOSITE", 0LL, NaN_, NaN_, "M0", 0, 0, 0, 0, "C0", nullptr, nullptr, nullptr, "UNDEFINED", 100,
     0, NaN_, 0, "UNDEFINED", 1.25, NaN_, NaN_, NaN_, NaN_},
    {"enum1",
     "NuamOrder{ENUM1, eventTime=20231114-221326.000, source=nuam, eventFlags=0x0, "
     "index=0x6e75616d00000002, time=20231114-221326.001, sequence=1, timeNanoPart=0, action=NEW, "
     "actionTime=0, orderId=0, auxOrderId=0, price=11.5, size=21.0, executedSize=NaN, count=0, "
     "exchange=B, side=SELL, scope=REGIONAL, tradeId=0, tradePrice=NaN, tradeSize=NaN, marketMaker='M1', "
     "actorId=1, participantId=0, submitterId=0, onBehalfOfSubmitterId=0, clientOrderId='C1', "
     "customerAccount='null', customerInfo='null', timeInForce=REST_OF_DAY, timeInForceData=101, "
     "triggerOrderBookId=0, triggerPrice=NaN, triggerSessionType=0, orderType=LIMIT, "
     "orderQuantity=1001.25, displayQuantity=NaN, refreshQuantity=NaN, leavesQuantity=NaN, "
     "matchedQuantity=NaN}",
     "ENUM1", "nuam", 1700000006000LL, 0, 2LL, 1700000006001LL, 1, 0, "NEW", 0LL, 0LL, 0LL, 11.5, 21.0, NaN_, 0LL, 66,
     "SELL", "REGIONAL", 0LL, NaN_, NaN_, "M1", 1, 0, 0, 0, "C1", nullptr, nullptr, nullptr, "REST_OF_DAY", 101, 0,
     NaN_, 0, "LIMIT", 1001.25, NaN_, NaN_, NaN_, NaN_},
    {"enum2",
     "NuamOrder{ENUM2, eventTime=20231114-221326.000, source=DEFAULT, eventFlags=0x0, index=0x3, "
     "time=20231114-221326.002, sequence=2, timeNanoPart=0, action=REPLACE, actionTime=0, orderId=0, "
     "auxOrderId=0, price=12.5, size=22.0, executedSize=NaN, count=0, exchange=C, side=BUY, "
     "scope=AGGREGATE, tradeId=0, tradePrice=NaN, tradeSize=NaN, marketMaker='M2', actorId=2, "
     "participantId=0, submitterId=0, onBehalfOfSubmitterId=0, clientOrderId='C2', "
     "customerAccount='null', customerInfo='null', timeInForce=GTC, timeInForceData=102, "
     "triggerOrderBookId=0, triggerPrice=NaN, triggerSessionType=0, orderType=MARKET, "
     "orderQuantity=2001.25, displayQuantity=NaN, refreshQuantity=NaN, leavesQuantity=NaN, "
     "matchedQuantity=NaN}",
     "ENUM2", "DEFAULT", 1700000006000LL, 0, 3LL, 1700000006002LL, 2, 0, "REPLACE", 0LL, 0LL, 0LL, 12.5, 22.0, NaN_,
     0LL, 67, "BUY", "AGGREGATE", 0LL, NaN_, NaN_, "M2", 2, 0, 0, 0, "C2", nullptr, nullptr, nullptr, "GTC", 102, 0,
     NaN_, 0, "MARKET", 2001.25, NaN_, NaN_, NaN_, NaN_},
    {"enum3",
     "NuamOrder{ENUM3, eventTime=20231114-221326.000, source=NUAM, eventFlags=0x0, "
     "index=0x4e55414d00000004, time=20231114-221326.003, sequence=3, timeNanoPart=0, action=MODIFY, "
     "actionTime=0, orderId=0, auxOrderId=0, price=13.5, size=23.0, executedSize=NaN, count=0, "
     "exchange=D, side=SELL, scope=ORDER, tradeId=0, tradePrice=NaN, tradeSize=NaN, marketMaker='M3', "
     "actorId=3, participantId=0, submitterId=0, onBehalfOfSubmitterId=0, clientOrderId='C3', "
     "customerAccount='null', customerInfo='null', timeInForce=IMMEDIATE_OR_CANCEL, timeInForceData=103, "
     "triggerOrderBookId=0, triggerPrice=NaN, triggerSessionType=0, orderType=MARKET_TO_LIMIT, "
     "orderQuantity=3001.25, displayQuantity=NaN, refreshQuantity=NaN, leavesQuantity=NaN, "
     "matchedQuantity=NaN}",
     "ENUM3", "NUAM", 1700000006000LL, 0, 4LL, 1700000006003LL, 3, 0, "MODIFY", 0LL, 0LL, 0LL, 13.5, 23.0, NaN_, 0LL,
     68, "SELL", "ORDER", 0LL, NaN_, NaN_, "M3", 3, 0, 0, 0, "C3", nullptr, nullptr, nullptr, "IMMEDIATE_OR_CANCEL",
     103, 0, NaN_, 0, "MARKET_TO_LIMIT", 3001.25, NaN_, NaN_, NaN_, NaN_},
    {"enum4",
     "NuamOrder{ENUM4, eventTime=20231114-221326.000, source=nuam, eventFlags=0x0, "
     "index=0x6e75616d00000005, time=20231114-221326.004, sequence=4, timeNanoPart=0, action=DELETE, "
     "actionTime=0, orderId=0, auxOrderId=0, price=14.5, size=24.0, executedSize=NaN, count=0, "
     "exchange=E, side=BUY, scope=COMPOSITE, tradeId=0, tradePrice=NaN, tradeSize=NaN, marketMaker='M4', "
     "actorId=4, participantId=0, submitterId=0, onBehalfOfSubmitterId=0, clientOrderId='C4', "
     "customerAccount='null', customerInfo='null', timeInForce=FILL_OR_KILL, timeInForceData=104, "
     "triggerOrderBookId=0, triggerPrice=NaN, triggerSessionType=0, orderType=BEST_ORDER, "
     "orderQuantity=4001.25, displayQuantity=NaN, refreshQuantity=NaN, leavesQuantity=NaN, "
     "matchedQuantity=NaN}",
     "ENUM4", "nuam", 1700000006000LL, 0, 5LL, 1700000006004LL, 4, 0, "DELETE", 0LL, 0LL, 0LL, 14.5, 24.0, NaN_, 0LL,
     69, "BUY", "COMPOSITE", 0LL, NaN_, NaN_, "M4", 4, 0, 0, 0, "C4", nullptr, nullptr, nullptr, "FILL_OR_KILL", 104,
     0, NaN_, 0, "BEST_ORDER", 4001.25, NaN_, NaN_, NaN_, NaN_},
    {"enum5",
     "NuamOrder{ENUM5, eventTime=20231114-221326.000, source=DEFAULT, eventFlags=0x0, index=0x6, "
     "time=20231114-221326.005, sequence=5, timeNanoPart=0, action=PARTIAL, actionTime=0, orderId=0, "
     "auxOrderId=0, price=15.5, size=25.0, executedSize=NaN, count=0, exchange=F, side=SELL, "
     "scope=REGIONAL, tradeId=0, tradePrice=NaN, tradeSize=NaN, marketMaker='M5', actorId=5, "
     "participantId=0, submitterId=0, onBehalfOfSubmitterId=0, clientOrderId='C5', "
     "customerAccount='null', customerInfo='null', timeInForce=GOOD_TIL_SESSION, timeInForceData=105, "
     "triggerOrderBookId=0, triggerPrice=NaN, triggerSessionType=0, orderType=IMBALANCE, "
     "orderQuantity=5001.25, displayQuantity=NaN, refreshQuantity=NaN, leavesQuantity=NaN, "
     "matchedQuantity=NaN}",
     "ENUM5", "DEFAULT", 1700000006000LL, 0, 6LL, 1700000006005LL, 5, 0, "PARTIAL", 0LL, 0LL, 0LL, 15.5, 25.0, NaN_,
     0LL, 70, "SELL", "REGIONAL", 0LL, NaN_, NaN_, "M5", 5, 0, 0, 0, "C5", nullptr, nullptr, nullptr,
     "GOOD_TIL_SESSION", 105, 0, NaN_, 0, "IMBALANCE", 5001.25, NaN_, NaN_, NaN_, NaN_},
    {"enum6",
     "NuamOrder{ENUM6, eventTime=20231114-221326.000, source=NUAM, eventFlags=0x0, "
     "index=0x4e55414d00000007, time=20231114-221326.006, sequence=6, timeNanoPart=0, action=EXECUTE, "
     "actionTime=0, orderId=0, auxOrderId=0, price=16.5, size=26.0, executedSize=NaN, count=0, "
     "exchange=G, side=BUY, scope=AGGREGATE, tradeId=0, tradePrice=NaN, tradeSize=NaN, marketMaker='M6', "
     "actorId=6, participantId=0, submitterId=0, onBehalfOfSubmitterId=0, clientOrderId='C6', "
     "customerAccount='null', customerInfo='null', timeInForce=NUMBER_OF_DAYS, timeInForceData=106, "
     "triggerOrderBookId=0, triggerPrice=NaN, triggerSessionType=0, orderType=UNDEFINED, "
     "orderQuantity=6001.25, displayQuantity=NaN, refreshQuantity=NaN, leavesQuantity=NaN, "
     "matchedQuantity=NaN}",
     "ENUM6", "NUAM", 1700000006000LL, 0, 7LL, 1700000006006LL, 6, 0, "EXECUTE", 0LL, 0LL, 0LL, 16.5, 26.0, NaN_, 0LL,
     71, "BUY", "AGGREGATE", 0LL, NaN_, NaN_, "M6", 6, 0, 0, 0, "C6", nullptr, nullptr, nullptr, "NUMBER_OF_DAYS", 106,
     0, NaN_, 0, "UNDEFINED", 6001.25, NaN_, NaN_, NaN_, NaN_},
};

// The field values of the NuamTrade cases and what Java toString() prints for them (with the time zone UTC).
struct NuamTradeCase {
    const char *name;
    const char *str;
    const char *symbol; // nullptr: the Java null symbol
    std::int64_t eventTime;
    std::int64_t time;
    std::int32_t timeNanoPart;
    std::int32_t sequence;
    std::int32_t exchangeCode;
    std::int64_t tradeId;
    double price;
    double change;
    double size;
    std::int32_t dayId;
    double dayVolume;
    double dayTurnover;
    const char *direction;
    bool extendedTradingHours;
    std::int64_t tradeStatTime;
    double lastSignificantPrice;
    double lastPriceForAll;
    std::int32_t numberOfTrades;
    double vwap;
};

inline const NuamTradeCase NUAM_TRADE[] = {
    {"default",
     "NuamTrade{null, eventTime=0, time=0, timeNanoPart=0, sequence=0, exchange=\\0, tradeId=0, price=NaN, "
     "change=NaN, size=NaN, day=19700101, dayVolume=NaN, dayTurnover=NaN, direction=UNDEFINED, ETH=false, "
     "tradeStatTime=0, lastSignificantPrice=NaN, lastPriceForAll=NaN, numberOfTrades=0, vwap=NaN}",
     nullptr, 0LL, 0LL, 0, 0, 0, 0LL, NaN_, NaN_, NaN_, 0, NaN_, NaN_, "UNDEFINED", false, 0LL, NaN_, NaN_, 0, NaN_},
    {"full",
     "NuamTrade{AAPL, eventTime=20231114-221321.456, time=20231114-221320.123, timeNanoPart=123456, "
     "sequence=42, exchange=Q, tradeId=0, price=150.25, change=1.25, size=100.5, day=20231114, "
     "dayVolume=123456.5, dayTurnover=9876543.25, direction=UP, ETH=true, "
     "tradeStatTime=20231114-221320.999, lastSignificantPrice=150.5, lastPriceForAll=150.25, "
     "numberOfTrades=321, vwap=149.875}",
     "AAPL", 1700000001456LL, 1700000000123LL, 123456, 42, 81, 0LL, 150.25, 1.25, 100.5, 19675, 123456.5, 9876543.25,
     "UP", true, 1700000000999LL, 150.5, 150.25, 321, 149.875},
    {"nan",
     "NuamTrade{IBM, eventTime=20231114-221322.000, time=20231114-221321.000, timeNanoPart=0, sequence=1, "
     "exchange=A, tradeId=0, price=NaN, change=NaN, size=NaN, day=19700101, dayVolume=NaN, "
     "dayTurnover=NaN, direction=UNDEFINED, ETH=false, tradeStatTime=0, lastSignificantPrice=NaN, "
     "lastPriceForAll=NaN, numberOfTrades=0, vwap=NaN}",
     "IBM", 1700000002000LL, 1700000001000LL, 0, 1, 65, 0LL, NaN_, NaN_, NaN_, 0, NaN_, NaN_, "UNDEFINED", false, 0LL,
     NaN_, NaN_, 0, NaN_},
    {"outside",
     "NuamTrade{MSFT, eventTime=0, time=20231114-221323.999, timeNanoPart=0, sequence=0, exchange=\\0, "
     "tradeId=0, price=0.001, change=NaN, size=9999999.5, day=19700101, dayVolume=NaN, dayTurnover=NaN, "
     "direction=UNDEFINED, ETH=false, tradeStatTime=20231114-221324.000, lastSignificantPrice=1.0E7, "
     "lastPriceForAll=1.0E-5, numberOfTrades=1, vwap=1.0E21}",
     "MSFT", 0LL, 1700000003999LL, 0, 0, 0, 0LL, 0.001, NaN_, 9999999.5, 0, NaN_, NaN_, "UNDEFINED", false,
     1700000004000LL, 10000000.0, 1e-05, 1, 1e+21},
    {"outside2",
     "NuamTrade{ES, eventTime=0, time=0, timeNanoPart=0, sequence=0, exchange=\\0, tradeId=0, "
     "price=4500.25, change=NaN, size=NaN, day=19700101, dayVolume=NaN, dayTurnover=NaN, "
     "direction=UNDEFINED, ETH=false, tradeStatTime=0, lastSignificantPrice=-0.0, "
     "lastPriceForAll=-1.5E10, numberOfTrades=2, vwap=Infinity}",
     "ES", 0LL, 0LL, 0, 0, 0, 0LL, 4500.25, NaN_, NaN_, 0, NaN_, NaN_, "UNDEFINED", false, 0LL, -0.0, -15000000000.0,
     2, INF_},
    {"extreme",
     "NuamTrade{NQ, eventTime=21000101-000000.123, time=20380119-031407.123, timeNanoPart=999999, "
     "sequence=4194303, exchange=\\u007f, tradeId=0, price=1.5, change=-0.5, size=2.5, day=19691231, "
     "dayVolume=3.5, dayTurnover=4.5, direction=ZERO_UP, ETH=true, tradeStatTime=21000101-000000.456, "
     "lastSignificantPrice=-Infinity, lastPriceForAll=NaN, numberOfTrades=2147483647, vwap=-Infinity}",
     "NQ", 4102444800123LL, 4102444800123LL, 999999, 4194303, 127, 0LL, 1.5, -0.5, 2.5, -1, 3.5, 4.5, "ZERO_UP", true,
     4102444800456LL, -INF_, NaN_, 2147483647, -INF_},
    {"extreme2",
     "NuamTrade{RTY, eventTime=0, time=19700101-000000.001, timeNanoPart=0, sequence=0, exchange=\\0, "
     "tradeId=0, price=NaN, change=NaN, size=NaN, day=22431017, dayVolume=NaN, dayTurnover=NaN, "
     "direction=UNDEFINED, ETH=false, tradeStatTime=19700101-000000.001, lastSignificantPrice=NaN, "
     "lastPriceForAll=NaN, numberOfTrades=-2147483648, vwap=NaN}",
     "RTY", 0LL, 1LL, 0, 0, 0, 0LL, NaN_, NaN_, NaN_, 100000, NaN_, NaN_, "UNDEFINED", false, 1LL, NaN_, NaN_,
     (-2147483647 - 1), NaN_},
    {"dir0",
     "NuamTrade{DIR0, eventTime=20231114-221326.000, time=20231114-221326.000, timeNanoPart=0, "
     "sequence=0, exchange=A, tradeId=0, price=10.5, change=-3.5, size=20.0, day=20220108, "
     "dayVolume=30.0, dayTurnover=40.5, direction=UNDEFINED, ETH=true, tradeStatTime=20231114-221327.000, "
     "lastSignificantPrice=50.0, lastPriceForAll=60.0, numberOfTrades=70, vwap=80.25}",
     "DIR0", 1700000006000LL, 1700000006000LL, 0, 0, 65, 0LL, 10.5, -3.5, 20.0, 19000, 30.0, 40.5, "UNDEFINED", true,
     1700000007000LL, 50.0, 60.0, 70, 80.25},
    {"dir1",
     "NuamTrade{DIR1, eventTime=20231114-221326.000, time=20231114-221326.001, timeNanoPart=0, "
     "sequence=1, exchange=B, tradeId=0, price=11.5, change=-2.5, size=21.0, day=20220109, "
     "dayVolume=31.0, dayTurnover=41.5, direction=DOWN, ETH=false, tradeStatTime=20231114-221327.001, "
     "lastSignificantPrice=51.0, lastPriceForAll=61.0, numberOfTrades=71, vwap=81.25}",
     "DIR1", 1700000006000LL, 1700000006001LL, 0, 1, 66, 0LL, 11.5, -2.5, 21.0, 19001, 31.0, 41.5, "DOWN", false,
     1700000007001LL, 51.0, 61.0, 71, 81.25},
    {"dir2",
     "NuamTrade{DIR2, eventTime=20231114-221326.000, time=20231114-221326.002, timeNanoPart=0, "
     "sequence=2, exchange=C, tradeId=0, price=12.5, change=-1.5, size=22.0, day=20220110, "
     "dayVolume=32.0, dayTurnover=42.5, direction=ZERO_DOWN, ETH=true, tradeStatTime=20231114-221327.002, "
     "lastSignificantPrice=52.0, lastPriceForAll=62.0, numberOfTrades=72, vwap=82.25}",
     "DIR2", 1700000006000LL, 1700000006002LL, 0, 2, 67, 0LL, 12.5, -1.5, 22.0, 19002, 32.0, 42.5, "ZERO_DOWN", true,
     1700000007002LL, 52.0, 62.0, 72, 82.25},
    {"dir3",
     "NuamTrade{DIR3, eventTime=20231114-221326.000, time=20231114-221326.003, timeNanoPart=0, "
     "sequence=3, exchange=D, tradeId=0, price=13.5, change=0.5, size=23.0, day=20220111, dayVolume=33.0, "
     "dayTurnover=43.5, direction=ZERO, ETH=false, tradeStatTime=20231114-221327.003, "
     "lastSignificantPrice=53.0, lastPriceForAll=63.0, numberOfTrades=73, vwap=83.25}",
     "DIR3", 1700000006000LL, 1700000006003LL, 0, 3, 68, 0LL, 13.5, 0.5, 23.0, 19003, 33.0, 43.5, "ZERO", false,
     1700000007003LL, 53.0, 63.0, 73, 83.25},
    {"dir4",
     "NuamTrade{DIR4, eventTime=20231114-221326.000, time=20231114-221326.004, timeNanoPart=0, "
     "sequence=4, exchange=E, tradeId=0, price=14.5, change=1.5, size=24.0, day=20220112, dayVolume=34.0, "
     "dayTurnover=44.5, direction=ZERO_UP, ETH=true, tradeStatTime=20231114-221327.004, "
     "lastSignificantPrice=54.0, lastPriceForAll=64.0, numberOfTrades=74, vwap=84.25}",
     "DIR4", 1700000006000LL, 1700000006004LL, 0, 4, 69, 0LL, 14.5, 1.5, 24.0, 19004, 34.0, 44.5, "ZERO_UP", true,
     1700000007004LL, 54.0, 64.0, 74, 84.25},
    {"dir5",
     "NuamTrade{DIR5, eventTime=20231114-221326.000, time=20231114-221326.005, timeNanoPart=0, "
     "sequence=5, exchange=F, tradeId=0, price=15.5, change=2.5, size=25.0, day=20220113, dayVolume=35.0, "
     "dayTurnover=45.5, direction=UP, ETH=false, tradeStatTime=20231114-221327.005, "
     "lastSignificantPrice=55.0, lastPriceForAll=65.0, numberOfTrades=75, vwap=85.25}",
     "DIR5", 1700000006000LL, 1700000006005LL, 0, 5, 70, 0LL, 15.5, 2.5, 25.0, 19005, 35.0, 45.5, "UP", false,
     1700000007005LL, 55.0, 65.0, 75, 85.25},
};

// The field values of the NuamTimeAndSale cases and what Java toString() prints for them (with the time zone UTC).
struct NuamTimeAndSaleCase {
    const char *name;
    const char *str;
    const char *symbol; // nullptr: the Java null symbol
    std::int64_t eventTime;
    std::int32_t eventFlags;
    std::int64_t time;
    std::int32_t timeNanoPart;
    std::int32_t sequence;
    std::int32_t exchangeCode;
    std::int64_t tradeId;
    double price;
    double size;
    double bidPrice;
    double askPrice;
    const char *exchangeSaleConditions; // nullptr: the Java null
    std::int32_t tradeThroughExempt;
    const char *aggressorSide;
    bool spreadLeg;
    bool extendedTradingHours;
    bool validTick;
    const char *type;
    const char *buyer; // nullptr: the Java null
    const char *seller; // nullptr: the Java null
    std::int64_t matchId;
};

inline const NuamTimeAndSaleCase NUAM_TIME_AND_SALE[] = {
    {"default",
     "NuamTimeAndSale{null, eventTime=0, eventFlags=0x0, time=0, timeNanoPart=0, sequence=0, exchange=\\0, "
     "tradeId=0, price=NaN, size=NaN, bid=NaN, ask=NaN, ESC='null', TTE=\\0, side=UNDEFINED, spread=false, "
     "ETH=false, validTick=false, type=NEW, matchId=0}",
     nullptr, 0LL, 0, 0LL, 0, 0, 0, 0LL, NaN_, NaN_, NaN_, NaN_, nullptr, 0, "UNDEFINED", false, false, false, "NEW",
     nullptr, nullptr, 0LL},
    {"full",
     "NuamTimeAndSale{AAPL, eventTime=20231114-221321.456, eventFlags=0x3, time=20231114-221320.123, "
     "timeNanoPart=123456, sequence=42, exchange=T, tradeId=0, price=150.25, size=100.5, bid=150.0, "
     "ask=150.5, ESC='TI', TTE=X, side=BUY, spread=true, ETH=true, validTick=true, type=CORRECTION, "
     "buyer='BUYER-1', seller='SELLER-2', matchId=1234567890123}",
     "AAPL", 1700000001456LL, 3, 1700000000123LL, 123456, 42, 84, 0LL, 150.25, 100.5, 150.0, 150.5, "TI", 88, "BUY",
     true, true, true, "CORRECTION", "BUYER-1", "SELLER-2", 1234567890123LL},
    {"nanNull",
     "NuamTimeAndSale{IBM, eventTime=20231114-221322.000, eventFlags=0x0, time=20231114-221321.000, "
     "timeNanoPart=0, sequence=1, exchange=A, tradeId=0, price=NaN, size=NaN, bid=NaN, ask=NaN, "
     "ESC='null', TTE=\\0, side=UNDEFINED, spread=false, ETH=false, validTick=false, type=NEW, matchId=0}",
     "IBM", 1700000002000LL, 0, 1700000001000LL, 0, 1, 65, 0LL, NaN_, NaN_, NaN_, NaN_, nullptr, 0, "UNDEFINED", false,
     false, false, "NEW", nullptr, nullptr, 0LL},
    {"empty",
     "NuamTimeAndSale{MSFT, eventTime=0, eventFlags=0x0, time=20231114-221323.999, timeNanoPart=0, "
     "sequence=0, exchange=\\0, tradeId=0, price=0.001, size=9999999.5, bid=NaN, ask=NaN, ESC='', TTE=\\0, "
     "side=UNDEFINED, spread=false, ETH=false, validTick=false, type=NEW, buyer='', seller='', "
     "matchId=1}",
     "MSFT", 0LL, 0, 1700000003999LL, 0, 0, 0, 0LL, 0.001, 9999999.5, NaN_, NaN_, "", 0, "UNDEFINED", false, false,
     false, "NEW", "", "", 1LL},
    {"extreme",
     "NuamTimeAndSale{NQ, eventTime=21000101-000000.123, eventFlags=0xffffffff, time=20380119-031407.123, "
     "timeNanoPart=999999, sequence=4194303, exchange=\\u007f, tradeId=0, price=1.5, size=2.5, bid=3.5, "
     "ask=4.5, ESC='ASCII', TTE=\\u007f, side=SELL, spread=false, ETH=false, validTick=false, type=NEW, "
     "buyer='a'b=c', seller=',;=#', matchId=-9223372036854775808}",
     "NQ", 4102444800123LL, -1, 4102444800123LL, 999999, 4194303, 127, 0LL, 1.5, 2.5, 3.5, 4.5, "ASCII", 127, "SELL",
     false, false, false, "NEW", "a'b=c", ",;=#", (-9223372036854775807LL - 1)},
    {"extreme2",
     "NuamTimeAndSale{RTY, eventTime=0, eventFlags=0x0, time=19700101-000000.001, timeNanoPart=0, "
     "sequence=0, exchange=\\0, tradeId=0, price=NaN, size=NaN, bid=NaN, ask=NaN, ESC='\"q\"', TTE=\\0, "
     "side=UNDEFINED, spread=false, ETH=false, validTick=false, type=NEW, buyer='back\\slash', "
     "matchId=9223372036854775807}",
     "RTY", 0LL, 0, 1LL, 0, 0, 0, 0LL, NaN_, NaN_, NaN_, NaN_, "\"q\"", 0, "UNDEFINED", false, false, false, "NEW",
     "back\\slash", nullptr, 9223372036854775807LL},
    {"type0",
     "NuamTimeAndSale{TYPE0, eventTime=20231114-221326.000, eventFlags=0x0, time=20231114-221326.000, "
     "timeNanoPart=0, sequence=0, exchange=A, tradeId=0, price=10.5, size=20.0, bid=30.25, ask=40.75, "
     "ESC='C0', TTE=F, side=UNDEFINED, spread=true, ETH=false, validTick=true, type=NEW, buyer='B0', "
     "seller='S0', matchId=1000}",
     "TYPE0", 1700000006000LL, 0, 1700000006000LL, 0, 0, 65, 0LL, 10.5, 20.0, 30.25, 40.75, "C0", 70, "UNDEFINED",
     true, false, true, "NEW", "B0", "S0", 1000LL},
    {"type1",
     "NuamTimeAndSale{TYPE1, eventTime=20231114-221326.000, eventFlags=0x1, time=20231114-221326.001, "
     "timeNanoPart=0, sequence=1, exchange=B, tradeId=0, price=11.5, size=21.0, bid=31.25, ask=41.75, "
     "ESC='C1', TTE=G, side=BUY, spread=false, ETH=true, validTick=false, type=CORRECTION, buyer='B1', "
     "seller='S1', matchId=1001}",
     "TYPE1", 1700000006000LL, 1, 1700000006001LL, 0, 1, 66, 0LL, 11.5, 21.0, 31.25, 41.75, "C1", 71, "BUY", false,
     true, false, "CORRECTION", "B1", "S1", 1001LL},
    {"type2",
     "NuamTimeAndSale{TYPE2, eventTime=20231114-221326.000, eventFlags=0x2, time=20231114-221326.002, "
     "timeNanoPart=0, sequence=2, exchange=C, tradeId=0, price=12.5, size=22.0, bid=32.25, ask=42.75, "
     "ESC='C2', TTE=H, side=SELL, spread=true, ETH=false, validTick=true, type=CANCEL, buyer='B2', "
     "seller='S2', matchId=1002}",
     "TYPE2", 1700000006000LL, 2, 1700000006002LL, 0, 2, 67, 0LL, 12.5, 22.0, 32.25, 42.75, "C2", 72, "SELL", true,
     false, true, "CANCEL", "B2", "S2", 1002LL},
};

// The field values of the MarketMaker cases and what Java toString() prints for them (with the time zone UTC).
struct MarketMakerCase {
    const char *name;
    const char *str;
    const char *symbol; // nullptr: the Java null symbol
    std::int64_t eventTime;
    std::int32_t eventFlags;
    std::int32_t exchangeCode;
    const char *marketMaker; // nullptr: the Java null
    std::int64_t index;
    std::int64_t bidTime;
    double bidPrice;
    double bidSize;
    std::int64_t bidCount;
    std::int64_t askTime;
    double askPrice;
    double askSize;
    std::int64_t askCount;
};

inline const MarketMakerCase MARKET_MAKER[] = {
    {"default",
     "MarketMaker{null, eventTime=0, source=DEFAULT, eventFlags=0x0, index=0x0, time=0, exchange=\\0, "
     "marketMaker='null', bidTime=0, bidPrice=NaN, bidSize=NaN, bidCount=0, askTime=0, askPrice=NaN, "
     "askSize=NaN, askCount=0}",
     nullptr, 0LL, 0, 0, nullptr, 0LL, 0LL, NaN_, NaN_, 0LL, 0LL, NaN_, NaN_, 0LL},
    {"full",
     "MarketMaker{AAPL, eventTime=20231114-221321.456, source=DEFAULT, eventFlags=0x3, "
     "index=0x514e534451, time=20231114-221321.123, exchange=Q, marketMaker='NSDQ', "
     "bidTime=20231114-221320.000, bidPrice=150.25, bidSize=100.5, bidCount=7, "
     "askTime=20231114-221321.123, askPrice=150.5, askSize=200.0, askCount=9}",
     "AAPL", 1700000001456LL, 3, 81, "NSDQ", 349206430801LL, 1700000000000LL, 150.25, 100.5, 7LL, 1700000001123LL,
     150.5, 200.0, 9LL},
    {"bidLater",
     "MarketMaker{IBM, eventTime=20231114-221322.000, source=DEFAULT, eventFlags=0x0, index=0x4141524358, "
     "time=20231114-221325.999, exchange=A, marketMaker='ARCX', bidTime=20231114-221325.999, "
     "bidPrice=0.001, bidSize=9999999.5, bidCount=1, askTime=20231114-221324.000, askPrice=1.0E7, "
     "askSize=1.0E-5, askCount=2}",
     "IBM", 1700000002000LL, 0, 65, "ARCX", 280268784472LL, 1700000005999LL, 0.001, 9999999.5, 1LL, 1700000004000LL,
     10000000.0, 1e-05, 2LL},
    {"truncated",
     "MarketMaker{MSFT, eventTime=0, source=DEFAULT, eventFlags=0x0, index=0x5a43444546, time=0, "
     "exchange=Z, marketMaker='CDEF', bidTime=0, bidPrice=NaN, bidSize=NaN, bidCount=0, askTime=0, "
     "askPrice=NaN, askSize=NaN, askCount=0}",
     "MSFT", 0LL, 0, 90, "ABCDEF", 387675604294LL, 0LL, NaN_, NaN_, 0LL, 0LL, NaN_, NaN_, 0LL},
    {"eightChars",
     "MarketMaker{MSFT, eventTime=0, source=DEFAULT, eventFlags=0x0, index=0x5a45464748, time=0, "
     "exchange=Z, marketMaker='EFGH', bidTime=0, bidPrice=NaN, bidSize=NaN, bidCount=0, askTime=0, "
     "askPrice=NaN, askSize=NaN, askCount=0}",
     "MSFT", 0LL, 0, 90, "ABCDEFGH", 387709290312LL, 0LL, NaN_, NaN_, 0LL, 0LL, NaN_, NaN_, 0LL},
    {"latin1",
     "MarketMaker{ES, eventTime=0, source=DEFAULT, eventFlags=0x0, index=0xdc004ddcff, time=0, "
     "exchange=\\u00dc, marketMaker='M\303\234\303\277', bidTime=0, bidPrice=NaN, bidSize=NaN, bidCount=0, askTime=0, "
     "askPrice=NaN, askSize=NaN, askCount=0}",
     "ES", 0LL, 0, 220, "M\303\234\303\277", 944897907967LL, 0LL, NaN_, NaN_, 0LL, 0LL, NaN_, NaN_, 0LL},
    {"highExchange",
     "MarketMaker{NQ, eventTime=0, source=DEFAULT, eventFlags=0x0, index=0xffff0000005a, time=0, "
     "exchange=\\uffff, marketMaker='Z', bidTime=0, bidPrice=-0.0, bidSize=NaN, bidCount=0, askTime=0, "
     "askPrice=Infinity, askSize=NaN, askCount=0}",
     "NQ", 0LL, 0, 65535, "Z", 281470681743450LL, 0LL, -0.0, NaN_, 0LL, 0LL, INF_, NaN_, 0LL},
    {"empty",
     "MarketMaker{RTY, eventTime=0, source=DEFAULT, eventFlags=0x0, index=0x0, time=19691231-235959.999, "
     "exchange=\\0, marketMaker='null', bidTime=19160218-014639.877, bidPrice=NaN, bidSize=NaN, "
     "bidCount=0, askTime=19691231-235959.999, askPrice=NaN, askSize=NaN, askCount=0}",
     "RTY", 0LL, 0, 0, "", 0LL, -1700000000123LL, NaN_, NaN_, 0LL, -1LL, NaN_, NaN_, 0LL},
    {"extreme",
     "MarketMaker{YM, eventTime=21000101-000000.123, source=DEFAULT, eventFlags=0xffffffff, "
     "index=0x7e27623d63, time=21000101-000000.123, exchange=~, marketMaker=''b=c', "
     "bidTime=21000101-000000.123, bidPrice=1.0E21, bidSize=-1.5E10, bidCount=9223372036854775807, "
     "askTime=21000101-000000.000, askPrice=-Infinity, askSize=NaN, askCount=-9223372036854775808}",
     "YM", 4102444800123LL, -1, 126, "a'b=c", 541826628963LL, 4102444800123LL, 1e+21, -15000000000.0,
     9223372036854775807LL, 4102444800000LL, -INF_, NaN_, (-9223372036854775807LL - 1)},
    {"nullSymbol",
     "MarketMaker{null, eventTime=0, source=DEFAULT, eventFlags=0x0, index=0x4200000058, time=0, "
     "exchange=B, marketMaker='X', bidTime=0, bidPrice=1.5, bidSize=NaN, bidCount=0, askTime=0, "
     "askPrice=2.5, askSize=NaN, askCount=0}",
     nullptr, 0LL, 0, 66, "X", 283467841624LL, 0LL, 1.5, NaN_, 0LL, 0LL, 2.5, NaN_, 0LL},
};

// The field values of the TextConfiguration cases and what Java toString() prints for them (with the time zone UTC).
struct TextConfigurationCase {
    const char *name;
    const char *str;
    const char *symbol; // nullptr: the Java null symbol
    std::int64_t eventTime;
    std::int64_t time;
    std::int32_t sequence;
    std::int32_t version;
    const char *text; // nullptr: the Java null
};

inline const TextConfigurationCase TEXT_CONFIGURATION[] = {
    {"default", "TextConfiguration{null, eventTime=0, time=0, sequence=0, version=0, text='null'}", nullptr, 0LL, 0LL,
     0, 0, nullptr},
    {"full",
     "TextConfiguration{AAPL, eventTime=20231114-221321.456, time=20231114-221320.123, sequence=42, "
     "version=7, text='{\"key\":\"value\"}'}",
     "AAPL", 1700000001456LL, 1700000000123LL, 42, 7, "{\"key\":\"value\"}"},
    {"nullText",
     "TextConfiguration{IBM, eventTime=20231114-221322.000, time=20231114-221321.000, sequence=1, "
     "version=1, text='null'}",
     "IBM", 1700000002000LL, 1700000001000LL, 1, 1, nullptr},
    {"emptyText", "TextConfiguration{MSFT, eventTime=0, time=20231114-221323.999, sequence=0, version=0, text=''}",
     "MSFT", 0LL, 1700000003999LL, 0, 0, ""},
    {"unicode",
     "TextConfiguration{ES, eventTime=0, time=20231114-221324.000, sequence=0, version=2, "
     "text='\303\234\342\202\254\360\237\230\200'}",
     "ES", 0LL, 1700000004000LL, 0, 2, "\303\234\342\202\254\360\237\230\200"},
    {"extreme",
     "TextConfiguration{NQ, eventTime=21000101-000000.123, time=20380119-031407.123, sequence=4194303, "
     "version=2147483647, text='a'b=c'}",
     "NQ", 4102444800123LL, 4102444800123LL, 4194303, 2147483647, "a'b=c"},
    {"negative",
     "TextConfiguration{RTY, eventTime=0, time=19160218-014639.877, sequence=0, version=-2147483648, "
     "text='back\\slash'}",
     "RTY", 0LL, -1700000000123LL, 0, (-2147483647 - 1), "back\\slash"},
    {"nullSymbol", "TextConfiguration{null, eventTime=0, time=19700101-000000.001, sequence=0, version=-1, text='x'}",
     nullptr, 0LL, 1LL, 0, -1, "x"},
};

} // namespace events_java_parity
