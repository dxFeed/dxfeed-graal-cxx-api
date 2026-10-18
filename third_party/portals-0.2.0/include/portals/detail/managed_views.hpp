// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0
#pragma once

// Included by managed.hpp after the scene definition; no additional runtime lives here.
namespace portals {
template <class T>
std::expected<ManagedModel<T>, ManagedError> ManagedScene::model(std::shared_ptr<const ModelSnapshot<T>> initial,
                                                                 ModelOptions options) {
  auto allowed = boundary(ManagedPhase::STATE, root_);

  if (!allowed) {
    return std::unexpected(allowed.error());
  }

  if (models_.size() >= options_.maxModels) {
    return std::unexpected(error(ManagedPhase::STATE, root_, TreeError::NODE_LIMIT));
  }

  try {
    auto created = Model<T>::create(app_.tree(), root_, std::move(initial), options);

    if (!created) {
      return std::unexpected(error(ManagedPhase::STATE, root_, created.error()));
    }

    auto source = std::make_shared<detail::ManagedModelSourceOf<T>>();
    ManagedModel<T> handle;

    source->model = std::move(*created);
    source->scene = root_.id();
    handle.source_ = source;
    handle.owner_ = owner_;
    models_.push_back(std::move(source));

    return handle;
  } catch (const std::bad_alloc &) {
    return std::unexpected(error(ManagedPhase::STATE, root_, TreeError::NO_MEMORY));
  }
}

template <class T>
std::expected<std::shared_ptr<detail::ManagedModelSourceOf<T>>, ManagedError>
ManagedScene::modelSource(ManagedModel<T> model, Node<Widget> parent) {
  auto allowed = boundary(ManagedPhase::MOUNT, parent);

  if (!allowed) {
    return std::unexpected(allowed.error());
  }

  auto source = model.lock();

  if (!source) {
    return std::unexpected(error(ManagedPhase::MOUNT, parent, source.error()));
  }

  if ((*source)->scene != root_.id()) {
    return std::unexpected(error(ManagedPhase::MOUNT, parent, TreeError::INVALID_ARGUMENT));
  }

  return *source;
}

template <class T, class W, class Prepare>
std::expected<Node<W>, ManagedError> ManagedScene::attachView(Node<W> node,
                                                              std::shared_ptr<detail::ManagedModelSourceOf<T>> source,
                                                              Prepare prepare, ManagedViewHandler<T> handler) {
  auto fail = [&](TreeError cause) -> std::expected<Node<W>, ManagedError> {
    auto failure = error(ManagedPhase::BIND, node, cause);
    auto removed = remove(node);

    if (!removed) {
      failure.cleanup = std::get<TreeError>(removed.error().cause);
    }

    return std::unexpected(failure);
  };

  try {
    auto entry = find(node);

    // Factory-created nodes cannot carry another binding. Construct the whole callback
    // before publishing it, so allocation failure retires the new child transactionally.
    entry->viewPrepare = [source, prepare = std::move(prepare), handler = std::move(handler)](
                           const LayoutSnapshot &layout) -> std::expected<void, TreeError> {
      if (!source->frame) {
        return std::unexpected(TreeError::INVALID_ARGUMENT);
      }

      auto work = prepare(*source->frame, layout);

      if (!work) {
        return std::unexpected(work.error());
      }

      if (handler) {
        return handler(*source->frame, *work);
      }

      // Without a provider callback there is nobody to retain and dispatch a receipt.
      if (*work) {
        (*work)->cancel();

        return std::unexpected(TreeError::INVALID_ARGUMENT);
      }

      return {};
    };

    if constexpr (std::same_as<W, VirtualTable<T>> || std::same_as<W, VirtualList<T>>) {
      auto binding = [&] {
        if constexpr (std::same_as<W, VirtualTable<T>>) {
          return bindVirtualTable(app_.router(), app_.tree(), node);
        } else {
          return bindVirtualList(app_.router(), app_.tree(), node);
        }
      }();

      if (!binding) {
        return fail(binding.error());
      }

      entry->binding.template emplace<ActionBinding>(std::move(*binding));
    }

    const char *type = std::same_as<W, Chart<T>> ? "Chart" : std::same_as<W, VirtualTable<T>> ? "Table" : "List";

    entry->target = StyleTarget{.type = type};
    entry->stylePending = true;
    stylesDirty_ = true;

    return node;
  } catch (const std::bad_alloc &) {
    return fail(TreeError::NO_MEMORY);
  } catch (...) {
    return fail(TreeError::CALLBACK_FAILED);
  }
}

template <class T>
std::expected<Node<VirtualTable<T>>, ManagedError>
ManagedScene::virtualTable(Node<Widget> parent, ManagedModel<T> model,
                           std::type_identity_t<std::span<const TableColumn<T>>> columns, VirtualTableOptions options,
                           SizePolicy policy, std::type_identity_t<ManagedViewHandler<T>> prepared) {
  auto source = modelSource(model, parent);

  if (!source) {
    return std::unexpected(source.error());
  }

  auto range = (*source)->model->openRange();

  if (!range) {
    return std::unexpected(error(ManagedPhase::MOUNT, parent, range.error()));
  }

  auto node = emplace<VirtualTable<T>>(parent, std::move(*range), columns, options, policy);

  if (!node) {
    return node;
  }

  return attachView(
    *node, *source,
    [this, node = *node](const ModelFrame<T> &frame, const LayoutSnapshot &layout) {
      return prepareVirtualTable(app_.tree(), node, frame, layout);
    },
    std::move(prepared));
}

template <class T>
std::expected<Node<VirtualList<T>>, ManagedError>
ManagedScene::virtualList(Node<Widget> parent, ManagedModel<T> model,
                          std::type_identity_t<ManagedListFormatter<T>> delegate, VirtualListOptions options,
                          SizePolicy policy, std::type_identity_t<ManagedViewHandler<T>> prepared) {
  auto source = modelSource(model, parent);

  if (!source) {
    return std::unexpected(source.error());
  }

  auto range = (*source)->model->openRange();

  if (!range) {
    return std::unexpected(error(ManagedPhase::MOUNT, parent, range.error()));
  }

  auto node = emplace<VirtualList<T>>(parent, std::move(*range), std::move(delegate), options, policy);

  if (!node) {
    return node;
  }

  return attachView(
    *node, *source,
    [this, node = *node](const ModelFrame<T> &frame, const LayoutSnapshot &layout) {
      return prepareVirtualList(app_.tree(), node, frame, layout);
    },
    std::move(prepared));
}

template <class T>
std::expected<Node<Chart<T>>, ManagedError> ManagedScene::chart(Node<Widget> parent, ManagedModel<T> model,
                                                                std::type_identity_t<ManagedChartAccessor<T>> accessor,
                                                                ChartOptions options, ManagedChartOrigin first,
                                                                std::type_identity_t<ManagedViewHandler<T>> prepared) {
  auto source = modelSource(model, parent);

  if (!source) {
    return std::unexpected(source.error());
  }

  auto range = (*source)->model->openRange();

  if (!range) {
    return std::unexpected(error(ManagedPhase::MOUNT, parent, range.error()));
  }

  auto node = emplace<Chart<T>>(parent, std::move(*range), std::move(accessor), options);

  if (!node) {
    return node;
  }

  return attachView(
    *node, *source,
    [this, node = *node, first = std::move(first)](const ModelFrame<T> &frame, const LayoutSnapshot &layout)
      -> std::expected<std::optional<ModelRequest<T>>, TreeError> {
      auto origin = first ? first() : std::expected<std::uint64_t, TreeError>(0);

      if (!origin) {
        return std::unexpected(origin.error());
      }

      return prepareChart(app_.tree(), node, frame, layout, *origin);
    },
    std::move(prepared));
}
} // namespace portals
