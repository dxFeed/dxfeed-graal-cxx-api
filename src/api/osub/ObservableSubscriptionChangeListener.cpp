// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../../include/dxfeed_graal_cpp_api/api/osub/ObservableSubscriptionChangeListener.hpp"

#include "../../../include/dxfeed_graal_cpp_api/internal/context/ApiContext.hpp"
#include "../../../include/dxfeed_graal_cpp_api/internal/managers/EntityManager.hpp"
#include "../../../include/dxfeed_graal_cpp_api/isolated/api/osub/IsolatedObservableSubscriptionChangeListener.hpp"

#include <dxfg_api.h>

DXFCPP_BEGIN_NAMESPACE

using ObservableSubscriptionChangeListenerManager = WeakEntityManager<ObservableSubscriptionChangeListener>;

struct ObservableSubscriptionChangeListener::Impl {
    static void onSymbolsAdded(graal_isolatethread_t * /* thread */, dxfg_symbol_list *symbols, void *userData) {
        if (!symbols) {
            return;
        }

        const auto id = Id<ObservableSubscriptionChangeListener>::from(userData);
        const auto listener =
            ApiContext::getInstance()->getManager<ObservableSubscriptionChangeListenerManager>()->getEntity(id);

        if (!listener) {
            return;
        }

        auto symbolList = SymbolWrapper::SymbolListUtils::fromGraalList(symbols);
        listener->onSymbolsAdded_({symbolList.begin(), symbolList.end()});
    }

    static void onSymbolsRemoved(graal_isolatethread_t * /* thread */, dxfg_symbol_list *symbols, void *userData) {
        if (!symbols) {
            return;
        }

        const auto id = Id<ObservableSubscriptionChangeListener>::from(userData);
        const auto listener =
            ApiContext::getInstance()->getManager<ObservableSubscriptionChangeListenerManager>()->getEntity(id);

        if (!listener) {
            return;
        }

        auto symbolList = SymbolWrapper::SymbolListUtils::fromGraalList(symbols);
        listener->onSymbolsRemoved_({symbolList.begin(), symbolList.end()});
    }

    static void onSubscriptionClosed(graal_isolatethread_t * /* thread */, void *userData) {
        const auto id = Id<ObservableSubscriptionChangeListener>::from(userData);
        const auto listener =
            ApiContext::getInstance()->getManager<ObservableSubscriptionChangeListenerManager>()->getEntity(id);

        if (!listener) {
            return;
        }

        listener->onSubscriptionClosed_();
    }
};

ObservableSubscriptionChangeListener::ObservableSubscriptionChangeListener(LockExternalConstructionTag)
    : impl_{std::make_unique<Impl>()} {
}

ObservableSubscriptionChangeListener::~ObservableSubscriptionChangeListener() noexcept {
    ApiContext::getInstance()->getManager<ObservableSubscriptionChangeListenerManager>()->unregisterEntity(id_);
}

std::shared_ptr<ObservableSubscriptionChangeListener> ObservableSubscriptionChangeListener::create(
    std::function<void(const std::unordered_set<SymbolWrapper> &symbols)> onSymbolsAdded) {
    auto listener = createShared();
    listener->id_ =
        ApiContext::getInstance()->getManager<ObservableSubscriptionChangeListenerManager>()->registerEntity(listener);

    listener->handle_ = isolated::api::IsolatedObservableSubscriptionChangeListener::create(
        dxfcpp::bit_cast<void *>(&Impl::onSymbolsAdded),
        dxfcpp::bit_cast<void *>(&Impl::onSymbolsRemoved),
        dxfcpp::bit_cast<void *>(&Impl::onSubscriptionClosed),
        dxfcpp::bit_cast<void *>(listener->id_.getValue()));
    listener->onSymbolsAdded_ += std::move(onSymbolsAdded);

    return listener;
}

std::shared_ptr<ObservableSubscriptionChangeListener> ObservableSubscriptionChangeListener::create(
    std::function<void(const std::unordered_set<SymbolWrapper> &symbols)> onSymbolsAdded,
    std::function<void(const std::unordered_set<SymbolWrapper> &symbols)> onSymbolsRemoved,
    std::function<void()> onSubscriptionClosed) {
    auto listener = createShared();
    listener->id_ =
        ApiContext::getInstance()->getManager<ObservableSubscriptionChangeListenerManager>()->registerEntity(listener);

    listener->handle_ = isolated::api::IsolatedObservableSubscriptionChangeListener::create(
        dxfcpp::bit_cast<void *>(&Impl::onSymbolsAdded),
        dxfcpp::bit_cast<void *>(&Impl::onSymbolsRemoved),
        dxfcpp::bit_cast<void *>(&Impl::onSubscriptionClosed),
        dxfcpp::bit_cast<void *>(listener->id_.getValue()));
    listener->onSymbolsAdded_ += std::move(onSymbolsAdded);
    listener->onSymbolsRemoved_ += std::move(onSymbolsRemoved);
    listener->onSubscriptionClosed_ += std::move(onSubscriptionClosed);

    return listener;
}

const JavaObjectHandle<ObservableSubscriptionChangeListener> &ObservableSubscriptionChangeListener::getHandle(Key) const {
    std::lock_guard guard{mutex_};

    return handle_;
}

void ObservableSubscriptionChangeListener::notifySymbolsAdded(
    Key, const std::unordered_set<SymbolWrapper> &symbols) {
    onSymbolsAdded_(symbols);
}

void ObservableSubscriptionChangeListener::notifySymbolsRemoved(
    Key, const std::unordered_set<SymbolWrapper> &symbols) {
    onSymbolsRemoved_(symbols);
}

void ObservableSubscriptionChangeListener::notifySubscriptionClosed(Key) {
    onSubscriptionClosed_();
}

DXFCPP_END_NAMESPACE
