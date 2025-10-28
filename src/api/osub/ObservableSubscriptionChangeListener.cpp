// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfeed_graal_cpp_api/api/osub/ObservableSubscriptionChangeListener.hpp>

#include <dxfeed_graal_cpp_api/internal/context/ApiContext.hpp>
#include <dxfeed_graal_cpp_api/internal/managers/EntityManager.hpp>

#include <dxfeed_graal_cpp_api/isolated/api/osub/IsolatedObservableSubscriptionChangeListener.hpp>

#include <dxfg_api.h>

DXFCPP_BEGIN_NAMESPACE

struct ObservableSubscriptionChangeListener::Impl {
    static void onSymbolsAdded(graal_isolatethread_t * /* thread */, dxfg_symbol_list *symbols, void *userData) {
        if (!symbols) {
            return;
        }

        auto id = Id<ObservableSubscriptionChangeListener>::from(userData);
        auto listener =
            ApiContext::getInstance()->getManager<EntityManager<ObservableSubscriptionChangeListener>>()->getEntity(id);

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

        auto id = Id<ObservableSubscriptionChangeListener>::from(userData);
        auto listener =
            ApiContext::getInstance()->getManager<EntityManager<ObservableSubscriptionChangeListener>>()->getEntity(id);

        if (!listener) {
            return;
        }

        auto symbolList = SymbolWrapper::SymbolListUtils::fromGraalList(symbols);
        listener->onSymbolsRemoved_({symbolList.begin(), symbolList.end()});
    }

    static void onSubscriptionClosed(graal_isolatethread_t * /* thread */, void *userData) {
        auto id = Id<ObservableSubscriptionChangeListener>::from(userData);
        auto listener =
            ApiContext::getInstance()->getManager<EntityManager<ObservableSubscriptionChangeListener>>()->getEntity(id);

        if (!listener) {
            return;
        }

        listener->onSubscriptionClosed_();
    }
};

ObservableSubscriptionChangeListener::ObservableSubscriptionChangeListener(LockExternalConstructionTag)
    : handle_{}, impl_{std::make_unique<ObservableSubscriptionChangeListener::Impl>()} {
}

ObservableSubscriptionChangeListener::~ObservableSubscriptionChangeListener() noexcept {
}

std::shared_ptr<ObservableSubscriptionChangeListener> ObservableSubscriptionChangeListener::create(
    std::function<void(const std::unordered_set<SymbolWrapper> &symbols)> onSymbolsAdded) {
    auto listener = ObservableSubscriptionChangeListener::createShared();
    auto id =
        ApiContext::getInstance()->getManager<EntityManager<ObservableSubscriptionChangeListener>>()->registerEntity(
            listener);

    listener->handle_ = isolated::api::IsolatedObservableSubscriptionChangeListener::create(
        dxfcpp::bit_cast<void *>(&ObservableSubscriptionChangeListener::Impl::onSymbolsAdded),
        dxfcpp::bit_cast<void *>(&ObservableSubscriptionChangeListener::Impl::onSymbolsRemoved),
        dxfcpp::bit_cast<void *>(&ObservableSubscriptionChangeListener::Impl::onSubscriptionClosed),
        dxfcpp::bit_cast<void *>(id.getValue()));
    listener->onSymbolsAdded_ += std::move(onSymbolsAdded);

    return listener;
}

std::shared_ptr<ObservableSubscriptionChangeListener> ObservableSubscriptionChangeListener::create(
    std::function<void(const std::unordered_set<SymbolWrapper> &symbols)> onSymbolsAdded,
    std::function<void(const std::unordered_set<SymbolWrapper> &symbols)> onSymbolsRemoved,
    std::function<void()> onSubscriptionClosed) {
    auto listener = ObservableSubscriptionChangeListener::createShared();
    auto id =
        ApiContext::getInstance()->getManager<EntityManager<ObservableSubscriptionChangeListener>>()->registerEntity(
            listener);

    listener->handle_ = isolated::api::IsolatedObservableSubscriptionChangeListener::create(
        dxfcpp::bit_cast<void *>(&ObservableSubscriptionChangeListener::Impl::onSymbolsAdded),
        dxfcpp::bit_cast<void *>(&ObservableSubscriptionChangeListener::Impl::onSymbolsRemoved),
        dxfcpp::bit_cast<void *>(&ObservableSubscriptionChangeListener::Impl::onSubscriptionClosed),
        dxfcpp::bit_cast<void *>(id.getValue()));
    listener->onSymbolsAdded_ += std::move(onSymbolsAdded);
    listener->onSymbolsRemoved_ += std::move(onSymbolsRemoved);
    listener->onSubscriptionClosed_ += std::move(onSubscriptionClosed);

    return listener;
}

const JavaObjectHandle<ObservableSubscriptionChangeListener> &ObservableSubscriptionChangeListener::getHandle(Key) const {
    std::lock_guard guard{mutex_};

    return handle_;
}

DXFCPP_END_NAMESPACE