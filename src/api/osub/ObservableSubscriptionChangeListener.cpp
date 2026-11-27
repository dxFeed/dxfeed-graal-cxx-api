// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../../include/dxfeed_graal_cpp_api/api/osub/ObservableSubscriptionChangeListener.hpp"

#include "../../internal/CallbackBarrier.hpp"
#include "../../internal/context/ApiContext.hpp"
#include "../../internal/managers/EntityManager.hpp"
#include "../../isolated/api/osub/IsolatedObservableSubscriptionChangeListener.hpp"

#include <dxfg_v4_api.h>

#include <iterator>
#include <unordered_set>

DXFCPP_BEGIN_NAMESPACE

using ObservableSubscriptionChangeListenerManager = WeakEntityManager<ObservableSubscriptionChangeListener>;

struct ObservableSubscriptionChangeListener::Impl {
    // The symbols are moved into a set sized for them at once, without rehashing.
    static std::unordered_set<SymbolWrapper> toSymbolSet(dxfg_v4_symbol_list *symbols) {
        auto symbolList = SymbolWrapper::SymbolListUtils::fromGraalList(symbols);

        // Parentheses, not braces: no initializer_list overload.
        return std::unordered_set<SymbolWrapper>(std::make_move_iterator(symbolList.begin()),
                                                 std::make_move_iterator(symbolList.end()), symbolList.size());
    }

    static void onSymbolsAdded(graal_isolatethread_t * /* thread */, dxfg_v4_symbol_list *symbols, void *userData) {
        runCallback("ObservableSubscriptionChangeListener", [&] {
            if (!symbols) {
                return;
            }

            const auto id = Id<ObservableSubscriptionChangeListener>::from(userData);
            const auto listener =
                ApiContext::getInstance().getManager<ObservableSubscriptionChangeListenerManager>()->getEntity(id);

            if (!listener) {
                return;
            }

            listener->onSymbolsAdded_(toSymbolSet(symbols));
        });
    }

    static void onSymbolsRemoved(graal_isolatethread_t * /* thread */, dxfg_v4_symbol_list *symbols, void *userData) {
        runCallback("ObservableSubscriptionChangeListener", [&] {
            if (!symbols) {
                return;
            }

            const auto id = Id<ObservableSubscriptionChangeListener>::from(userData);
            const auto listener =
                ApiContext::getInstance().getManager<ObservableSubscriptionChangeListenerManager>()->getEntity(id);

            if (!listener) {
                return;
            }

            listener->onSymbolsRemoved_(toSymbolSet(symbols));
        });
    }

    static void onSubscriptionClosed(graal_isolatethread_t * /* thread */, void *userData) {
        runCallback("ObservableSubscriptionChangeListener", [&] {
            const auto id = Id<ObservableSubscriptionChangeListener>::from(userData);
            const auto listener =
                ApiContext::getInstance().getManager<ObservableSubscriptionChangeListenerManager>()->getEntity(id);

            if (!listener) {
                return;
            }

            listener->onSubscriptionClosed_();
        });
    }
};

ObservableSubscriptionChangeListener::ObservableSubscriptionChangeListener(LockExternalConstructionTag)
    : impl_{std::make_unique<Impl>()} {
}

ObservableSubscriptionChangeListener::~ObservableSubscriptionChangeListener() noexcept {
    ApiContext::getInstance().getManager<ObservableSubscriptionChangeListenerManager>()->unregisterEntity(
        Id<ObservableSubscriptionChangeListener>::from(id_));
}

std::shared_ptr<ObservableSubscriptionChangeListener> ObservableSubscriptionChangeListener::create(
    std::function<void(const std::unordered_set<SymbolWrapper> &symbols)> onSymbolsAdded) {
    auto listener = createShared();
    listener->id_ = ApiContext::getInstance()
                        .getManager<ObservableSubscriptionChangeListenerManager>()
                        ->registerEntity(listener)
                        .getValue();

    listener->handle_ = isolated::api::IsolatedObservableSubscriptionChangeListener::create(
        dxfcpp::bit_cast<void *>(&Impl::onSymbolsAdded), dxfcpp::bit_cast<void *>(&Impl::onSymbolsRemoved),
        dxfcpp::bit_cast<void *>(&Impl::onSubscriptionClosed), dxfcpp::bit_cast<void *>(listener->id_));
    listener->onSymbolsAdded_ += std::move(onSymbolsAdded);

    return listener;
}

std::shared_ptr<ObservableSubscriptionChangeListener> ObservableSubscriptionChangeListener::create(
    std::function<void(const std::unordered_set<SymbolWrapper> &symbols)> onSymbolsAdded,
    std::function<void(const std::unordered_set<SymbolWrapper> &symbols)> onSymbolsRemoved,
    std::function<void()> onSubscriptionClosed) {
    auto listener = createShared();
    listener->id_ = ApiContext::getInstance()
                        .getManager<ObservableSubscriptionChangeListenerManager>()
                        ->registerEntity(listener)
                        .getValue();

    listener->handle_ = isolated::api::IsolatedObservableSubscriptionChangeListener::create(
        dxfcpp::bit_cast<void *>(&Impl::onSymbolsAdded), dxfcpp::bit_cast<void *>(&Impl::onSymbolsRemoved),
        dxfcpp::bit_cast<void *>(&Impl::onSubscriptionClosed), dxfcpp::bit_cast<void *>(listener->id_));
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
