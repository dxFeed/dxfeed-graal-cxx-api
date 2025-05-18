// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../entity/SharedEntity.hpp"
#include "../internal/CEntryPointErrors.hpp"
#include "../internal/Common.hpp"
#include "../internal/Handler.hpp"
#include "../internal/Isolate.hpp"
#include "../internal/JavaObjectHandle.hpp"
#include "../event/EventType.hpp"
#include "../event/EventTypeEnum.hpp"
#include "../symbols/SymbolWrapper.hpp"

#include <memory>
#include <mutex>
#include <unordered_set>

DXFCPP_BEGIN_NAMESPACE

/**
 * Represents an endpoint for accessing historical data, supporting configurable parameters such as compression and data
 * format. This class provides functionality to retrieve and process time-series data using a flexible API.
 */
struct DXFCPP_EXPORT HistoryEndpoint final : public dxfcpp::RequireMakeShared<HistoryEndpoint> {
    /// The alias to a type of shared pointer to the HistoryEndpoint object
    using Ptr = std::shared_ptr<HistoryEndpoint>;

    /// The alias to a type of unique pointer to the HistoryEndpoint object
    using Unique = std::unique_ptr<HistoryEndpoint>;

    /**
     * The Compression enum represents different compression algorithms that can be applied to data during transmission
     * or storage.
     * <p>
     * It provides three types of compression:
     * <ul>
     *     <li><code>NONE</code>: No compression is applied.
     *     <li><code>ZIP</code>: ZIP compression is applied.
     *     <li><code>GZIP</code>: GZIP compression is applied.
     * </ul>
     * <p>
     * This enum is used to dictate the type of compression to be applied
     * in various data handling scenarios.
     */
    enum class Compression { NONE = 0, ZIP, GZIP };

    /**
     * The Format enum represents different formats that can be used to handle data. It provides three types of
     * formats:
     * <p>
     * It provides three types of compression:
     * <ul>
     *     <li><code>TEXT</code>: Data is represented as plain text.
     *     <li><code>CSV</code>: Data is represented as comma-separated values.
     *     <li><code>BINARY</code>: Data is represented in binary format.
     * </ul>
     * <p>
     * This enum is used to specify the format in various data transmission and processing scenarios.
     */
    enum class Format { TEXT = 0, CSV, BINARY };

    private:
    JavaObjectHandle<HistoryEndpoint> handle_;

    public:

    class DXFCPP_EXPORT Builder : public RequireMakeShared<Builder> {
        friend HistoryEndpoint;

        JavaObjectHandle<Builder> handle_;

        public:

        std::shared_ptr<Builder> withAddress(const std::string &address);
        std::shared_ptr<Builder> withUserName(const std::string &userName);
        std::shared_ptr<Builder> withPassword(const std::string &password);
        std::shared_ptr<Builder> withAuthToken(const std::string &authToken);
        std::shared_ptr<Builder> withCompression(Compression compression);
        std::shared_ptr<Builder> withFormat(Format format);
        std::shared_ptr<HistoryEndpoint> build();
    };

    template <Derived<TimeSeriesEvent> E>
    std::vector<std::shared_ptr<E>> getTimeSeries(const SymbolWrapper& symbol, std::int64_t from, std::int64_t to) {
        return {};
    }

    std::shared_ptr<Builder> newBuilder();
};

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()