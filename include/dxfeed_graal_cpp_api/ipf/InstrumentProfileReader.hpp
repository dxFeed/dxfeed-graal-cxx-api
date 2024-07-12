// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../internal/Common.hpp"
#include "../internal/Id.hpp"
#include "../internal/JavaObjectHandle.hpp"

#include "../entity/SharedEntity.hpp"

#include "InstrumentProfile.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

DXFCPP_BEGIN_NAMESPACE

/**
 * Reads instrument profiles from the stream using Instrument Profile Format (IPF).
 * Please see <b>Instrument Profile Format</b> documentation for complete description.
 * This reader automatically uses data formats as specified in the stream.
 *
 * <p>This reader is intended for "one time only" usage: create new instances for new IPF reads.
 * <p>Use InstrumentProfileConnection if support for streaming updates of instrument profiles is needed.
 *
 * <p>For backward compatibility reader can be configured with system property "-Dcom.dxfeed.ipf.complete" to control
 * the strategy for missing "##COMPLETE" tag when reading IPF, possible values are:
 * <ul>
 *     <li>`warn` - show warning in the log (default)</li>
 *     <li>`error - throw exception (future default)</li>
 *     <li>`ignore` - do nothing (for backward compatibility)</li>
 * </ul>
 */
class DXFCPP_EXPORT InstrumentProfileReader final : public SharedEntity {
    Id<InstrumentProfileReader> id_;
    JavaObjectHandle<InstrumentProfileReader> handle_;

    InstrumentProfileReader();

  public:
    /// The alias to a type of shared pointer to the InstrumentProfileReader object
    using Ptr = std::shared_ptr<InstrumentProfileReader>;

    /// The alias to a type of unique pointer to the InstrumentProfileReader object
    using Unique = std::unique_ptr<InstrumentProfileReader>;

    /**
     * Creates the new InstrumentProfileReader
     *
     * @return The new InstrumentProfileReader
     */
    static Ptr create();

    /**
     * Returns last modification time (in milliseconds) from last InstrumentProfileReader::readFromFile() operation
     * or zero if it is unknown.
     */
    std::int64_t getLastModified() const;

    /**
     * Returns `true` if IPF was fully read on last InstrumentProfileReader::readFromFile() operation.
     */
    bool wasComplete() const;

    /**
     * Converts a specified string address specification into an URL that will be read by
     * InstrumentProfileReader::readFromFile() method.
     */
    static std::string resolveSourceURL(const StringLikeWrapper &address);

    /**
     * Reads and returns instrument profiles from specified file.
     * This method recognizes data compression formats "zip" and "gzip" automatically.
     * In case of <em>zip</em> the first file entry will be read and parsed as a plain data stream.
     * In case of <em>gzip</em> compressed content will be read and processed.
     * In other cases data considered uncompressed and will be parsed as is.
     *
     * <p>Authentication information can be supplied to this method as part of URL user info
     * like {@code "http://user:password@host:port/path/file.ipf"}.
     *
     * <p>This operation updates @ref ::getLastModified() "lastModified" and @ref ::wasComplete() "wasComplete".
     *
     * @param address URL of file to read from
     * @return list of instrument profiles
     */
    std::vector<std::shared_ptr<InstrumentProfile>> readFromFile(const StringLikeWrapper &address) const;

    /**
     * Reads and returns instrument profiles from specified address with a specified basic user and password
     * credentials.
     * This method recognizes data compression formats "zip" and "gzip" automatically.
     * In case of <em>zip</em> the first file entry will be read and parsed as a plain data stream.
     * In case of <em>gzip</em> compressed content will be read and processed.
     * In other cases data considered uncompressed and will be parsed as is.
     *
     * <p>Specified user and password take precedence over authentication information that is supplied to this method
     * as part of URL user info like {@code "http://user:password@host:port/path/file.ipf"}.
     *
     * <p>This operation updates @ref ::getLastModified() "lastModified" and @ref ::wasComplete() "wasComplete".
     *
     * @param address URL of file to read from
     * @param user the user name
     * @param password the password
     * @return list of instrument profiles
     */
    std::vector<std::shared_ptr<InstrumentProfile>> readFromFile(const StringLikeWrapper &address,
                                                                 const StringLikeWrapper &user,
                                                                 const StringLikeWrapper &password) const;

    /**
     * Reads and returns instrument profiles from specified address with a specified token credentials.
     * This method recognizes data compression formats "zip" and "gzip" automatically.
     * In case of <em>zip</em> the first file entry will be read and parsed as a plain data stream.
     * In case of <em>gzip</em> compressed content will be read and processed.
     * In other cases data considered uncompressed and will be parsed as is.
     *
     * <p>Specified token take precedence over authentication information that is supplied to this method
     * as part of URL user info like {@code "http://user:password@host:port/path/file.ipf"}.
     *
     * <p>This operation updates @ref ::getLastModified() "lastModified" and @ref ::wasComplete() "wasComplete".
     *
     * @param address URL of file to read from
     * @param token the token
     * @return list of instrument profiles
     * @throws JavaException("InstrumentProfileFormatException") if input stream does not conform to the Instrument
     * Profile Format
     * @throws JavaException("IOException") If an I/O error occurs
     */
    std::vector<std::shared_ptr<InstrumentProfile>> readFromFile(const StringLikeWrapper &address,
                                                                 const AuthToken &token) const;
};

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()