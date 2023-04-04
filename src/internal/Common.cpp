// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_native_c_api/api.h>
#include <dxfeed_graal_native_cpp_api/api.hpp>

#include <cstring>
#include <memory>
#include <utf8cpp/utf8.h>
#include <utility>

namespace dxfcpp {

namespace handler_utils {

template <typename T> void JavaObjectHandler<T>::deleter(void *handler) noexcept {
    runIsolatedOrElse(
        [handler = handler](auto threadHandle) {
            if (handler) {
                return dxfg_JavaObjectHandler_release(threadHandle, bit_cast<dxfg_java_object_handler *>(handler)) == 0;
            }

            return true;
        },
        false);
}

struct EventClassList::Impl {
    dxfg_event_clazz_list_t list_;

    Impl() noexcept : list_{0, nullptr} {}

    void set(std::size_t index, std::uint32_t id) const noexcept {
        if (list_.size == 0) {
            return;
        }

        if (index < list_.size) {
            *list_.elements[index] = static_cast<dxfg_event_clazz_t>(id);
        }
    }

    [[nodiscard]] bool isEmpty() const noexcept { return list_.size == 0; }

    [[nodiscard]] std::size_t size() const noexcept { return static_cast<std::size_t>(list_.size); }

    void *getHandler() noexcept { return bit_cast<void *>(&list_); }

    void init(std::uint32_t size) noexcept {
        if (size <= 0) {
            return;
        }

        list_.size = static_cast<std::int32_t>(size);
        list_.elements = new (std::nothrow) dxfg_event_clazz_t *[list_.size];

        if (!list_.elements) {
            release();

            return;
        }

        bool needToRelease = false;

        for (std::int32_t i = 0; i < list_.size; i++) {
            list_.elements[i] = new (std::nothrow) dxfg_event_clazz_t{};

            if (!list_.elements[i]) {
                needToRelease = true;
            }
        }

        if (needToRelease) {
            release();
        }
    }

    void release() {
        if (list_.size == 0 || list_.elements == nullptr) {
            return;
        }

        for (auto i = list_.size - 1; i >= 0; i--) {
            delete list_.elements[i];
        }

        delete[] list_.elements;
        list_.size = 0;
        list_.elements = nullptr;
    }

    ~Impl() noexcept { release(); }
};

template struct handler_utils::JavaObjectHandler<DXEndpoint>;
template struct handler_utils::JavaObjectHandler<DXEndpointStateChangeListener>;
template struct handler_utils::JavaObjectHandler<DXFeed>;
template struct handler_utils::JavaObjectHandler<DXFeedSubscription>;
template struct handler_utils::JavaObjectHandler<DXFeedEventListener>;
template struct handler_utils::JavaObjectHandler<DXEndpoint::Builder>;

EventClassList::EventClassList() noexcept : impl_(std::make_unique<EventClassList::Impl>()) {}

std::unique_ptr<EventClassList> EventClassList::create(std::size_t size) noexcept {
    auto result = std::unique_ptr<EventClassList>(new EventClassList{});

    result->impl_->init(static_cast<std::uint32_t>(size));

    return result;
}

void EventClassList::set(std::size_t index, std::uint32_t id) noexcept { impl_->set(index, id); }

bool EventClassList::isEmpty() const noexcept { return impl_->isEmpty(); }

std::size_t EventClassList::size() const noexcept { return impl_->size(); }

void *EventClassList::getHandler() noexcept { return impl_->getHandler(); }

EventClassList::~EventClassList() noexcept {}

}

std::string toString(const char *chars) {
    if (chars == nullptr) {
        return "";
    }

    return chars;
}

char utf16to8(std::int16_t in) {
    try {
        std::string out{};
        auto utf16in = {in};

        utf8::utf16to8(std::begin(utf16in), std::end(utf16in), std::back_inserter(out));

        return out.empty() ? char{} : out[0];
    } catch (...) {
        // TODO: error handling
        return char{};
    }
}

std::int16_t utf8to16(char in) {
    try {
        std::u16string out{};
        auto utf8in = {in};

        utf8::utf8to16(std::begin(utf8in), std::end(utf8in), std::back_inserter(out));

        return out.empty() ? std::int16_t{} : static_cast<std::int16_t>(out[0]);
    } catch (...) {
        // TODO: error handling
        return std::int16_t{};
    }
}

}