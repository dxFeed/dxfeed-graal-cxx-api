// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../Common.hpp"
#include "../Id.hpp"
#include "../NonCopyable.hpp"

#include <memory>
#include <mutex>
#include <optional>
#include <unordered_map>

DXFCPP_BEGIN_NAMESPACE

template <typename EntityType_, typename EntityIdType_ = EntityType_>
struct EntityManager : private NonCopyable<EntityManager<EntityType_, EntityIdType_>> {
    using EntityType = EntityType_;
    using EntityIdType = EntityIdType_;

#if DXFCPP_DEBUG == 1
    static std::string getDebugName() {
        return typeid(EntityManager<EntityType_, EntityIdType_>).name();
    }
#endif

    std::unordered_map<Id<EntityIdType>, std::shared_ptr<EntityType>> entitiesById_;
    std::unordered_map<std::shared_ptr<EntityType>, Id<EntityIdType>> idsByEntities_;
    std::size_t lastId_{0};
    std::mutex mutex_;

    EntityManager() : entitiesById_{}, idsByEntities_{}, mutex_{} {
        if constexpr (Debugger::isDebug) {
            Debugger::debug(getDebugName() + "()");
        }
    }

    public:
    Id<EntityIdType> registerEntity(std::shared_ptr<EntityType> entity) {
        if constexpr (Debugger::isDebug) {
            Debugger::debug(getDebugName() + "::registerEntity(" + entity->toString() + ")");
        }

        std::lock_guard lockGuard{mutex_};

        if (auto it = idsByEntities_.find(entity); it != idsByEntities_.end()) {
            return it->second;
        }

        auto id = Id<EntityIdType>::getNext();

        entitiesById_[id] = entity;
        idsByEntities_.emplace(std::make_pair(entity, id));
        lastId_ = id.getValue();

        return id;
    }

    std::size_t getLastId() {
        std::lock_guard lockGuard{mutex_};

        return lastId_;
    }

    bool unregisterEntity(std::shared_ptr<EntityType> entity) {
        if constexpr (Debugger::isDebug) {
            Debugger::debug(getDebugName() + "::unregisterEntity(" + entity->toString() + ")");
        }

        std::lock_guard lockGuard{mutex_};

        if (auto it = idsByEntities_.find(entity); it != idsByEntities_.end()) {
            entitiesById_.erase(it->second);
            idsByEntities_.erase(entity);

            return true;
        }

        return false;
    }

    bool unregisterEntity(Id<EntityIdType> id) {
        if constexpr (Debugger::isDebug) {
            Debugger::debug(getDebugName() + "::unregisterEntity(id = " + std::to_string(id.getValue()) + ")");
        }

        std::lock_guard lockGuard{mutex_};

        if (auto it = entitiesById_.find(id); it != entitiesById_.end()) {
            idsByEntities_.erase(it->second);
            entitiesById_.erase(id);

            return true;
        }

        return false;
    }

    template <typename H> bool unregisterEntity(H *handle) {
        return unregisterEntity(Id<EntityIdType>::template from<H>(handle));
    }

    template <typename H> bool unregisterEntity(const H *handle) {
        return unregisterEntity(Id<EntityIdType>::template from<H>(handle));
    }

    std::shared_ptr<EntityType> getEntity(Id<EntityIdType> id) {
        if constexpr (Debugger::isDebug) {
            Debugger::debug(getDebugName() + "::getEntity(id = " + std::to_string(id.getValue()) + ")");
        }

        std::lock_guard lockGuard{mutex_};

        if (auto it = entitiesById_.find(id); it != entitiesById_.end()) {
            return it->second;
        }

        return {};
    }

    template <typename H> std::shared_ptr<EntityType> getEntity(H *handle) {
        return getEntity(Id<EntityIdType>::template from<H>(handle));
    }

    template <typename H> std::shared_ptr<EntityType> getEntity(const H *handle) {
        return getEntity(Id<EntityIdType>::template from<H>(handle));
    }

