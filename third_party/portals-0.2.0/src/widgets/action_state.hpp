// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0
#pragma once

#include <atomic>
#include <portals/action.hpp>
#include <thread>

namespace portals::detail {
// Worker-side tokens share only this admission flag. They never own callback captures,
// whose potentially reentrant destructors belong to UI collection or active delivery.
struct ActionCancellation {
  std::atomic<bool> connected{true};
};

struct ActionRegistration {
  Node<Widget> node;
  bool focusable{};
  std::shared_ptr<ActionCancellation> cancellation;
  std::function<std::expected<EventReply, TreeError>(const RoutedEvent &)> callback;
};
} // namespace portals::detail

namespace portals {
struct EventRouter::State {
  WidgetTree &tree;
  EventRouterOptions options;
  std::thread::id owner{std::this_thread::get_id()};
  std::vector<std::shared_ptr<detail::ActionRegistration>> registrations =
    std::vector<std::shared_ptr<detail::ActionRegistration>>(0);
  std::vector<Node<Widget>> history = std::vector<Node<Widget>>(0);
  Node<Widget> focus;
  // These UI-only guards reject nested delivery and mutations during capture reclamation;
  // callback disconnect uses the independent atomic flag and remains available on any thread.
  bool dispatching{};
  bool collecting{};
  bool closing{};
};
} // namespace portals
