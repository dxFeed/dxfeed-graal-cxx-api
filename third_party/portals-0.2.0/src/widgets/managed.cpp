// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0
#include <portals/managed.hpp>

#include <algorithm>

namespace portals {
namespace {
bool retired(TreeError error) {
  return error == TreeError::STALE_NODE || error == TreeError::CLOSED;
}

TreeError bridge(const ManagedError &error) {
  if (const auto *tree = std::get_if<TreeError>(&error.cause)) {
    return *tree;
  }

  if (const auto *style = std::get_if<StyleError>(&error.cause)) {
    return style->cause;
  }

  return TreeError::CALLBACK_FAILED;
}
} // namespace

ManagedScene::ManagedScene(Application &app, ManagedOptions options)
    : app_(app), owner_(std::this_thread::get_id()), options_(options), lifetime_(std::make_shared<const bool>(true)),
      styles_(app.tree(), options.maxNodes) {
}

ManagedError ManagedScene::error(ManagedPhase phase, Node<Widget> node, TreeError cause) {
  return {phase, node.id(), cause, {}, {}};
}

std::expected<std::unique_ptr<ManagedScene>, ManagedError> ManagedScene::create(Application &app,
                                                                                ManagedOptions options) {
  auto allowed = app.tree().withScope(app.root(), [](const UiScope &) {
  });

  if (!allowed) {
    return std::unexpected(error(ManagedPhase::MOUNT, app.root(), allowed.error()));
  }

  if (!options.maxNodes || !options.maxStates || !options.maxModels) {
    return std::unexpected(error(ManagedPhase::MOUNT, app.root(), TreeError::INVALID_ARGUMENT));
  }

  try {
    auto scene = std::unique_ptr<ManagedScene>(new ManagedScene(app, options));
    auto root = app.tree().emplaceChecked<Widget>(app.root());

    if (!root) {
      scene->closed_ = true;

      return std::unexpected(error(ManagedPhase::MOUNT, app.root(), root.error()));
    }

    scene->root_ = *root;

    return scene;
  } catch (const std::bad_alloc &) {
    return std::unexpected(error(ManagedPhase::MOUNT, app.root(), TreeError::NO_MEMORY));
  }
}

ManagedScene::~ManagedScene() {
  if (owner_ != std::this_thread::get_id() || preparing_) {
    std::terminate();
  }

  // Router/scheduler may retain disconnected captures until their next collection.
  // Weak state handles end with this owner even while those internal borrows drain.
  lifetime_.reset();

  auto result = close();

  if (!result && bridge(result.error()) == TreeError::WOULD_BLOCK) {
    std::terminate();
  }
}

std::expected<void, ManagedError> ManagedScene::boundary(ManagedPhase phase, Node<Widget> node) const {
  if (owner_ != std::this_thread::get_id()) {
    return std::unexpected(error(phase, node, TreeError::WRONG_THREAD));
  }

  if (closed_) {
    return std::unexpected(error(phase, node, TreeError::CLOSED));
  }

  if (preparing_) {
    return std::unexpected(error(phase, node, TreeError::WOULD_BLOCK));
  }

  auto allowed = app_.tree().withScope(node, [](const UiScope &) {
  });

  if (!allowed) {
    return std::unexpected(error(phase, node, allowed.error()));
  }

  return {};
}

std::shared_ptr<ManagedScene::Entry> ManagedScene::find(Node<Widget> node) const {
  const auto found = std::ranges::find(entries_, node, &Entry::node);

  return found == entries_.end() ? nullptr : *found;
}

std::expected<std::shared_ptr<ManagedScene::Entry>, ManagedError> ManagedScene::bindingEntry(Node<Widget> node) {
  auto allowed = boundary(ManagedPhase::BIND, node);

  if (!allowed) {
    return std::unexpected(allowed.error());
  }

  auto entry = find(node);

  if (!entry || !std::holds_alternative<std::monostate>(entry->binding)) {
    return std::unexpected(error(ManagedPhase::BIND, node, TreeError::INVALID_ARGUMENT));
  }

  return entry;
}

std::unexpected<ManagedError> ManagedScene::rollback(Node<Widget> node, TreeError cause) {
  auto result = error(ManagedPhase::MOUNT, node, cause);
  auto removed = app_.tree().remove(node);

  if (!removed && !retired(removed.error())) {
    result.cleanup = removed.error();
  }

  return std::unexpected(result);
}

std::expected<void, ManagedError> ManagedScene::remove(Node<Widget> node) {
  auto allowed = boundary(ManagedPhase::CLEANUP, root_);

  if (!allowed) {
    return allowed;
  }

  if (node == root_) {
    return std::unexpected(error(ManagedPhase::CLEANUP, node, TreeError::INVALID_ARGUMENT));
  }

  if (!find(node)) {
    auto live = app_.tree().withScope(node, [](const UiScope &) {
    });

    if (!live && retired(live.error())) {
      return {};
    }

    return std::unexpected(error(ManagedPhase::CLEANUP, node, live ? TreeError::INVALID_ARGUMENT : live.error()));
  }

  struct Removing {
    bool &flag;

    ~Removing() {
      flag = false;
    }
  } guard{preparing_};

  preparing_ = true;

  auto removed = app_.tree().remove(node);

  if (!removed && !retired(removed.error())) {
    return std::unexpected(error(ManagedPhase::CLEANUP, node, removed.error()));
  }

  // Logical retirement is immediate even when the router still borrows an admitted callback.
  // Do not call style hooks during rollback: pruning on its next transaction is allocation-safe.
  std::erase_if(entries_, [&](const auto &entry) {
    auto live = app_.tree().withScope(entry->node, [](const UiScope &) {
    });

    return !live && retired(live.error());
  });
  stylesDirty_ = true;

  return {};
}

void ManagedScene::remember(ManagedError failure) {
  if (!failure_) {
    failure_ = std::move(failure);
  }

  app_.requestStop();
}

std::expected<void, ManagedError> ManagedScene::onClick(Node<Button> node,
                                                        std::function<std::expected<void, TreeError>()> action) {
  auto entry = bindingEntry(node);

  if (!entry) {
    return std::unexpected(entry.error());
  }

  if (!action) {
    return std::unexpected(error(ManagedPhase::BIND, node, TreeError::INVALID_ARGUMENT));
  }

  auto bound = bindButton(app_.router(), node, [this, node, action = std::move(action)] {
    try {
      auto result = action();

      if (!result) {
        remember(error(ManagedPhase::INPUT, node, result.error()));
      }
    } catch (const std::bad_alloc &) {
      remember(error(ManagedPhase::INPUT, node, TreeError::NO_MEMORY));
    } catch (...) {
      remember(error(ManagedPhase::INPUT, node, TreeError::CALLBACK_FAILED));
    }
  });

  if (!bound) {
    return std::unexpected(error(ManagedPhase::BIND, node, bound.error()));
  }

  (*entry)->binding.emplace<ActionBinding>(std::move(*bound));

  return {};
}

std::expected<void, ManagedError>
ManagedScene::bind(Node<CheckBox> node, ManagedState<bool> model,
                   std::function<std::expected<void, TreeError>(CheckBoxResult)> changed) {
  auto entry = bindingEntry(node);

  if (!entry) {
    return std::unexpected(entry.error());
  }

  auto property = model.lock();

  if (!property || model.scene_ != root_.id()) {
    return std::unexpected(error(ManagedPhase::BIND, node, property ? TreeError::INVALID_ARGUMENT : property.error()));
  }

  auto bound = bindCheckbox(app_.tree(), app_.router(), node, *property,
                            [this, node, changed = std::move(changed)](CheckBoxResult receipt) {
                              try {
                                if (!receipt) {
                                  remember(error(ManagedPhase::INPUT, node, receipt.error()));
                                }

                                if (changed) {
                                  auto result = changed(std::move(receipt));

                                  if (!result) {
                                    remember(error(ManagedPhase::INPUT, node, result.error()));
                                  }
                                }
                              } catch (const std::bad_alloc &) {
                                remember(error(ManagedPhase::INPUT, node, TreeError::NO_MEMORY));
                              } catch (...) {
                                remember(error(ManagedPhase::INPUT, node, TreeError::CALLBACK_FAILED));
                              }
                            });

  if (!bound) {
    return std::unexpected(error(ManagedPhase::BIND, node, bound.error()));
  }

  (*entry)->binding.emplace<CheckBoxBinding>(std::move(*bound));

  return {};
}

std::expected<void, ManagedError>
ManagedScene::bind(Node<LineEdit> node, ManagedState<std::string> model,
                   std::function<std::expected<void, TreeError>(LineEditResult)> result,
                   std::shared_ptr<Clipboard> clipboard, std::function<bool(std::string_view)> validate) {
  auto entry = bindingEntry(node);

  if (!entry) {
    return std::unexpected(entry.error());
  }

  auto property = model.lock();

  if (!property || model.scene_ != root_.id()) {
    return std::unexpected(error(ManagedPhase::BIND, node, property ? TreeError::INVALID_ARGUMENT : property.error()));
  }

  auto bound = bindLineEdit(
    app_.tree(), app_.router(), node, *property,
    [this, node, result = std::move(result)](LineEditResult receipt) {
      try {
        if (receipt.status == LineEditStatus::FAILED && receipt.error) {
          remember(error(ManagedPhase::INPUT, node, *receipt.error));
        }

        if (result) {
          auto status = result(std::move(receipt));

          if (!status) {
            remember(error(ManagedPhase::INPUT, node, status.error()));
          }
        }
      } catch (const std::bad_alloc &) {
        remember(error(ManagedPhase::INPUT, node, TreeError::NO_MEMORY));
      } catch (...) {
        remember(error(ManagedPhase::INPUT, node, TreeError::CALLBACK_FAILED));
      }
    },
    std::move(clipboard), std::move(validate));

  if (!bound) {
    return std::unexpected(error(ManagedPhase::BIND, node, bound.error()));
  }

  (*entry)->binding.emplace<LineEditBinding>(std::move(*bound));

  return {};
}

std::expected<void, ManagedError> ManagedScene::style(Node<Widget> node, StyleTarget target) {
  auto allowed = boundary(ManagedPhase::STYLE, node);

  if (!allowed) {
    return allowed;
  }

  auto entry = find(node);

  if (!entry) {
    return std::unexpected(error(ManagedPhase::STYLE, node, TreeError::INVALID_ARGUMENT));
  }

  auto dirty = app_.tree().invalidate(node, Invalidation::PAINT);

  if (!dirty) {
    return std::unexpected(error(ManagedPhase::STYLE, node, dirty.error()));
  }

  entry->target = std::move(target);
  entry->stylePending = true;
  stylesDirty_ = true;

  return {};
}

std::expected<void, ManagedError> ManagedScene::flushStyles() {
  if (!stylesDirty_) {
    return {};
  }

  try {
    std::vector<StyleRegistration> registrations;

    registrations.reserve(entries_.size());

    for (const auto &entry : entries_) {
      auto live = app_.tree().read(entry->node, [](const Widget &) {
      });

      if (!live) {
        if (retired(live.error())) {
          continue;
        }

        return std::unexpected(error(ManagedPhase::STYLE, entry->node, live.error()));
      }

      if (entry->target && entry->stylePending) {
        registrations.push_back({entry->node, *entry->target});
      }
    }

    auto installed = styles_.bindBatch(registrations);

    if (!installed) {
      return std::unexpected(ManagedError{ManagedPhase::STYLE, root_.id(), installed.error(), {}, {}});
    }

    for (const auto &entry : entries_) {
      entry->stylePending = false;
    }

    stylesDirty_ = false;

    return {};
  } catch (const std::bad_alloc &) {
    return std::unexpected(error(ManagedPhase::STYLE, root_, TreeError::NO_MEMORY));
  }
}

std::expected<void, ManagedError> ManagedScene::theme(StyleSheet sheet) {
  auto allowed = boundary(ManagedPhase::STYLE, root_);

  if (!allowed) {
    return allowed;
  }

  auto flushed = flushStyles();

  if (!flushed) {
    return flushed;
  }

  auto installed = styles_.replace(std::move(sheet));

  if (!installed) {
    return std::unexpected(ManagedError{ManagedPhase::STYLE, root_.id(), installed.error(), {}, {}});
  }

  return {};
}

std::expected<void, ManagedError>
ManagedScene::onInput(std::function<std::expected<bool, ManagedError>(const InputEvent &)> callback) {
  auto allowed = boundary(ManagedPhase::BIND, root_);

  if (!allowed) {
    return allowed;
  }

  try {
    input_ = callback ? std::make_shared<decltype(callback)>(std::move(callback)) : nullptr;
  } catch (const std::bad_alloc &) {
    return std::unexpected(error(ManagedPhase::BIND, root_, TreeError::NO_MEMORY));
  }

  return {};
}

std::expected<void, TreeError> ManagedScene::prepare(Application &app, CellSize) {
  if (owner_ != std::this_thread::get_id()) {
    return std::unexpected(TreeError::WRONG_THREAD);
  }

  auto allowed = boundary(ManagedPhase::PREPARE, root_);

  if (!allowed) {
    remember(allowed.error());

    return std::unexpected(bridge(allowed.error()));
  }

  if (&app != &app_) {
    return std::unexpected(TreeError::INVALID_ARGUMENT);
  }

  struct Preparing {
    bool &flag;

    ~Preparing() {
      flag = false;
    }
  } guard{preparing_};

  preparing_ = true;

  try {
    // The preparing guard rejects registry changes; prune only after every active borrow ends.
    struct Prune {
      std::vector<std::shared_ptr<Entry>> &entries;

      ~Prune() {
        std::erase_if(entries, [](const auto &entry) {
          return entry->removed;
        });
      }
    } prune{entries_};

    auto focused = app_.router().focused();

    if (!focused) {
      remember(error(ManagedPhase::PREPARE, root_, focused.error()));

      return std::unexpected(focused.error());
    }

    for (const auto &entry : entries_) {
      auto live = app_.tree().read(entry->node, [](const Widget &) {
      });

      if (!live && retired(live.error())) {
        stylesDirty_ = true;
        entry->removed = true;

        continue;
      }

      std::expected<void, TreeError> status = live;

      if (status) {
        if (auto *edit = std::get_if<LineEditBinding>(&entry->binding)) {
          status = edit->sync(*focused);
        } else if (auto *check = std::get_if<CheckBoxBinding>(&entry->binding)) {
          status = check->sync();
        }
      }

      if (status && entry->sync) {
        status = entry->sync(*focused);
      }

      if (status && entry->customSync) {
        status = entry->customSync(*focused);
      }

      if (!status) {
        // Retirement cannot erase an explicit error from an admitted callback.
        remember(error(ManagedPhase::PREPARE, entry->node, status.error()));

        return status;
      }
    }

    for (const auto &source : models_) {
      auto pinned = source->pin();

      if (!pinned) {
        remember(error(ManagedPhase::PREPARE, root_, pinned.error()));

        return pinned;
      }
    }

    auto styled = flushStyles();

    if (!styled) {
      remember(styled.error());

      return std::unexpected(bridge(styled.error()));
    }

    return {};
  } catch (const std::bad_alloc &) {
    remember(error(ManagedPhase::PREPARE, root_, TreeError::NO_MEMORY));

    return std::unexpected(TreeError::NO_MEMORY);
  } catch (...) {
    remember(error(ManagedPhase::PREPARE, root_, TreeError::CALLBACK_FAILED));

    return std::unexpected(TreeError::CALLBACK_FAILED);
  }
}

bool ManagedScene::usesPreparedLayout() const noexcept {
  return std::ranges::any_of(entries_, [](const auto &entry) {
    return static_cast<bool>(entry->viewPrepare);
  });
}

std::expected<void, TreeError> ManagedScene::prepareViews(Application &app, const LayoutSnapshot &snapshot) {
  auto allowed = boundary(ManagedPhase::PREPARE, root_);

  if (!allowed) {
    return std::unexpected(bridge(allowed.error()));
  }

  if (&app != &app_) {
    return std::unexpected(TreeError::INVALID_ARGUMENT);
  }

  struct Preparing {
    bool &flag;

    ~Preparing() {
      flag = false;
    }
  } guard{preparing_};

  preparing_ = true;

  for (const auto &entry : entries_) {
    if (!entry->viewPrepare) {
      continue;
    }

    // Low-level removal from an earlier callback can retire a later sibling.
    // Skip only retired identities; never erase an admitted callback's explicit error.
    auto live = app_.tree().read(entry->node, [](const Widget &) {
    });

    if (!live && retired(live.error())) {
      continue;
    }

    std::expected<void, TreeError> result = live;

    try {
      if (result) {
        result = entry->viewPrepare(snapshot);
      }
    } catch (const std::bad_alloc &) {
      result = std::unexpected(TreeError::NO_MEMORY);
    } catch (...) {
      result = std::unexpected(TreeError::CALLBACK_FAILED);
    }

    if (!result) {
      remember(error(ManagedPhase::PREPARE, entry->node, result.error()));

      return result;
    }
  }

  return {};
}

std::expected<bool, TreeError> ManagedScene::input(Application &app, const InputEvent &event) {
  if (owner_ != std::this_thread::get_id()) {
    return std::unexpected(TreeError::WRONG_THREAD);
  }

  auto allowed = boundary(ManagedPhase::INPUT, root_);

  if (!allowed) {
    remember(allowed.error());

    return std::unexpected(bridge(allowed.error()));
  }

  if (&app != &app_) {
    return std::unexpected(TreeError::INVALID_ARGUMENT);
  }

  try {
    // Pin the callback even if it replaces the interceptor or closes its subtree.
    auto callback = input_;

    if (!callback) {
      return false;
    }

    auto result = (*callback)(event);

    if (!result) {
      remember(result.error());

      return std::unexpected(bridge(result.error()));
    }

    return *result;
  } catch (const std::bad_alloc &) {
    remember(error(ManagedPhase::INPUT, root_, TreeError::NO_MEMORY));

    return std::unexpected(TreeError::NO_MEMORY);
  } catch (...) {
    remember(error(ManagedPhase::INPUT, root_, TreeError::CALLBACK_FAILED));

    return std::unexpected(TreeError::CALLBACK_FAILED);
  }
}

std::expected<void, ManagedError> ManagedScene::close() {
  if (owner_ != std::this_thread::get_id()) {
    return std::unexpected(error(ManagedPhase::CLEANUP, root_, TreeError::WRONG_THREAD));
  }

  if (preparing_) {
    return std::unexpected(error(ManagedPhase::CLEANUP, root_, TreeError::WOULD_BLOCK));
  }

  if (closed_) {
    return {};
  }

  auto removed = app_.tree().remove(root_);

  if (!removed && !retired(removed.error())) {
    return std::unexpected(error(ManagedPhase::CLEANUP, root_, removed.error()));
  }

  // Mark closed before retiring captures: their destructors may reenter application code.
  closed_ = true;
  entries_.clear();
  models_.clear();
  input_ = {};

  return {};
}

void ManagedScene::stop(Application &app) {
  if (&app != &app_) {
    cleanup_ = TreeError::INVALID_ARGUMENT;

    return;
  }

  auto result = close();

  if (!result) {
    cleanup_ = bridge(result.error());
  }
}

std::expected<ApplicationStats, ManagedError>
ManagedScene::finish(std::expected<ApplicationStats, ApplicationError> result) {
  if (failure_) {
    auto failure = *failure_;

    if (!result) {
      failure.application = result.error();
    }

    failure.cleanup = cleanup_;

    return std::unexpected(failure);
  }

  if (!result) {
    return std::unexpected(ManagedError{ManagedPhase::RUN, root_.id(), result.error(), result.error(), cleanup_});
  }

  if (cleanup_) {
    return std::unexpected(ManagedError{ManagedPhase::CLEANUP, root_.id(), *cleanup_, {}, cleanup_});
  }

  return *result;
}

std::expected<ApplicationStats, ManagedError> ManagedScene::run(std::stop_token stop) {
  auto allowed = boundary(ManagedPhase::RUN, root_);

  if (!allowed) {
    return std::unexpected(allowed.error());
  }

  return finish(app_.run(*this, stop));
}

std::expected<ApplicationStats, ManagedError> ManagedScene::run(ApplicationTerminal &terminal, std::stop_token stop) {
  auto allowed = boundary(ManagedPhase::RUN, root_);

  if (!allowed) {
    return std::unexpected(allowed.error());
  }

  return finish(app_.run(*this, terminal, stop));
}
} // namespace portals