    bool contains(Id<EntityIdType> id) {
        std::lock_guard lockGuard{mutex_};

        return entitiesById_.contains(id);
    }

    template <typename H> bool contains(H *handle) {
        return contains(Id<EntityIdType>::template from<H>(handle));
    }

    template <typename H> bool contains(const H *handle) {
        return contains(Id<EntityIdType>::template from<H>(handle));
    }

    std::optional<Id<EntityIdType>> getId(std::shared_ptr<EntityType> entity) {
        if constexpr (Debugger::isDebug) {
            Debugger::debug(getDebugName() + "::getId(" + entity->toString() + ")");
        }

        std::lock_guard lockGuard{mutex_};

        if (auto it = idsByEntities_.find(entity); it != idsByEntities_.end()) {
            return it->second;
        }

        return {};
    }

    bool contains(std::shared_ptr<EntityType> entity) {
        std::lock_guard lockGuard{mutex_};

        return idsByEntities_.contains(entity);
    }
};

/**
 * Thread-safe registry that associates callback identifiers with entities without owning them.
 *
 * A callback obtains a temporary shared pointer with getEntity(), so an entity cannot be destroyed while that
 * callback is using it. Unlike EntityManager, this registry does not keep an otherwise unused entity alive.
 * Expired entries are removed lazily during lookups and explicitly by entity destructors.
 *
 * @tparam EntityType_ The registered entity type.
 * @tparam EntityIdType_ The type used to distinguish the identifier domain.
 */
template <typename EntityType_, typename EntityIdType_ = EntityType_>
class WeakEntityManager : private NonCopyable<WeakEntityManager<EntityType_, EntityIdType_>> {
    public:
    /// The registered entity type.
    using EntityType = EntityType_;

    /// The type whose identifier domain is used by the registry.
    using EntityIdType = EntityIdType_;

    private:
    /// A non-owning registration together with the address used for reverse lookup.
    struct Entry {
        std::weak_ptr<EntityType> entity;
        EntityType *address;
    };

    std::unordered_map<Id<EntityIdType>, Entry> entitiesById_{};
    std::unordered_map<EntityType *, Id<EntityIdType>> idsByEntities_{};
    std::size_t lastId_{0};
    std::mutex mutex_{};

    void eraseEntry(typename std::unordered_map<Id<EntityIdType>, Entry>::iterator entry) {
        if (const auto reverse = idsByEntities_.find(entry->second.address);
            reverse != idsByEntities_.end() && reverse->second == entry->first) {
            idsByEntities_.erase(reverse);
        }

        entitiesById_.erase(entry);
    }

    public:
    /** Constructs an empty weak entity registry. */
    WeakEntityManager() = default;

    /**
     * Registers an entity and returns its callback identifier.
     *
     * Registering the same live entity more than once returns its existing identifier.
     *
     * @param entity The entity to register.
     * @return The identifier associated with the entity.
     */
    Id<EntityIdType> registerEntity(const std::shared_ptr<EntityType> &entity) {
        std::lock_guard lockGuard{mutex_};
        auto *const address = entity.get();

        if (const auto reverse = idsByEntities_.find(address); reverse != idsByEntities_.end()) {
            if (const auto entry = entitiesById_.find(reverse->second);
                entry != entitiesById_.end() && !entry->second.entity.expired()) {
                return reverse->second;
            }

            idsByEntities_.erase(reverse);
        }

        const auto id = Id<EntityIdType>::getNext();
        entitiesById_.emplace(id, Entry{entity, address});
        idsByEntities_.emplace(address, id);
        lastId_ = id.getValue();

        return id;
    }

    /** @return The numeric value of the most recently assigned identifier. */
    std::size_t getLastId() {
        std::lock_guard lockGuard{mutex_};
        return lastId_;
    }

