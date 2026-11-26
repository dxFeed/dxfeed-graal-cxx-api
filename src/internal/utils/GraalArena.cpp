// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "GraalArena.hpp"

#include <algorithm>
#include <cstdint>
#include <functional>
#include <new>

DXFCPP_BEGIN_NAMESPACE

namespace {

constexpr std::size_t MAX_ALIGNMENT = alignof(std::max_align_t);

constexpr std::size_t alignUp(std::size_t value, std::size_t alignment) noexcept {
    return (value + alignment - 1) & ~(alignment - 1);
}

thread_local GraalArena *currentArena = nullptr;

} // namespace

// The header of a block; its memory follows it, aligned to MAX_ALIGNMENT.
struct GraalArena::Block {
    Block *next;
    std::size_t capacity;
    std::size_t used;

    static constexpr std::size_t HEADER_SIZE = alignUp(sizeof(Block *) + 2 * sizeof(std::size_t), MAX_ALIGNMENT);

    static Block *create(std::size_t capacity) {
        void *memory = ::operator new(HEADER_SIZE + capacity);

        return new (memory) Block{nullptr, capacity, 0};
    }

    static void destroy(Block *block) noexcept {
        ::operator delete(static_cast<void *>(block));
    }

    unsigned char *data() noexcept {
        return reinterpret_cast<unsigned char *>(this) + HEADER_SIZE;
    }

    const unsigned char *data() const noexcept {
        return reinterpret_cast<const unsigned char *>(this) + HEADER_SIZE;
    }

    // The aligned place of `size` bytes in the free memory of the block, or nullptr if they do not fit. The address is
    // aligned, not the offset: the memory of the block itself is aligned to MAX_ALIGNMENT only.
    void *tryAllocate(std::size_t size, std::size_t alignment) noexcept {
        const auto base = reinterpret_cast<std::uintptr_t>(data());
        const auto offset = static_cast<std::size_t>(alignUp(base + used, alignment) - base);

        if (offset > capacity || size > capacity - offset) {
            return nullptr;
        }

        used = offset + size;

        return data() + offset;
    }
};

GraalArena::GraalArena(Block *first) noexcept : first_(first), current_(first) {
}

GraalArena *GraalArena::create(std::size_t capacity) {
    const auto self = alignUp(sizeof(GraalArena), MAX_ALIGNMENT);
    auto *block = Block::create(self + capacity);

    // The arena is the start of its first block.
    void *place = block->tryAllocate(sizeof(GraalArena), MAX_ALIGNMENT);

    return new (place) GraalArena(block);
}

void GraalArena::destroy(GraalArena *arena) noexcept {
    if (arena == nullptr) {
        return;
    }

    // The arena lives in its first block: the other blocks go first.
    auto *first = arena->first_;

    for (auto *block = first->next; block != nullptr;) {
        auto *next = block->next;

        Block::destroy(block);
        block = next;
    }

    Block::destroy(first);
}

void *GraalArena::allocate(std::size_t size, std::size_t alignment) {
    if (void *memory = current_->tryAllocate(size, alignment)) {
        return memory;
    }

    // A new block: a large allocation gets a block of its own size, with room to align it beyond MAX_ALIGNMENT.
    const auto padding = alignment > MAX_ALIGNMENT ? alignment - MAX_ALIGNMENT : 0;
    auto *block = Block::create(std::max(BLOCK_CAPACITY, alignUp(size, MAX_ALIGNMENT) + padding));

    current_->next = block;
    current_ = block;

    return block->tryAllocate(size, alignment);
}

bool GraalArena::owns(const void *pointer) const noexcept {
    const auto *p = static_cast<const unsigned char *>(pointer);

    for (const auto *block = first_; block != nullptr; block = block->next) {
        if (!std::less<const unsigned char *>{}(p, block->data()) &&
            std::less<const unsigned char *>{}(p, block->data() + block->capacity)) {
            return true;
        }
    }

    return false;
}

std::size_t GraalArena::blockCount() const noexcept {
    std::size_t count = 0;

    for (const auto *block = first_; block != nullptr; block = block->next) {
        count++;
    }

    return count;
}

GraalArena *GraalArena::current() noexcept {
    return currentArena;
}

GraalArena::Scope::Scope(GraalArena *arena) noexcept : previous_(currentArena) {
    currentArena = arena;
}

GraalArena::Scope::~Scope() {
    currentArena = previous_;
}

DXFCPP_END_NAMESPACE
