// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <memory>
#include <unordered_map>
#include <unordered_set>

#include "../Common.hpp"

namespace dxfcpp {

struct DXEndpoint;

class DXEndpointManager : private NonCopyable<DXEndpointManager> {
    //TODO: Boost.Bimap
    std::unordered_map<Id<DXEndpoint>, std::shared_ptr<DXEndpoint>> entitiesById_{};
    std::unordered_map<std::shared_ptr<DXEndpoint>, Id<DXEndpoint>> idsByEntities_{};
    std::mutex mutex_{};

  public:

    Id<DXEndpoint> registerEntity(std::shared_ptr<DXEndpoint> endpoint) {
        std::lock_guard lockGuard{mutex_};

        if (auto it = idsByEntities_.find(endpoint); it != idsByEntities_.end()) {
            return it->second;
        }

        auto id = Id<DXEndpoint>::getNext();

        entitiesById_[id] = endpoint;
        idsByEntities_.emplace(std::make_pair(endpoint, id));

        return id;
    }

    bool unregisterEntity(std::shared_ptr<DXEndpoint> endpoint) {
        std::lock_guard lockGuard{mutex_};

        if (auto it = idsByEntities_.find(endpoint); it != idsByEntities_.end()) {
            entitiesById_.erase(it->second);
            idsByEntities_.erase(endpoint);

            return true;
        }

        return false;
    }

    std::shared_ptr<DXEndpoint> getEntity(Id<DXEndpoint> id) {
        std::lock_guard lockGuard{mutex_};

        if (auto it = entitiesById_.find(id); it != entitiesById_.end()) {
            return it->second;
        }

        return {};
    }

    std::optional<Id<DXEndpoint>> getId(std::shared_ptr<DXEndpoint> endpoint) {
        std::lock_guard lockGuard{mutex_};

        if (auto it = idsByEntities_.find(endpoint); it != idsByEntities_.end()) {
            return it->second;
        }

        return {};
    }
};

} // namespace dxfcpp