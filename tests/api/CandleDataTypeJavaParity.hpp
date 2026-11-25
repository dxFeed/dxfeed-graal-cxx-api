// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

// Expected values recorded from the Java API (QD 3.356): CandleDataType and the data type of CandleSymbol.
// Generated from the probe output, do not edit by hand.

#pragma once

namespace candle_data_type_java_parity {

struct ValueOfCase {
    const char *input;
    const char *constant; // The name of the constant that is returned, "-" for a new data type.
    bool isDefault;
};

inline const ValueOfCase VALUE_OF[] = {
    {"source", "SOURCE", false},
    {"tho", "THO", false},
    {"all", "ALL", false},
    {"rth", "RTH", false},
    {"eth", "ETH", false},
    {"", "UNDEFINED", true},
    {"SOURCE", "-", false},
    {"Tho", "-", false},
    {"THO", "-", false},
    {"xyz", "-", false},
    {" tho", "-", false},
    {"tho ", "-", false},
    {"s", "-", false},
    {"t", "-", false},
    {"a", "-", false},
    {"all_", "-", false},
    {"undefined", "-", false},
    {"-", "-", false},
    {"true", "-", false},
    {"false", "-", false},
    {"any", "-", false},
    {"dt", "-", false},
    {"a b", "-", false},
    {"{}", "-", false},
    {"=", "-", false},
    {",", "-", false},
};

struct SymbolCase {
    const char *input;
    const char *str;
    const char *base;
    const char *dataType;
    const char *constant;
    const char *period;
    const char *session;
};

inline const SymbolCase SYMBOL_VALUE_OF[] = {
    {"A", "A", "A", "", "UNDEFINED", "t", "false"},
    {"A{dt=}", "A", "A", "", "UNDEFINED", "t", "false"},
    {"A{dt=source}", "A{dt=source}", "A", "source", "SOURCE", "t", "false"},
    {"A{dt=SOURCE}", "A{dt=SOURCE}", "A", "SOURCE", "-", "t", "false"},
    {"A{dt=tho}", "A{dt=tho}", "A", "tho", "THO", "t", "false"},
    {"A{dt=all}", "A{dt=all}", "A", "all", "ALL", "t", "false"},
    {"A{dt=rth}", "A{dt=rth}", "A", "rth", "RTH", "t", "false"},
    {"A{dt=eth}", "A{dt=eth}", "A", "eth", "ETH", "t", "false"},
    {"A{dt=xyz}", "A{dt=xyz}", "A", "xyz", "-", "t", "false"},
    {"A{dt=Tho}", "A{dt=Tho}", "A", "Tho", "-", "t", "false"},
    {"A{=m,dt=rth}", "A{=m,dt=rth}", "A", "rth", "RTH", "m", "false"},
    {"A{dt=eth,=5m}", "A{dt=eth,=5m}", "A", "eth", "ETH", "5m", "false"},
    {"A{dt=all,tho=true}", "A{dt=all,tho=true}", "A", "all", "ALL", "t", "true"},
    {"A&Q{dt=tho,price=bid}", "A&Q{dt=tho,price=bid}", "A", "tho", "THO", "t", "false"},
    {"A{dt=,=m}", "A{=m}", "A", "", "UNDEFINED", "m", "false"},
    {"A{=m,dt=}", "A{=m}", "A", "", "UNDEFINED", "m", "false"},
    {"A{dt=}{=t}", "A{dt=}{=t}", "A{dt=}", "", "UNDEFINED", "t", "false"},
    {"A{=t}{dt=}", "A{=t}", "A", "", "UNDEFINED", "t", "false"},
    {"A{dt=x}{dt=y}", "A{dt=x}{dt=y}", "A{dt=x}", "y", "-", "t", "false"},
    {"A{dt=tho,pl=0.5,a=s}", "A{dt=tho,pl=0.5,a=s}", "A", "tho", "THO", "t", "false"},
    {"A{dT=tho}", "A{dT=tho}", "A", "", "UNDEFINED", "t", "false"},
    {"A{DT=tho}", "A{DT=tho}", "A", "", "UNDEFINED", "t", "false"},
    {"A{dt=a b}", "A{dt=a b}", "A", "a b", "-", "t", "false"},
    {"A{=d,dt=source,tho=false}", "A{=d,dt=source,tho=false}", "A", "source", "SOURCE", "d", "false"},
    {"A{dt=tho}{dt=tho}", "A{dt=tho}{dt=tho}", "A{dt=tho}", "tho", "THO", "t", "false"},
    {"{dt=tho}", "{dt=tho}", "", "tho", "THO", "t", "false"},
    {"A{dt=tho,dt=eth}", "A{dt=tho,dt=eth}", "A", "tho", "THO", "t", "false"},
};

struct WithDataTypeCase {
    const char *input;
    const char *dataType;
    const char *str;
    const char *resultDataType;
    const char *changed; // CandleDataType::changeAttributeForSymbol(input)
};

inline const WithDataTypeCase SYMBOL_WITH_DATA_TYPE[] = {
    {"A", "tho", "A{dt=tho}", "tho", "A{dt=tho}"},
    {"A", "", "A", "", "A"},
    {"A{dt=tho}", "", "A", "", "A"},
    {"A{dt=tho}", "eth", "A{dt=eth}", "eth", "A{dt=eth}"},
    {"A{=m}", "xyz", "A{=m,dt=xyz}", "xyz", "A{=m,dt=xyz}"},
    {"A{=m,dt=rth}", "", "A{=m}", "", "A{=m}"},
    {"A", "SOURCE", "A{dt=SOURCE}", "SOURCE", "A{dt=SOURCE}"},
    {"A&Q{=5m}", "all", "A&Q{=5m,dt=all}", "all", "A&Q{=5m,dt=all}"},
};

// CandleSymbol.valueOf("A", CandlePeriod.valueOf(5, CandleType.MINUTE), CandleDataType.RTH, CandleSession.REGULAR)
inline constexpr const char *SYMBOL_WITH_ATTRIBUTES = "A{=5m,dt=rth,tho=true}";

} // namespace candle_data_type_java_parity
