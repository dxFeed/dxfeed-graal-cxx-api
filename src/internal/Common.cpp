// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include <cstring>
#include <memory>
#include <utf8.h>
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

//TODO: RawListSizeType
template <typename RawListType, typename RawListElementType, typename IndexType, typename ValueType, auto ElementSetter>
struct RawListWrapper {
    mutable RawListType list_;

    RawListWrapper() noexcept : list_{0, nullptr} {
        if constexpr (traceLists) {
            trace("RawListWrapper<{}, {}, {}, {}>::()", typeid(RawListType).name(), typeid(RawListElementType).name(),
                  typeid(IndexType).name(), typeid(ValueType).name());
        }
    }

    void set(IndexType index, ValueType value) const noexcept {
        if constexpr (traceLists) {
            trace("RawListWrapper<{}, {}, {}, {}>::set({}, {})", typeid(RawListType).name(),
                  typeid(RawListElementType).name(), typeid(IndexType).name(), typeid(ValueType).name(), index, value);
        }

        if (list_.size == 0) {
            if constexpr (traceLists) {
                trace("RawListWrapper<{}, {}, {}, {}>::set({}, {}): list_.size == 0 ", typeid(RawListType).name(),
                      typeid(RawListElementType).name(), typeid(IndexType).name(), typeid(ValueType).name(), index,
                      value);
            }

            return;
        }

        if (index < list_.size) {
            if constexpr (traceLists) {
                trace("RawListWrapper<{}, {}, {}, {}>::set({}, {}): index < list_.size ", typeid(RawListType).name(),
                      typeid(RawListElementType).name(), typeid(IndexType).name(), typeid(ValueType).name(), index,
                      value);
            }

            ElementSetter(list_, index, value);
        }
    }

    [[nodiscard]] bool isEmpty() const noexcept {
        if constexpr (traceLists) {
            trace("RawListWrapper<{}, {}, {}, {}>::isEmpty() -> ", typeid(RawListType).name(),
                  typeid(RawListElementType).name(), typeid(IndexType).name(), typeid(ValueType).name(),
                  (list_.size == 0));
        }

        return list_.size == 0;
    }

    [[nodiscard]] IndexType size() const noexcept {
        if constexpr (traceLists) {
            trace("RawListWrapper<{}, {}, {}, {}>::size() -> ", typeid(RawListType).name(),
                  typeid(RawListElementType).name(), typeid(IndexType).name(), typeid(ValueType).name(),
                  (static_cast<IndexType>(list_.size)));
        }

        return static_cast<std::size_t>(list_.size);
    }

    void *getHandler() noexcept {
        if constexpr (traceLists) {
            trace("RawListWrapper<{}, {}, {}, {}>::getHandler() -> ", typeid(RawListType).name(),
                  typeid(RawListElementType).name(), typeid(IndexType).name(), typeid(ValueType).name(),
                  (bit_cast<void *>(&list_)));
        }

        return bit_cast<void *>(&list_);
    }

    void init(IndexType size) noexcept {
        if constexpr (traceLists) {
            trace("RawListWrapper<{}, {}, {}, {}>::init({})", typeid(RawListType).name(),
                  typeid(RawListElementType).name(), typeid(IndexType).name(), typeid(ValueType).name(), size);
        }

        if (size <= 0) {
            if constexpr (traceLists) {
                trace("RawListWrapper<{}, {}, {}, {}>::init({}): size <= 0", typeid(RawListType).name(),
                      typeid(RawListElementType).name(), typeid(IndexType).name(), typeid(ValueType).name());
            }

            return;
        }

        list_.size = static_cast<std::int32_t>(size);
        list_.elements = new (std::nothrow) RawListElementType *[list_.size];

        if (!list_.elements) {
            if constexpr (traceLists) {
                trace("RawListWrapper<{}, {}, {}, {}>::init({}): !list_.elements", typeid(RawListType).name(),
                      typeid(RawListElementType).name(), typeid(IndexType).name(), typeid(ValueType).name());
            }

            release();

            return;
        }

        bool needToRelease = false;

        for (std::int32_t i = 0; i < list_.size; i++) {
            list_.elements[i] = new (std::nothrow) RawListElementType{};

            if (!list_.elements[i]) {
                needToRelease = true;
            }
        }

        if (needToRelease) {
            if constexpr (traceLists) {
                trace("RawListWrapper<{}, {}, {}, {}>::init({}): needToRelease", typeid(RawListType).name(),
                      typeid(RawListElementType).name(), typeid(IndexType).name(), typeid(ValueType).name());
            }

            release();
        }
    }

