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
    std::unordered_map<Id<EntityType>, std::shared_ptr<EntityType>> entitiesById_{};
    std::unordered_map<std::shared_ptr<EntityType>, Id<EntityType>> idsByEntities_{};
    std::mutex mutex_{};

  public:
    Id<EntityType> registerEntity(std::shared_ptr<EntityType> entity) {
        if constexpr (isDebug) {
            debug("EntityManager::registerEntity({})", entity->toString());
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
            debug("EntityManager::unregisterEntity({})", entity->toString());
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
            debug("EntityManager::unregisterEntity(id = {})", id.getValue());
        }

        std::lock_guard lockGuard{mutex_};

        if (auto it = entitiesById_.find(id); it != entitiesById_.end()) {
            entitiesById_.erase(id);
            idsByEntities_.erase(it->second);

            return true;
        }

        return false;
    }

    std::shared_ptr<EntityType> getEntity(Id<EntityType> id) {
        std::lock_guard lockGuard{mutex_};

        if (auto it = entitiesById_.find(id); it != entitiesById_.end()) {
            return it->second;
        }

        return {};
    }

    std::optional<Id<EntityType>> getId(std::shared_ptr<EntityType> entity) {
        std::lock_guard lockGuard{mutex_};

        if (auto it = idsByEntities_.find(entity); it != idsByEntities_.end()) {
            return it->second;
        }

        return {};
    }
};

} // namespace dxfcpp