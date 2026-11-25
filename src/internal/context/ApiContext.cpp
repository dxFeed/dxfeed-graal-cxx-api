// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "ApiContext.hpp"

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

DXFCPP_BEGIN_NAMESPACE

ApiContext::ApiContext() noexcept {
}

ApiContext::~ApiContext() noexcept {
}

ApiContext &ApiContext::getInstance() noexcept {
    // Never deleted: the Graal callbacks may use the context until the process ends.
    // NOLINTNEXTLINE(bugprone-unhandled-exception-at-new): the function is noexcept, out of memory ends the process.
    static auto &instance = *new ApiContext{};

    return instance;
}

namespace {

/// The managers of the process by the name of their type.
struct ManagerRegistry {
    std::mutex mutex{};
    std::unordered_map<std::string, std::shared_ptr<void>> managers{};
};

} // namespace

std::shared_ptr<void> ApiContext::getManagerImpl(const char *typeName,
                                                 std::shared_ptr<void> (*create)()) const noexcept {
    // The registry owns the managers and is never deleted: the Graal callbacks look the entities up in the managers
    // until the process ends, and the entities are not destroyed at exit (their destructors would call Graal).
    static auto &registry = *new ManagerRegistry{};

    std::lock_guard lock(registry.mutex);
    auto &manager = registry.managers[typeName];

    if (!manager) {
        manager = create();
    }

    return manager;
}

DXFCPP_END_NAMESPACE
