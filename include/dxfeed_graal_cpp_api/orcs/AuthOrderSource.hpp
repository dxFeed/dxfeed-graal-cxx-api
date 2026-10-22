// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../entity/SharedEntity.hpp"
#include "../event/market/OrderSource.hpp"
#include "../internal/JavaObjectHandle.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>

/**
 * \addtogroup dxfcpp_orcs
 * @{
 */

DXFCPP_BEGIN_NAMESPACE

/**
 * The order sources and the symbols for each order source available to the client of a PriceLevelService.
 * The sources and the symbols are filtered according to the client permissions.
 *
 * It is returned by PriceLevelService::getAuthOrderSource().
 */
struct DXFCPP_EXPORT AuthOrderSource final : RequireMakeShared<AuthOrderSource> {
    /// The alias to a type of shared pointer to the AuthOrderSource object
    using Ptr = std::shared_ptr<AuthOrderSource>;

    private:
    JavaObjectHandle<AuthOrderSource> handle_;

    public:
    explicit AuthOrderSource(RequireMakeShared<AuthOrderSource>::LockExternalConstructionTag,
                             JavaObjectHandle<AuthOrderSource> &&handle);

    ~AuthOrderSource() noexcept override;

    /**
     * Returns the available symbols by order source id (OrderSource::id()).
     *
     * @return the available symbols by order source id.
     */
    std::unordered_map<std::int32_t, std::unordered_set<std::string>> getByIds() const;

    /**
     * Returns the available symbols by order source. The sources are made with OrderSource::valueOf() from the ids of
     * getByIds().
     *
     * @return the available symbols by order source.
     */
    std::unordered_map<OrderSource, std::unordered_set<std::string>> getByOrderSources() const;
};

DXFCPP_END_NAMESPACE

/// @}

DXFCXX_DISABLE_MSC_WARNINGS_POP()
