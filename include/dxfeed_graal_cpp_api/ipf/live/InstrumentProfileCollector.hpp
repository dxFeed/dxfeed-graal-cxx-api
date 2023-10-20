// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../internal/Conf.hpp"

#include "../../internal/Common.hpp"
#include "../../internal/Id.hpp"
#include "../../internal/JavaObjectHandle.hpp"

#include "../../entity/SharedEntity.hpp"

namespace dxfcpp {

class InstrumentProfileConnection;

class DXFCPP_EXPORT InstrumentProfileCollector final : public SharedEntity {
    friend InstrumentProfileConnection;

    Id<InstrumentProfileCollector> id_;
    JavaObjectHandle<InstrumentProfileCollector> handle_;
    JavaObjectHandle<InstrumentProfileUpdateListener> instrumentProfileUpdateListenerHandle_;
    Handler<void(const std::vector<std::shared_ptr<InstrumentProfile>> &)> onInstrumentProfilesUpdate_{1};

    InstrumentProfileCollector() noexcept;

    struct Impl;

  public:
    /// The alias to a type of shared pointer to the InstrumentProfileCollector object
    using Ptr = std::shared_ptr<InstrumentProfileCollector>;

    /// The alias to a type of unique pointer to the InstrumentProfileCollector object
    using Unique = std::unique_ptr<InstrumentProfileCollector>;

    /**
     * Creates the new InstrumentProfileCollector
     *
     * @return The new InstrumentProfileCollector
     */
    static Ptr create() noexcept;

    /**
     * Returns last modification time (in milliseconds) of instrument profiles or zero if it is unknown.
     * Note, that while the time is represented in milliseconds, the actual granularity of time here is a second.
     *
     * @return The last modification time (in milliseconds) of instrument profiles or zero if it is unknown.
     */
    std::int64_t getLastUpdateTime() const noexcept;

    /**
     * Returns last modification time (as std::chrono::milliseconds) of instrument profiles or zero if it is unknown.
     * Note, that while the time is represented in milliseconds, the actual granularity of time here is a second.
     *
     * @return The last modification time (as std::chrono::milliseconds) of instrument profiles or zero if it is
     * unknown.
     */
    std::chrono::milliseconds getLastUpdateTimeAsDuration() const noexcept {
        return std::chrono::milliseconds(getLastUpdateTime());
    }

    /**
     * Convenience method to update one instrument profile in this collector. This is a shortcut for:
     * <pre><tt>
     *    @ref InstrumentProfileCollector::updateInstrumentProfiles() "updateInstrumentProfiles"({ip}, nullptr);
     * </tt></pre>
     *
     * @param ip The instrument profile.
     */
    void updateInstrumentProfile(std::shared_ptr<InstrumentProfile> ip) const noexcept;

    /**
     * Convenience method to update one instrument profile in this collector. This is a shortcut for:
     * <pre><tt>
     *    @ref InstrumentProfileCollector::updateInstrumentProfiles() "updateInstrumentProfiles"({ip}, nullptr);
     * </tt></pre>
     *
     * @param ip The instrument profile.
     */
    void updateInstrumentProfile(const InstrumentProfile &ip) const noexcept;

    /**
     * Adds listener that is notified about any updates in the set of instrument profiles.
     * If a set of instrument profiles is not empty, then this listener will be immediately notified.
     *
     * @warning The API will be changed in the future. Instead of a vector, a wrapped Java-like iterator will be passed.
     *
     * Example:
     * ```cpp
     * collector->addUpdateListener([](const std::vector<std::shared_ptr<dxfcpp::InstrumentProfile>> &profiles) -> void {
     *     for (const auto &p : profiles) {
     *         if (InstrumentProfileType::REMOVED->getName() == p->getType()) {
     *             std::cout << p->getSymbol() + ": " + p->getType() + "\n";
     *         } else {
     *             std::cout << p->getSymbol() + " (" + p->getDescription() + ")\n";
     *         }
     *     }
     * });
     * ```
     *
     * Example:
     * ```cpp
     * collector->addUpdateListener([](auto &&profiles) -> void {
     *     for (const auto &p : profiles) {
     *         if (InstrumentProfileType::REMOVED->getName() == p->getType()) {
     *             std::cout << p->getSymbol() + ": " + p->getType() + "\n";
     *         } else {
     *             std::cout << p->getSymbol() + " (" + p->getDescription() + ")\n";
     *         }
     *     }
     * });
    * ```
     * @tparam InstrumentProfileUpdateListener The listener type. Listener can be callable with signature: `void(const
     * std::vector<std::shared_ptr<InstrumentProfile>&)`
     * @param listener The profile update listener. The listener can be callable with signature: `void(const
     * std::vector<std::shared_ptr<InstrumentProfile>&)`
     * @return The listener id
     */
    template <typename InstrumentProfileUpdateListener>
    std::size_t addUpdateListener(InstrumentProfileUpdateListener listener) noexcept
#if __cpp_concepts
        requires requires {
            { listener(std::vector<std::shared_ptr<InstrumentProfile>>{}) } -> std::same_as<void>;
        }
#endif
    {
        return onInstrumentProfilesUpdate_ += listener;
    }

    /**
     * Removes listener for instrument profile updates.
     *
     * Example:
     * ```cpp
     * auto id = collector->addUpdateListener([](auto){});
     *
     * collector->removeEventListener(id);
     * ```
     *
     * @param listenerId The listener id
     */
    void removeUpdateListener(std::size_t listenerId) noexcept {
        onInstrumentProfilesUpdate_ -= listenerId;
    }
};

} // namespace dxfcpp