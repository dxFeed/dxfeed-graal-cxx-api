// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../entity/SharedEntity.hpp"
#include "../event/EventType.hpp"
#include "../event/EventTypeEnum.hpp"
#include "../internal/CEntryPointErrors.hpp"
#include "../internal/Common.hpp"
#include "../internal/Handler.hpp"
#include "../internal/Isolate.hpp"
#include "../internal/JavaObjectHandle.hpp"
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

    std::vector<std::shared_ptr<EventType>> getTimeSeriesImpl(const EventTypeEnum &eventType,
                                                              const SymbolWrapper &symbol, std::int64_t from,
                                                              std::int64_t to) const;

    public:
    explicit HistoryEndpoint(RequireMakeShared<HistoryEndpoint>::LockExternalConstructionTag,
                             JavaObjectHandle<HistoryEndpoint> &&handle);
    ~HistoryEndpoint() override = default;

    /**
     * Builder is a static inner class that provides a flexible and readable way to construct instances of the
     * HistoryEndpoint class.
     * <p>
     * This class follows the builder pattern, allowing users to specify various configuration parameters for a
     * HistoryEndpoint instance through fluent method chaining. The final built object is created using the
     * Builder::build() method.
     */
    class DXFCPP_EXPORT Builder : public RequireMakeShared<Builder> {
        friend HistoryEndpoint;

        JavaObjectHandle<Builder> handle_;

        public:
        explicit Builder(RequireMakeShared<Builder>::LockExternalConstructionTag, JavaObjectHandle<Builder> &&handle);
        ~Builder() override = default;

        /**
         * Specifies the address for the target endpoint.
         *
         * @param address the address of the endpoint to be set
         * @return the Builder instance with the updated address value
         */
        std::shared_ptr<Builder> withAddress(const std::string &address);

        /**
         * Sets the username for the target endpoint.
         *
         * @param userName the username to be set for the endpoint
         * @return the Builder instance with the updated username value
         */
        std::shared_ptr<Builder> withUserName(const std::string &userName);

        /**
         * Sets the password for the target endpoint.
         *
         * @param password the password to be set for the endpoint
         * @return the Builder instance with the updated password value
         */
        std::shared_ptr<Builder> withPassword(const std::string &password);

        /**
         * Sets the authentication token for the target endpoint.
         *
         * @param authToken the authentication token to be used for access
         * @return the Builder instance with the updated authentication token value
         */
        std::shared_ptr<Builder> withAuthToken(const std::string &authToken);

        /**
         * Sets the compression type to be used for data transmission or storage.
         *
         * @param compression the compression type to be applied, represented by the {@link Compression} enum
         * @return the Builder instance with the updated compression value
         */
        std::shared_ptr<Builder> withCompression(Compression compression);

        /**
         * Sets the format to be used for data handling.
         *
         * @param format the format type to be applied, represented by the {@link Format} enum
         * @return the Builder instance with the updated format value
         */
        std::shared_ptr<Builder> withFormat(Format format);

        /**
         * Builds and returns a configured instance of {@code HistoryEndpoint}.
         * <p>
         * This method uses the values set in the {@code Builder} instance, such as address, username, password,
         * compression, format, and authentication token, to create a new {@code HistoryEndpoint} object.
         *
         * @return a new instance of {@code HistoryEndpoint} configured with the provided settings
         */
        std::shared_ptr<HistoryEndpoint> build();
    };

    /**
     * Retrieves a list of time series events for a specific type of event and symbol within the given time range.
     *
     * @tparam E the subclass of {@link TimeSeriesEvent} that specifies the type of event to retrieve
     * @param symbol the identifier of the symbol for which the time series data is requested
     * @param from the start timestamp for the time series query, in milliseconds since epoch
     * @param to the end timestamp for the time series query, in milliseconds since epoch
     * @return a list of events of the specified type and symbol within the given time range
     */
    template <Derived<TimeSeriesEvent> E>
    std::vector<std::shared_ptr<E>> getTimeSeries(const SymbolWrapper &symbol, std::int64_t from, std::int64_t to) {
        return convertEvents<EventType, E>(getTimeSeriesImpl(E::TYPE, symbol, from, to));
    }

    /**
     * Creates a new instance of {@link HistoryEndpoint.Builder} with default configurations. The default settings
     * include:
     * <ul>
     *     <li>{@link Compression#ZIP} as the compression type.
     *     <li>{@link Format#BINARY} as the data format.
     * </ul>
     *
     * @return a new {@link HistoryEndpoint.Builder} instance with preset default values
     */
    static std::shared_ptr<Builder> newBuilder();
};

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()