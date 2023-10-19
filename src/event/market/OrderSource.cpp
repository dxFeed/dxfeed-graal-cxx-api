// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

namespace dxfcpp {

const OrderSource OrderSource::COMPOSITE_BID(1, "COMPOSITE_BID", 0);
const OrderSource OrderSource::COMPOSITE_ASK(2, "COMPOSITE_ASK", 0);
const OrderSource OrderSource::REGIONAL_BID(3, "REGIONAL_BID", 0);
const OrderSource OrderSource::REGIONAL_ASK(4, "REGIONAL_ASK", 0);
const OrderSource OrderSource::AGGREGATE_BID(5, "AGGREGATE_BID", 0);
const OrderSource OrderSource::AGGREGATE_ASK(6, "AGGREGATE_ASK", 0);
const OrderSource OrderSource::DEFAULT(0, "DEFAULT",
                                       PUB_ORDER | PUB_ANALYTIC_ORDER | PUB_SPREAD_ORDER | FULL_ORDER_BOOK);

const OrderSource OrderSource::NTV("NTV", PUB_ORDER | FULL_ORDER_BOOK);
const OrderSource OrderSource::ntv("ntv", PUB_ORDER);
const OrderSource OrderSource::NFX("NFX", PUB_ORDER);
const OrderSource OrderSource::ESPD("ESPD", PUB_ORDER);
const OrderSource OrderSource::XNFI("XNFI", PUB_ORDER);
const OrderSource OrderSource::ICE("ICE", PUB_ORDER);
const OrderSource OrderSource::ISE("ISE", PUB_ORDER | PUB_SPREAD_ORDER);
const OrderSource OrderSource::DEA("DEA", PUB_ORDER);
const OrderSource OrderSource::DEX("DEX", PUB_ORDER);
const OrderSource OrderSource::dex("dex", PUB_ORDER);
const OrderSource OrderSource::BYX("BYX", PUB_ORDER);
const OrderSource OrderSource::BZX("BZX", PUB_ORDER);
const OrderSource OrderSource::bzx("bzx", PUB_ORDER);
const OrderSource OrderSource::BATE("BATE", PUB_ORDER);
const OrderSource OrderSource::CHIX("CHIX", PUB_ORDER);
const OrderSource OrderSource::CEUX("CEUX", PUB_ORDER);
const OrderSource OrderSource::BXTR("BXTR", PUB_ORDER);
const OrderSource OrderSource::IST("IST", PUB_ORDER);
const OrderSource OrderSource::BI20("BI20", PUB_ORDER);
const OrderSource OrderSource::ABE("ABE", PUB_ORDER);
const OrderSource OrderSource::FAIR("FAIR", PUB_ORDER);
const OrderSource OrderSource::GLBX("GLBX", PUB_ORDER | PUB_ANALYTIC_ORDER);
const OrderSource OrderSource::glbx("glbx", PUB_ORDER);
const OrderSource OrderSource::ERIS("ERIS", PUB_ORDER);
const OrderSource OrderSource::XEUR("XEUR", PUB_ORDER);
const OrderSource OrderSource::xeur("xeur", PUB_ORDER);
const OrderSource OrderSource::CFE("CFE", PUB_ORDER);
const OrderSource OrderSource::C2OX("C2OX", PUB_ORDER);
const OrderSource OrderSource::SMFE("SMFE", PUB_ORDER);
const OrderSource OrderSource::smfe("smfe", PUB_ORDER);
const OrderSource OrderSource::iex("iex", PUB_ORDER);
const OrderSource OrderSource::MEMX("MEMX", PUB_ORDER);
const OrderSource OrderSource::memx("memx", PUB_ORDER);

const std::unordered_map<std::variant<std::int32_t, std::string>, std::reference_wrapper<const OrderSource>>
    OrderSource::PREDEFINED_SOURCES =
        [](std::initializer_list<std::reference_wrapper<const OrderSource>> orders) {
            std::unordered_map<std::variant<std::int32_t, std::string>, std::reference_wrapper<const OrderSource>>
                result{};

            for (auto &&o : orders) {
                result.emplace(o.get().id(), o);
                result.emplace(o.get().name(), o);
            }

            return result;
        }({COMPOSITE_BID, COMPOSITE_ASK, REGIONAL_BID, REGIONAL_ASK, AGGREGATE_BID, AGGREGATE_ASK, DEFAULT, NTV,
           ntv,           NFX,           ESPD,         XNFI,         ICE,           ISE,           DEA,     DEX,
           dex,           BYX,           BZX,          bzx,          BATE,          CHIX,          CEUX,    BXTR,
           IST,           BI20,          ABE,          FAIR,         GLBX,          glbx,          ERIS,    XEUR,
           xeur,          CFE,           C2OX,         SMFE,         smfe,          iex,           MEMX,    memx});

std::unordered_map<std::int32_t, OrderSource> OrderSource::USER_SOURCES_{};

void *OrderSource::toGraal() const noexcept {
    auto *graalSource = new (std::nothrow) dxfg_indexed_event_source_t{ORDER_SOURCE, id(), createCString(name())};

    return static_cast<void *>(graalSource);
}

} // namespace dxfcpp