// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0

module;
#include <toy/api.hpp>

export module toy;

// Re-export global-module entities without duplicating their declarations.
export namespace toy {
using ::toy::bump;
using ::toy::identity;
using ::toy::sum;
using ::toy::Value;
} // namespace toy
