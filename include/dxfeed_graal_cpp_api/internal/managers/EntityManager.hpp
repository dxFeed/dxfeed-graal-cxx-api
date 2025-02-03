// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include <memory>
#include <optional>
#include <unordered_map>
#include <unordered_set>

#include "../Common.hpp"

#include "../NonCopyable.hpp"

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
    std::atomic<std::size_t> lastId_{};
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

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()