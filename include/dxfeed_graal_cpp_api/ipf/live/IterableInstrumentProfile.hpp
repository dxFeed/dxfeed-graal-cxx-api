// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../../internal/Common.hpp"
#include "../../internal/Id.hpp"
#include "../../internal/JavaObjectHandle.hpp"

#include "../../entity/SharedEntity.hpp"

DXFCPP_BEGIN_NAMESPACE

class InstrumentProfileCollector;

/**
 * A wrapper around Java Iterator for InstrumentProfile
 */
class DXFCPP_EXPORT IterableInstrumentProfile final {
    friend InstrumentProfileCollector;

    JavaObjectHandle<IterableInstrumentProfile> handle_;
    //dxfg_iterable_ip_t *iterable;

    explicit IterableInstrumentProfile(void* handle = nullptr) noexcept;

    static std::shared_ptr<IterableInstrumentProfile> create(void* handle = nullptr) noexcept;

  public:

    /**
     * Returns `true` if the iterable has more elements.
     *
     * @return `true` if the iteration has more elements
     */
    bool hasNext() const noexcept;

    /**
     * Returns the next element in the iteration.
     *
     * @return the next element in the iteration
     * @throw InvalidArgumentException
     * @throw JavaException "NoSuchElementException"
     */
    std::shared_ptr<InstrumentProfile> next() const;
};

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()