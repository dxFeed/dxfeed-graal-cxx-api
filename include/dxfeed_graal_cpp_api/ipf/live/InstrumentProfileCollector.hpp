// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../../internal/Common.hpp"
#include "../../internal/Id.hpp"
#include "../../internal/JavaObjectHandle.hpp"

#include "../../entity/SharedEntity.hpp"
#include "../InstrumentProfile.hpp"
#include "IterableInstrumentProfile.hpp"

#include <unordered_map>

DXFCPP_BEGIN_NAMESPACE

class InstrumentProfileConnection;

/**
 * Collects instrument profile updates and provides the live list of instrument profiles.
 * This class contains a map that keeps a unique instrument profile per symbol.
 * This class is intended to be used with InstrumentProfileConnection as a repository that keeps
 * profiles of all known instruments. See InstrumentProfileConnection for a usage example.
 *
 * <p>As set of instrument profiles stored in this collector can be accessed with @ref InstrumentProfileCollector::view() "view" method.
 * A snapshot plus a live stream of updates can be accessed with
 * @ref InstrumentProfileCollector::addUpdateListener() "addUpdateListener" method.
 *
 * <p>Removal of instrument profile is represented by an InstrumentProfile instance with a
 * @ref InstrumentProfile::getType() "type" equal to
 * <code>@ref InstrumentProfileType "InstrumentProfileType"::@ref InstrumentProfileType::REMOVED "REMOVED".@ref InstrumentProfileType::getName() "getName()"</code>.
 *
 * <p><b>This class is thread-safe.</b>
 */
class DXFCPP_EXPORT InstrumentProfileCollector final : public SharedEntity {
    friend InstrumentProfileConnection;

    Id<InstrumentProfileCollector> id_;
    JavaObjectHandle<InstrumentProfileCollector> handle_;

    std::mutex listenersMutex_{};
    std::unordered_map<std::size_t, JavaObjectHandle<InstrumentProfileUpdateListener>> listenerHandles_{};
    std::unordered_map<std::size_t, SimpleHandler<void(const std::vector<std::shared_ptr<InstrumentProfile>> &)>>
        onInstrumentProfilesUpdateHandlers_{};

    InstrumentProfileCollector() noexcept;

    struct Impl;

    void addListenerHandle(std::size_t id) noexcept;
    void removeListenerHandle(std::size_t id) noexcept;

    void removeUpdateListenerImpl(std::size_t listenerId) noexcept {
        removeListenerHandle(listenerId);

        onInstrumentProfilesUpdateHandlers_[listenerId].remove(listenerId);
        onInstrumentProfilesUpdateHandlers_.erase(listenerId);
    }

  public:
    /// The alias to a type of shared pointer to the InstrumentProfileCollector object
    using Ptr = std::shared_ptr<InstrumentProfileCollector>;

    /// The alias to a type of unique pointer to the InstrumentProfileCollector object
    using Unique = std::unique_ptr<InstrumentProfileCollector>;

    ~InstrumentProfileCollector() noexcept override;

    /**
     * Creates the new InstrumentProfileCollector
     *
     * @return The new InstrumentProfileCollector
     */
    static Ptr create();

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
    void updateInstrumentProfile(std::shared_ptr<InstrumentProfile> ip) const;

    /**
     * Returns a concurrent view of the set of instrument profiles.
     * Note, that removal of instrument profile is represented by an InstrumentProfile instance with a
     * @ref InstrumentProfile::getType() "type" equal to
     * <code>@ref InstrumentProfileType "InstrumentProfileType"::@ref InstrumentProfileType::REMOVED "REMOVED".@ref
     * InstrumentProfileType::getName() "getName()"</code>. Normally, this view exposes only non-removed profiles.
     * However, if iteration is concurrent with removal, then a removed instrument profile (with a removed type) can be
     * exposed by this view.
     *
     * @return A concurrent view of the set of instrument profiles.
     */
    std::shared_ptr<IterableInstrumentProfile> view() const noexcept;

    /**
     * Adds listener that is notified about any updates in the set of instrument profiles.
     * If a set of instrument profiles is not empty, then this listener will be immediately notified.
     *
     * @warning The API will be changed in the future. Instead of a vector, a wrapped Java-like iterator will be passed.
     *
     * Example:
     * ```cpp
     * collector->addUpdateListener([](const std::vector<std::shared_ptr<dxfcpp::InstrumentProfile>> &profiles) {
     *     for (const auto &p : profiles) {
     *         if (InstrumentProfileType::REMOVED->getName() == p->getType()) {
     *             std::cout << p->getSymbol() + ": " + p->getType() + "\n";
     *         } else {
     *             std::cout << p->getSymbol() + " (" + p->getDescription() + ")\n";
     *         }
     *     }
     * });
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
        std::lock_guard guard{listenersMutex_};

        SimpleHandler<void(const std::vector<std::shared_ptr<InstrumentProfile>> &)> h{};

        std::size_t id = h.add(listener);
        onInstrumentProfilesUpdateHandlers_.try_emplace(id, std::move(h));
        addListenerHandle(id);

        return id;
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
        std::lock_guard guard{listenersMutex_};

        if (onInstrumentProfilesUpdateHandlers_.contains(listenerId)) {
            removeUpdateListenerImpl(listenerId);
        }
    }
};

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()