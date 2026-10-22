// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../include/dxfeed_graal_cpp_api/event/IndexedEventSource.hpp"

#include <dxfg_api.h>

DXFCPP_BEGIN_NAMESPACE

constinit const IndexedEventSource IndexedEventSource::DEFAULT{0, ConstString::Literal{"DEFAULT"}};

void *IndexedEventSource::toGraal() const {
    auto *graalSource = new dxfg_indexed_event_source_t{orderSource_ ? ORDER_SOURCE : INDEXED_EVENT_SOURCE, id_,
                                                        createCString(name_.view())};

    return graalSource;
}

void IndexedEventSource::freeGraal(void *graalNative) {
    if (graalNative == nullptr) {
        return;
    }

    const auto *graalSource = static_cast<dxfg_indexed_event_source_t *>(graalNative);

    delete[] graalSource->name;
    delete graalSource;
}

std::unique_ptr<void, decltype(&IndexedEventSource::freeGraal)> IndexedEventSource::toGraalUnique() const noexcept {
    return {toGraal(), freeGraal};
}

IndexedEventSource IndexedEventSource::fromGraal(void *graalNative) {
    auto *graalSource = static_cast<dxfg_indexed_event_source_t *>(graalNative);

    if (graalSource == nullptr) {
        return {};
    }

    return {graalSource->id, graalSource->name};
}

IndexedEventSource::IndexedEventSource() noexcept {
}

IndexedEventSource::IndexedEventSource(std::int32_t id, const StringLike &name) noexcept
    : IndexedEventSource(id, name, false) {
}

IndexedEventSource::IndexedEventSource(std::int32_t id, const StringLike &name, bool orderSource) noexcept
    : id_{id}, name_{ConstString::copyOf(name)}, orderSource_{orderSource} {
}

IndexedEventSource::IndexedEventSource(const IndexedEventSource &other) noexcept : IndexedEventSource(other, false) {
}

IndexedEventSource::IndexedEventSource(const IndexedEventSource &other, bool orderSource) noexcept
    : id_{other.id_}, name_{other.name_}, orderSource_{orderSource} {
}

IndexedEventSource &IndexedEventSource::operator=(const IndexedEventSource &other) noexcept {
    id_ = other.id_;
    name_ = other.name_;

    return *this;
}

std::int32_t IndexedEventSource::id() const noexcept {
    return id_;
}

std::string IndexedEventSource::name() const {
    return name_.str();
}

std::string IndexedEventSource::toString() const {
    return name_.str();
}

bool IndexedEventSource::operator==(const IndexedEventSource &indexedEventSource) const {
    return id_ == indexedEventSource.id_;
}

bool IndexedEventSource::operator<(const IndexedEventSource &indexedEventSource) const {
    return id_ < indexedEventSource.id_;
}

DXFCPP_END_NAMESPACE