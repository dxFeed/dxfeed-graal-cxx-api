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

const std::unordered_map<std::int32_t, std::reference_wrapper<const OrderSource>> OrderSource::INTERNAL_{
    {COMPOSITE_BID.id(), std::cref(COMPOSITE_BID)},
    {COMPOSITE_ASK.id(), std::cref(COMPOSITE_ASK)},
    {REGIONAL_BID.id(), std::cref(REGIONAL_BID)},
    {REGIONAL_ASK.id(), std::cref(REGIONAL_ASK)},
    {AGGREGATE_BID.id(), std::cref(AGGREGATE_BID)},
    {AGGREGATE_ASK.id(), std::cref(AGGREGATE_ASK)},
    {DEFAULT.id(), std::cref(DEFAULT)},

    {NTV.id(), std::cref(NTV)},
    {ntv.id(), std::cref(ntv)},
    {NFX.id(), std::cref(NFX)},
    {ESPD.id(), std::cref(ESPD)},
    {XNFI.id(), std::cref(XNFI)},
    {ICE.id(), std::cref(ICE)},
    {ISE.id(), std::cref(ISE)},
    {DEA.id(), std::cref(DEA)},
    {DEX.id(), std::cref(DEX)},
    {dex.id(), std::cref(dex)},
    {BYX.id(), std::cref(BYX)},
    {BZX.id(), std::cref(BZX)},
    {bzx.id(), std::cref(bzx)},
    {BATE.id(), std::cref(BATE)},
    {CHIX.id(), std::cref(CHIX)},
    {CEUX.id(), std::cref(CEUX)},
    {BXTR.id(), std::cref(BXTR)},
    {IST.id(), std::cref(IST)},
    {BI20.id(), std::cref(BI20)},
    {ABE.id(), std::cref(ABE)},
    {FAIR.id(), std::cref(FAIR)},
    {GLBX.id(), std::cref(GLBX)},
    {glbx.id(), std::cref(glbx)},
    {ERIS.id(), std::cref(ERIS)},
    {XEUR.id(), std::cref(XEUR)},
    {xeur.id(), std::cref(xeur)},
    {CFE.id(), std::cref(CFE)},
    {C2OX.id(), std::cref(C2OX)},
    {SMFE.id(), std::cref(SMFE)},
    {smfe.id(), std::cref(smfe)},
    {iex.id(), std::cref(iex)},
    {MEMX.id(), std::cref(MEMX)},
    {memx.id(), std::cref(memx)},
};

void *OrderSource::toGraal() const noexcept {
    auto *graalSource = new (std::nothrow) dxfg_indexed_event_source_t{ORDER_SOURCE, id(), createCString(name())};

    return dxfcpp::bit_cast<void *>(graalSource);
}

} // namespace dxfcpp