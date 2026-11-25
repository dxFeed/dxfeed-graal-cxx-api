// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../../include/dxfeed_graal_cpp_api/detail/Conf.hpp"
#include "../InternalExport.hpp"

#include <cstddef>

DXFCPP_BEGIN_NAMESPACE

/**
 * The memory of the native events of one list: blocks taken from the heap, freed all at once.
 *
 * While a GraalArena::Scope is alive on a thread, newGraalEvent() and createCString() of that thread take their memory
 * from the arena of the scope, so a list of events costs a few heap operations instead of a few per event. Memory of the
 * arena is never freed alone: the free functions of the events are not called for it.
 */
class DXFCPP_INTERNAL_EXPORT GraalArena final {
    struct Block;

    Block *first_;
    Block *current_;

    explicit GraalArena(Block *first) noexcept;

    public:
    /// The capacity of a block after the first one; a larger allocation gets a block of its own size.
    static constexpr std::size_t BLOCK_CAPACITY = 64 * 1024;

    /**
     * Creates an arena whose first block holds at least `capacity` bytes (with the arena itself in it).
     *
     * @throws std::bad_alloc
     */
    static GraalArena *create(std::size_t capacity);

    /// Frees the arena and all its memory; nullptr is ignored.
    static void destroy(GraalArena *arena) noexcept;

    GraalArena(const GraalArena &) = delete;
    GraalArena &operator=(const GraalArena &) = delete;

    /**
     * Returns `size` bytes aligned to `alignment` (a power of two), not initialized.
     *
     * @throws std::bad_alloc
     */
    void *allocate(std::size_t size, std::size_t alignment);

    /// Whether the pointer is in the memory of the arena.
    bool owns(const void *pointer) const noexcept;

    /// The number of blocks (for the tests).
    std::size_t blockCount() const noexcept;

    /// The arena of the scope of the current thread, or nullptr.
    static GraalArena *current() noexcept;

    /// Makes the arena current on this thread for its lifetime and restores the previous one after.
    class Scope final {
        GraalArena *previous_;

        public:
        explicit Scope(GraalArena *arena) noexcept;
        ~Scope();

        Scope(const Scope &) = delete;
        Scope &operator=(const Scope &) = delete;
    };
};

DXFCPP_END_NAMESPACE
