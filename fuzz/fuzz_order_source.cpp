// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

// Fuzz OrderSource::valueOf(name) / valueOf(id) and name<->id codec.
// Invariant: valueOf(valueOf(id).name()).id() == id for any id accepted by valueOf(id).
#include <dxfeed_graal_cpp_api/api.hpp>

#include <cstdint>
#include <cstring>
#include <exception>
#include <string>

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t *data, std::size_t size) {
    const std::string s(reinterpret_cast<const char *>(data), size);

    try {
        const auto &src = dxfcpp::OrderSource::valueOf(s);
        (void)src.toString();
        (void)dxfcpp::OrderSource::isSpecialSourceId(src.id());
    } catch (const std::exception &) {
    }

    if (size >= 4) {
        std::int32_t id;
        std::memcpy(&id, data, 4);
        try {
            const auto &src = dxfcpp::OrderSource::valueOf(id);
            const auto &again = dxfcpp::OrderSource::valueOf(src.name());
            if (again.id() != id && !dxfcpp::OrderSource::isSpecialSourceId(id)) {
                __builtin_trap();
            }
        } catch (const std::exception &) {
        }
    }

    return 0;
}