    void release() {
        if constexpr (traceLists) {
            trace("RawListWrapper<{}, {}, {}, {}>::release()", typeid(RawListType).name(),
                  typeid(RawListElementType).name(), typeid(IndexType).name(), typeid(ValueType).name());
        }

        if (list_.size == 0 || list_.elements == nullptr) {
            if constexpr (traceLists) {
                trace("RawListWrapper<{}, {}, {}, {}>::release(): list_.size == 0 || list_.elements == nullptr",
                      typeid(RawListType).name(), typeid(RawListElementType).name(), typeid(IndexType).name(),
                      typeid(ValueType).name());
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
        if constexpr (traceLists) {
            trace("RawListWrapper<{}, {}, {}, {}>::~()", typeid(RawListType).name(), typeid(RawListElementType).name(),
                  typeid(IndexType).name(), typeid(ValueType).name());
        }

        release();
    }
};

struct EventClassList::Impl : public RawListWrapper<dxfg_event_clazz_list_t, dxfg_event_clazz_t, std::size_t, std::uint32_t,
                                    [](dxfg_event_clazz_list_t &list, std::size_t index, std::uint32_t id) {
                                        *list.elements[index] = static_cast<dxfg_event_clazz_t>(id);
                                    }> {};

//struct EventClassList::Impl {
//    dxfg_event_clazz_list_t list_;
//
//    Impl() noexcept : list_{0, nullptr} {
//        if constexpr (traceLists) {
//            trace("EventClassList::Impl()");
//        }
//    }
//
//    void set(std::size_t index, std::uint32_t id) const noexcept {
//        if constexpr (traceLists) {
//            trace("EventClassList::Impl::set({}, {})", index, id);
//        }
//
//        if (list_.size == 0) {
//            if constexpr (traceLists) {
//                trace("EventClassList::Impl::set({}, {}): list_.size == 0 ", index, id);
//            }
//
//            return;
//        }
//
//        if (index < list_.size) {
//            if constexpr (traceLists) {
//                trace("EventClassList::Impl::set({}, {}): index < list_.size ", index, id);
//            }
//
//            *list_.elements[index] = static_cast<dxfg_event_clazz_t>(id);
//        }
//    }
//
//    [[nodiscard]] bool isEmpty() const noexcept {
//        if constexpr (traceLists) {
//            trace("EventClassList::Impl::isEmpty() -> ", (list_.size == 0));
//        }
//
//        return list_.size == 0;
//    }
//
//    [[nodiscard]] std::size_t size() const noexcept {
//        if constexpr (traceLists) {
//            trace("EventClassList::Impl::size() -> ", (static_cast<std::size_t>(list_.size)));
//        }
//
//        return static_cast<std::size_t>(list_.size);
//    }
//
//    void *getHandler() noexcept {
//        if constexpr (traceLists) {
//            trace("EventClassList::Impl::getHandler() -> ", (bit_cast<void *>(&list_)));
//        }
//
//        return bit_cast<void *>(&list_);
//    }
//
//    void init(std::uint32_t size) noexcept {
//        if constexpr (traceLists) {
//            trace("EventClassList::Impl::init({})", size);
//        }
//
//        if (size <= 0) {
//            if constexpr (traceLists) {
//                trace("EventClassList::Impl::init({}): size <= 0");
//            }
//
//            return;
//        }
//
//        list_.size = static_cast<std::int32_t>(size);
//        list_.elements = new (std::nothrow) dxfg_event_clazz_t *[list_.size];
//
//        if (!list_.elements) {
//            if constexpr (traceLists) {
//                trace("EventClassList::Impl::init({}): !list_.elements");
//            }
//
//            release();
//
//            return;
//        }
//
//        bool needToRelease = false;
//
//        for (std::int32_t i = 0; i < list_.size; i++) {
//            list_.elements[i] = new (std::nothrow) dxfg_event_clazz_t{};
//
//            if (!list_.elements[i]) {
//                needToRelease = true;
//            }
//        }
//
//        if (needToRelease) {
//            if constexpr (traceLists) {
//                trace("EventClassList::Impl::init({}): needToRelease");
//            }
//
//            release();
//        }
//    }
//
//    void release() {
//        if constexpr (traceLists) {
//            trace("EventClassList::Impl::release()");
//        }
//
//        if (list_.size == 0 || list_.elements == nullptr) {
//            if constexpr (traceLists) {
//                trace("EventClassList::Impl::release(): list_.size == 0 || list_.elements == nullptr");
//            }
//
//            return;
//        }
//
//        for (auto i = list_.size - 1; i >= 0; i--) {
//            delete list_.elements[i];
//        }
//
//        delete[] list_.elements;
//        list_.size = 0;
//        list_.elements = nullptr;
//    }
//
//    ~Impl() noexcept {
//        if constexpr (traceLists) {
//            trace("EventClassList::Impl::~()");
//        }
//
//        release();
//    }
//};

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

} // namespace dxfcpp