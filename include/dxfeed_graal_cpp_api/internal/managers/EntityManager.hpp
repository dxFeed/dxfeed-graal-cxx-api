// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <memory>
#include <optional>
#include <unordered_map>
#include <unordered_set>

#include "../Common.hpp"

namespace dxfcpp {

template <typename EntityType> struct EntityManager : private NonCopyable<EntityManager<EntityType>> {
    // TODO: Boost.Bimap
    std::unordered_map<Id<EntityType>, std::shared_ptr<EntityType>> entitiesById_;
    std::unordered_map<std::shared_ptr<EntityType>, Id<EntityType>> idsByEntities_;
    std::mutex mutex_;

    EntityManager() : entitiesById_{}, idsByEntities_{}, mutex_{} {
        if constexpr (isDebug) {
            auto name = typeid(EntityType).name();

            debug("EntityManager<{}>()", name);
        }
    }

  public:
    Id<EntityType> registerEntity(std::shared_ptr<EntityType> entity) {
        if constexpr (isDebug) {
            auto name = typeid(EntityType).name();

            debug("EntityManager<{}>::registerEntity({})", name, entity->toString());
        }

        std::lock_guard lockGuard{mutex_};

        if (auto it = idsByEntities_.find(entity); it != idsByEntities_.end()) {
            return it->second;
        }

        auto id = Id<EntityType>::getNext();

        entitiesById_[id] = entity;
        idsByEntities_.emplace(std::make_pair(entity, id));

        return id;
    }

    bool unregisterEntity(std::shared_ptr<EntityType> entity) {
        if constexpr (isDebug) {
            auto name = typeid(EntityType).name();

            debug("EntityManager<{}>::unregisterEntity({})", name, entity->toString());
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
        if constexpr (isDebug) {
            auto name = typeid(EntityType).name();

            debug("EntityManager<{}>::unregisterEntity(id = {})", name, id.getValue());
        }

        std::lock_guard lockGuard{mutex_};

        if (auto it = entitiesById_.find(id); it != entitiesById_.end()) {
            idsByEntities_.erase(it->second);
            entitiesById_.erase(id);

            return true;
        }

        return false;
    }

    std::shared_ptr<EntityType> getEntity(Id<EntityType> id) {
        if constexpr (isDebug) {
            auto name = typeid(EntityType).name();

            debug("EntityManager<{}>::getEntity(id = {})", name, id.getValue());
        }

        std::lock_guard lockGuard{mutex_};

        if (auto it = entitiesById_.find(id); it != entitiesById_.end()) {
            return it->second;
        }

        return {};
    }

    std::optional<Id<EntityType>> getId(std::shared_ptr<EntityType> entity) {
        if constexpr (isDebug) {
            auto name = typeid(EntityType).name();

            debug("EntityManager<{}>::getId({})", name, entity->toString());
        }

        std::lock_guard lockGuard{mutex_};

        if (auto it = idsByEntities_.find(entity); it != idsByEntities_.end()) {
            return it->second;
        }

        return {};
    }
};

} // namespace dxfcpp