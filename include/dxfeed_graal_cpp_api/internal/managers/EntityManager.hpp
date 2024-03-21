// Copyright (c) 2024 Devexperts LLC.
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

template <typename EntityType_> struct EntityManager : private NonCopyable<EntityManager<EntityType_>> {
    using EntityType = EntityType_;

#if DXFCPP_DEBUG == 1
    static auto getDebugName() {
        return std::string("EntityManager<") + typeid(EntityType).name() + ">";
    }
#endif

    std::unordered_map<Id<EntityType>, std::shared_ptr<EntityType>> entitiesById_;
    std::unordered_map<std::shared_ptr<EntityType>, Id<EntityType>> idsByEntities_;
    std::atomic<std::size_t> lastId_{};
    std::mutex mutex_;

    EntityManager() : entitiesById_{}, idsByEntities_{}, mutex_{} {
        if constexpr (Debugger::isDebug) {
            Debugger::debug(getDebugName() + "()");
        }
    }

  public:
    Id<EntityType> registerEntity(std::shared_ptr<EntityType> entity) {
        if constexpr (Debugger::isDebug) {
            Debugger::debug(getDebugName() + "::registerEntity(" + entity->toString() + ")");
        }

        std::lock_guard lockGuard{mutex_};

        if (auto it = idsByEntities_.find(entity); it != idsByEntities_.end()) {
            return it->second;
        }

        auto id = Id<EntityType>::getNext();

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

    bool unregisterEntity(Id<EntityType> id) {
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
        return unregisterEntity(Id<EntityType>::template from<H>(handle));
    }

    template <typename H> bool unregisterEntity(const H *handle) {
        return unregisterEntity(Id<EntityType>::template from<H>(handle));
    }

    std::shared_ptr<EntityType> getEntity(Id<EntityType> id) {
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
        return getEntity(Id<EntityType>::template from<H>(handle));
    }

    template <typename H> std::shared_ptr<EntityType> getEntity(const H *handle) {
        return getEntity(Id<EntityType>::template from<H>(handle));
    }

    bool contains(Id<EntityType> id) {
        std::lock_guard lockGuard{mutex_};

        return entitiesById_.contains(id);
    }

    template <typename H> bool contains(H *handle) {
        return contains(Id<EntityType>::template from<H>(handle));
    }

    template <typename H> bool contains(const H *handle) {
        return contains(Id<EntityType>::template from<H>(handle));
    }

    std::optional<Id<EntityType>> getId(std::shared_ptr<EntityType> entity) {
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