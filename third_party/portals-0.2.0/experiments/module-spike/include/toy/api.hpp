// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0

#pragma once

#include <span>

#if defined(_WIN32)
#  if defined(TOY_BUILDING_LIBRARY)
#    define TOY_API __declspec(dllexport)
#  else
#    define TOY_API __declspec(dllimport)
#  endif
#else
#  define TOY_API __attribute__((visibility("default")))
#endif

/** @brief Experimental values and functions; no Portals UI or stable ABI. */
namespace toy {
/** @brief Independent, owning scalar value. */
struct Value {
  /** @brief The scalar represented by this value. */
  int number;
};

/**
 * @brief Return the successor, saturating at the maximum representable int.
 * @param value Owning input copied by value.
 * @return Independent value; never invalidated by another call.
 * @details Thread-safe, no affinity, allocation, I/O, errors or invalidation.
 * Same-toolchain shared-library experiment; not a stable C++ ABI promise.
 */
TOY_API Value bump(Value value) noexcept;

/**
 * @brief Sum a borrowed contiguous range into a wide accumulator.
 * @param values Borrowed only during the call; caller keeps storage alive and
 * immutable. Every mathematical prefix sum must fit in long long.
 * @return Independent scalar, zero for an empty range.
 * @details Concurrent calls allowed with immutable inputs; no affinity,
 * allocation, I/O, errors or invalidation. Prefix representability is a
 * precondition of this deliberately small ABI experiment, not a UI policy.
 */
TOY_API long long sum(std::span<const int> values) noexcept;

/**
 * @brief Copy a value through the single-source inline template.
 * @tparam T Copy-constructible value type.
 * @param value Borrowed during the call.
 * @return A copy, with ownership determined by T (a pointer remains observing).
 * @details Affinity, copying errors and invalidation follow T's copy operation.
 * For Value, copying cannot fail and has no affinity or invalidation.
 */
template <typename T> T identity(const T &value) {
  return value;
}
} // namespace toy