    /**
     * Unregisters an entity.
     *
     * @param entity The entity to unregister.
     * @return `true` if a registration was removed.
     */
    bool unregisterEntity(const std::shared_ptr<EntityType> &entity) {
        std::lock_guard lockGuard{mutex_};
        const auto reverse = idsByEntities_.find(entity.get());

        if (reverse == idsByEntities_.end()) {
            return false;
        }

        entitiesById_.erase(reverse->second);
        idsByEntities_.erase(reverse);
        return true;
    }

    /**
     * Unregisters an entity by identifier.
     *
     * @param id The registration identifier.
     * @return `true` if a registration was removed.
     */
    bool unregisterEntity(Id<EntityIdType> id) {
        std::lock_guard lockGuard{mutex_};
        const auto entry = entitiesById_.find(id);

        if (entry == entitiesById_.end()) {
            return false;
        }

        eraseEntry(entry);
        return true;
    }

    /**
     * Unregisters an entity using an opaque callback handle.
     *
     * @tparam H The callback handle type.
     * @param handle The handle containing the registration identifier.
     * @return `true` if a registration was removed.
     */
    template <typename H> bool unregisterEntity(H *handle) {
        return unregisterEntity(Id<EntityIdType>::template from<H>(handle));
    }

    /**
     * Unregisters an entity using a constant opaque callback handle.
     *
     * @tparam H The callback handle type.
     * @param handle The handle containing the registration identifier.
     * @return `true` if a registration was removed.
     */
    template <typename H> bool unregisterEntity(const H *handle) {
        return unregisterEntity(Id<EntityIdType>::template from<H>(handle));
    }

    /**
     * Locks and returns the entity registered for an identifier.
     *
     * The returned shared pointer keeps the entity alive until the caller finishes using it.
     *
     * @param id The registration identifier.
     * @return The registered entity, or an empty pointer if it has expired or is not registered.
     */
    std::shared_ptr<EntityType> getEntity(Id<EntityIdType> id) {
        std::lock_guard lockGuard{mutex_};
        const auto entry = entitiesById_.find(id);

        if (entry == entitiesById_.end()) {
            return {};
        }

        auto entity = entry->second.entity.lock();

        if (!entity) {
            eraseEntry(entry);
        }

        return entity;
    }

    /**
     * Locks and returns the entity identified by an opaque callback handle.
     *
     * @tparam H The callback handle type.
     * @param handle The handle containing the registration identifier.
     * @return The registered entity, or an empty pointer if it has expired or is not registered.
     */
    template <typename H> std::shared_ptr<EntityType> getEntity(H *handle) {
        return getEntity(Id<EntityIdType>::template from<H>(handle));
    }

    /**
     * Locks and returns the entity identified by a constant opaque callback handle.
     *
     * @tparam H The callback handle type.
     * @param handle The handle containing the registration identifier.
     * @return The registered entity, or an empty pointer if it has expired or is not registered.
     */
    template <typename H> std::shared_ptr<EntityType> getEntity(const H *handle) {
        return getEntity(Id<EntityIdType>::template from<H>(handle));
    }

    /** @return `true` if the identifier currently refers to a live entity. */
    bool contains(Id<EntityIdType> id) {
        return static_cast<bool>(getEntity(id));
    }

    /** @return The identifier of a live registered entity, if present. */
    std::optional<Id<EntityIdType>> getId(const std::shared_ptr<EntityType> &entity) {
        std::lock_guard lockGuard{mutex_};
        const auto reverse = idsByEntities_.find(entity.get());

        if (reverse == idsByEntities_.end()) {
            return {};
        }

        const auto entry = entitiesById_.find(reverse->second);

        if (entry == entitiesById_.end() || entry->second.entity.expired()) {
            if (entry != entitiesById_.end()) {
                eraseEntry(entry);
            } else {
                idsByEntities_.erase(reverse);
            }

            return {};
        }

        return reverse->second;
    }

    /** @return `true` if the entity is currently registered and alive. */
    bool contains(const std::shared_ptr<EntityType> &entity) {
        return getId(entity).has_value();
    }
};

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()
