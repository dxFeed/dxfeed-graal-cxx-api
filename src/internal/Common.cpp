// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include <cstring>
#include <memory>
#include <utf8.h>
#include <utility>

#include <fmt/chrono.h>
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <fmt/std.h>

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

template <typename T>
concept RawGraalList = requires(T list) {
    { list.size };
    { list.elements };
};

template <RawGraalList List> struct RawGraalListTraits {
    using SizeType = typename std::decay_t<decltype(List{}.size)>;
    using ElementType = typename std::decay_t<std::remove_pointer_t<std::remove_pointer_t<decltype(List{}.elements)>>>;
};

template <RawGraalList List, auto ElementSetter> struct RawListWrapper {
#if DXFCPP_DEBUG == 1
    static auto getDebugName() {
        return fmt::format("RawListWrapper<{}, {}>", typeid(List).name(),
                           typeid(ElementSetter).name());
    }
#endif

    mutable List list_;

    RawListWrapper() noexcept : list_{0, nullptr} {
        if constexpr (Debugger::traceLists) {
            Debugger::trace("{}::()", getDebugName());
        }
    }

    void set(std::size_t index, auto value) const noexcept {
        if constexpr (Debugger::traceLists) {
            Debugger::trace("{}::set({}, {})", getDebugName(), index, value);
        }

        if (list_.size == 0) {
            if constexpr (Debugger::traceLists) {
                Debugger::trace("{}::set({}, {}): list_.size == 0 ", getDebugName(), index, value);
            }

            return;
        }

        if (index < list_.size) {
            if constexpr (Debugger::traceLists) {
                Debugger::trace("{}::set({}, {}): index < list_.size ", getDebugName(), index, value);
            }

            ElementSetter(list_, index, value);
        }
    }

    [[nodiscard]] bool isEmpty() const noexcept {
        if constexpr (Debugger::traceLists) {
            Debugger::trace("{}::isEmpty() -> ", getDebugName(), (list_.size == 0));
        }

        return list_.size == 0;
    }

    [[nodiscard]] std::size_t size() const noexcept {
        if constexpr (Debugger::traceLists) {
            Debugger::trace("{}::size() -> ", getDebugName(), (static_cast<std::size_t>(list_.size)));
        }

        return static_cast<std::size_t>(list_.size);
    }

    void *getHandler() noexcept {
        if constexpr (Debugger::traceLists) {
            Debugger::trace("{}::getHandler() -> ", getDebugName(), (bit_cast<void *>(&list_)));
        }

        return bit_cast<void *>(&list_);
    }

    void init(std::size_t size) noexcept {
        if constexpr (Debugger::traceLists) {
            Debugger::trace("{}::init({})", getDebugName(), size);
        }

        if (size <= 0) {
            if constexpr (Debugger::traceLists) {
                Debugger::trace("{}::init({}): size <= 0", getDebugName());
            }

            return;
        }

        list_.size = static_cast<std::int32_t>(size);
        list_.elements = new (std::nothrow) RawGraalListTraits<List>::ElementType *[list_.size];

        if (!list_.elements) {
            if constexpr (Debugger::traceLists) {
                Debugger::trace("{}::init({}): !list_.elements", getDebugName());
            }

            release();

            return;
        }

        bool needToRelease = false;

        for (std::int32_t i = 0; i < list_.size; i++) {
            list_.elements[i] = new (std::nothrow) RawGraalListTraits<List>::ElementType{};

            if (!list_.elements[i]) {
                needToRelease = true;
            }
        }

        if (needToRelease) {
            if constexpr (Debugger::traceLists) {
                Debugger::trace("{}::init({}): needToRelease", getDebugName());
            }

            release();
        }
    }

    void release() {
        if constexpr (Debugger::traceLists) {
            Debugger::trace("{}::release()", getDebugName());
        }

        if (list_.size == 0 || list_.elements == nullptr) {
            if constexpr (Debugger::traceLists) {
                Debugger::trace(
                    "{}::release(): list_.size == 0 || list_.elements == nullptr",
                    getDebugName());
            }

            return;
        }

        for (auto i = list_.size - 1; i >= 0; i--) {
            delete list_.elements[i];
        }

        delete[] list_.elements;
        list_.size = 0;
        list_.elements = nullptr;
    }

    ~RawListWrapper() noexcept {
        if constexpr (Debugger::traceLists) {
            Debugger::trace("{}::~()", getDebugName());
        }

        release();
    }
};

struct EventClassList::Impl
    : public RawListWrapper<dxfg_event_clazz_list_t,
                            [](dxfg_event_clazz_list_t &list, std::size_t index, std::uint32_t id) {
                                *list.elements[index] = static_cast<dxfg_event_clazz_t>(id);
                            }> {};

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

EventClassList::~EventClassList() noexcept = default;

} // namespace handler_utils

std::string toString(bool b) {
    return b ? "true" : "false";
}

std::string toString(const char *chars) {
    if (chars == nullptr) {
        return "";
    }

    return chars;
}

std::string toString(std::thread::id threadId) {
    std::ostringstream result{};

    result << threadId;

    return result.str();
}

std::string toString(void* ptr) {
    std::ostringstream result{};

    result << ptr;

    return result.str();
}

std::string string_util::encodeChar(std::int16_t c) {
    if (c >= 32 && c <= 126) {
        return std::string{} + static_cast<char>(c);
    }

    if (c == 0) {
        return "\\0";
    }

    return fmt::format("\\u{:04x}", c);
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

std::string formatTimeStamp(std::int64_t timestamp) {
    auto tm = fmt::localtime(static_cast<std::time_t>(timestamp / 1000));

    return fmt::format("{:%y%m%d-%H%M%S%z}", tm);
}

std::string formatTimeStampWithMillis(std::int64_t timestamp) {
    auto ms = timestamp % 1000;
    auto tm = fmt::localtime(static_cast<std::time_t>(timestamp / 1000));

    return fmt::format("{:%y%m%d-%H%M%S}.{:0>3}{:%z}", tm, ms, tm);
}

} // namespace dxfcpp