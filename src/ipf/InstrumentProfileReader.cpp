// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include <cstring>
#include <memory>
#include <utf8.h>
#include <utility>

#include <fmt/chrono.h>
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <fmt/std.h>

namespace dxfcpp {

InstrumentProfileReader::InstrumentProfileReader() noexcept : id_{Id<InstrumentProfileReader>::UNKNOWN}, handle_{} {
    handle_ = JavaObjectHandle<DXFeedSubscription>(runIsolatedOrElse(
        [](auto threadHandle) {
            return dxfg_InstrumentProfileReader_new(dxfcpp::bit_cast<graal_isolatethread_t *>(threadHandle));
        },
        nullptr));
}

std::vector<std::shared_ptr<InstrumentProfile>> fromGraalList(void *graalList) {
    using ListType = dxfg_instrument_profile_list;
    using SizeType = decltype(ListType::size);

    if (!graalList) {
        return {};
    }

    std::vector<std::shared_ptr<InstrumentProfile>> result{};

    auto list = dxfcpp::bit_cast<ListType *>(graalList);

    if (list->size > 0 && list->elements != nullptr) {
        for (SizeType elementIndex = 0; elementIndex < list->size; elementIndex++) {
            if (list->elements[elementIndex]) {
                result.emplace_back(
                    InstrumentProfile::fromGraal(dxfcpp::bit_cast<void *>(list->elements[elementIndex])));
            }
        }
    }

    return result;
}

std::vector<std::shared_ptr<InstrumentProfile>>
InstrumentProfileReader::readFromFile(const std::string &address) const noexcept {
    if (!handle_) {
        return {};
    }

    dxfg_instrument_profile_list *list = runIsolatedOrElse(
        [handle = dxfcpp::bit_cast<dxfg_instrument_profile_reader_t *>(handle_.get()),
         address = address](auto threadHandle) {
            return dxfg_InstrumentProfileReader_readFromFile(dxfcpp::bit_cast<graal_isolatethread_t *>(threadHandle),
                                                             handle, address.c_str());
        },
        nullptr);

    auto result = fromGraalList(dxfcpp::bit_cast<void *>(list));

    runIsolatedOrElse(
        [list](auto threadHandle) {
            return dxfg_CList_InstrumentProfile_release(dxfcpp::bit_cast<graal_isolatethread_t *>(threadHandle),
                                                        list) == 0;
        },
        false);

    return result;
}

std::vector<std::shared_ptr<InstrumentProfile>>
InstrumentProfileReader::readFromFile(const std::string &address, const std::string &user,
                                      const std::string &password) const noexcept {
    if (!handle_) {
        return {};
    }

    dxfg_instrument_profile_list *list = runIsolatedOrElse(
        [handle = dxfcpp::bit_cast<dxfg_instrument_profile_reader_t *>(handle_.get()), address = address, user = user,
         password = password](auto threadHandle) {
            return dxfg_InstrumentProfileReader_readFromFile2(dxfcpp::bit_cast<graal_isolatethread_t *>(threadHandle),
                                                              handle, address.c_str(), user.c_str(), password.c_str());
        },
        nullptr);

    auto result = fromGraalList(dxfcpp::bit_cast<void *>(list));

    runIsolatedOrElse(
        [list](auto threadHandle) {
            return dxfg_CList_InstrumentProfile_release(dxfcpp::bit_cast<graal_isolatethread_t *>(threadHandle),
                                                        list) == 0;
        },
        false);

    return result;
}

} // namespace dxfcpp