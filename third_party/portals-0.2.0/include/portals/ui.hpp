// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0
#pragma once

#include <portals/grid.hpp>
#include <portals/managed.hpp>

#include <algorithm>
#include <tuple>

/// @brief Owning construction descriptions mounted through the existing ManagedScene lifecycle.
namespace portals::ui {
/** @brief Single-use owning construction recipe; no mounted widget ownership.
 * @tparam T Public Widget subtype constructed by the recipe, including final types.
 */
template <class T> class Description;

namespace detail {
/** @brief Recognize an owning recipe after cv/ref normalization.
 * @tparam T Type tested for an exact Description
 * specialization.
 */
template <class T> struct IsDescription : std::false_type {};

/** @brief Recognize the owning recipe specialization.
 * @tparam T Widget type described by this recipe.
 */
template <class T> struct IsDescription<Description<T>> : std::true_type {};

/** @brief Admit only consuming recipe arguments, rejecting implicit moves from lvalues.
 * @tparam T
 * Forwarding-reference argument type before normalization.
 */
template <class T>
concept DescriptionInput = IsDescription<std::remove_cvref_t<T>>::value && !std::is_lvalue_reference_v<T>;

/** @brief Preserve a construction failure without formatting its diagnostic.
 * @param node Observing target identity,
 * possibly empty before allocation.
 * @param cause Original tree error from the failed construction step.
 * @return
 * Mount-phase error with independent diagnostic fields initially empty.
 */
inline ManagedError error(Node<Widget> node, TreeError cause) {
  return {ManagedPhase::MOUNT, node.id(), cause, {}, {}};
}

/** @brief Stack-owned transaction bookkeeping, discarded after one description mount. */
struct MountContext {
  Node<Widget> root;               ///< First created node; identifies the subtree to retire on failure.
  std::vector<Node<Widget>> nodes; ///< Created observers checked for liveness before publication.
  std::vector<std::move_only_function<void()>> publish; ///< Nonthrowing assignments publishing reserved refs.
  std::vector<std::move_only_function<void()>> release; ///< Nonthrowing releases of temporary ref reservations.

  /** @brief Release ref reservations after either publication or rollback. */
  ~MountContext() {
    for (auto &reservation : release) {
      reservation();
    }
  }
};

/** @brief Internal bridge to existing managed validation and private consuming recipes. */
struct Access {
  /** @brief Mount a root or child inside the same transaction.
   * @tparam T Public Widget subtype constructed by the
   * recipe.
   * @param description Owning recipe consumed by this call, including on failure.
   * @param scene
   * Existing UI-affine owner of all mounted nodes.
   * @param parent Live managed parent receiving the new child.
   *
   * @param context Borrowed transaction retaining observations and deferred ref publication.
   * @return Typed
   * observing node or the unchanged creation/setup/child error.
   */
  template <class T>
  static std::expected<Node<T>, ManagedError> mount(Description<T> &&description, ManagedScene &scene,
                                                    Node<Widget> parent, MountContext &context);

  /** @brief Wrap a checked managed factory without exposing recipe construction publicly.
   * @tparam T Widget subtype returned by the factory.
   * @tparam Factory Owning callable returning expected<Node<T>, ManagedError> for scene and parent.
   * @param factory Capture retained until the consuming mount; construction may throw.
   * @return Single-use description with the existing transaction semantics.
   */
  template <class T, class Factory> static Description<T> describe(Factory factory);

  /** @brief Verify a created node remains accessible before publishing references.
   * @param scene Existing managed
   * owner, borrowed for this check.
   * @param node Observing identity whose tree scope is validated.
   * @return
   * Success or the original managed boundary/liveness error.
   */
  static std::expected<void, ManagedError> check(ManagedScene &scene, Node<Widget> node) {
    return scene.boundary(ManagedPhase::MOUNT, node);
  }
};
} // namespace detail

/** @brief Shared observing slot published only after a complete successful description mount.
 * @tparam T Observed public Widget subtype; no widget or scene ownership is retained.
 * Read/write the slot only on the UI thread. Copies share the slot. A published identity is
 * never automatically reset; checked tree access detects later retirement. Use a new slot
 * for another description. Construction can throw std::bad_alloc.
 */
template <class T> class NodeRef {
public:
  /** @brief Allocate an empty observing slot, with no widget ownership. */
  NodeRef() : slot_(std::make_shared<Slot>()) {
  }

  /** @return Empty before successful mount, otherwise the observing identity, possibly stale. */
  Node<T> node() const noexcept {
    return slot_ ? slot_->node : Node<T>{};
  }

private:
  friend class Description<T>;

  /** @brief Shared identity storage; never owns a widget or its scene. */
  struct Slot {
    Node<T> node;   ///< Published identity, empty until the complete mount succeeds.
    bool pending{}; ///< Reserved by an in-progress mount, including across reentrant calls.
  };

  std::shared_ptr<Slot> slot_;
};

/** @brief Owning setup action invoked once after construction, before children.
 * @tparam T Concrete Widget type observed by the action.
 * Arguments are borrowed for the invocation. External effects and separately created scene
 * state are not rolled back; the attempted description subtree is removed on failure.
 */
template <class T> using Setup = std::move_only_function<std::expected<void, ManagedError>(ManagedScene &, Node<T>)>;

/** @brief Owning optional editor customization, preserving existing LineEdit behavior. */
struct LineEditConfig {
  std::function<std::expected<void, TreeError>(LineEditResult)> result{}; ///< Optional callback after standard editing.
  LineEditOptions options{}; ///< Editor limits and appearances; scene width policy overrides options.widthPolicy.
  SizePolicy policy{};       ///< Owning layout constraints validated by the existing layout engine.
  std::shared_ptr<Clipboard> clipboard{};           ///< Optional UI-owned clipboard adapter retained by the binding.
  std::function<bool(std::string_view)> validate{}; ///< Optional commit validator; false rejects Enter.
};

/** @brief Move-only recipe whose strings, arguments, children and callbacks survive their sources.
 * @tparam T Public Widget subtype. Factories enforce constructor compatibility when instantiated.
 * Factory/modifier allocations and capture construction may throw before mount begins. Mount
 * translates internal exceptions, consumes the entire recipe even on failure, and transfers
 * registered callbacks to the existing ManagedScene. Destruction never changes mounted UI.
 */
template <class T> class Description {
public:
  /** @brief Construct an empty recipe; mounting it returns INVALID_ARGUMENT. */
  Description() = default;

  /** @brief Transfer the unnamed recipe; its source becomes empty. */
  Description(Description &&) noexcept = default;

  /** @brief Replace this recipe with the unnamed source, which becomes empty.
   * @return This recipe. Destroying a replaced recipe has no mounted UI effect.
   */
  Description &operator=(Description &&) noexcept = default;

  Description(const Description &) = delete;

  Description &operator=(const Description &) = delete;

  /** @param reference Empty observing slot; duplicate slots in one mount are rejected.
   * @return This consuming recipe for chaining. The slot remains empty on mount failure.
   */
  Description &&reference(NodeRef<T> reference) && {
    return std::move(*this).storeReference(std::move(reference));
  }

  /** @param target Owning style identity, staged through ManagedScene::style.
   * @return This consuming recipe for chaining.
   */
  Description &&style(StyleTarget target) && {
    return std::move(*this).setup([target = std::move(target)](ManagedScene &scene, Node<T> node) mutable {
      return scene.style(node, std::move(target));
    });
  }

  /** @param callback Existing managed custom event adapter; standard bindings reject duplicates.
   * @return This consuming recipe for chaining; the scene owns the callback after mounting.
   */
  Description &&onEvent(ManagedEventHandler<T> callback) && {
    return std::move(*this).setup([callback = std::move(callback)](ManagedScene &scene, Node<T> node) mutable {
      return scene.onEvent(node, std::move(callback));
    });
  }

  /** @param callback Existing managed dirty-frame adapter; no new polling or runtime is introduced.
   * @return This consuming recipe for chaining.
   */
  Description &&onPrepare(ManagedPrepareHandler<T> callback) && {
    return std::move(*this).setup([callback = std::move(callback)](ManagedScene &scene, Node<T> node) mutable {
      return scene.onPrepare(node, std::move(callback));
    });
  }

  /** @param callback Owning customization action, run in modifier order before children.
   * Capture the existing Application explicitly when a low-level tree operation is needed.
   * @return This consuming recipe for chaining. Empty callbacks fail at mount.
   */
  Description &&setup(Setup<T> callback) && {
    if (state_) {
      state_->setup.push_back(std::move(callback));
    }

    return std::move(*this);
  }

private:
  friend struct detail::Access;
  template <class U, class... Args> friend Description<U> custom(Args &&...args);
  template <class U> friend Description<U> control(std::string_view, ControlStyle, SizePolicy);
  template <class U, detail::DescriptionInput... Children> friend Description<U> linear(LinearOptions, Children &&...);
  template <detail::DescriptionInput... Children> friend Description<Grid> grid(Children &&...);
  friend Description<LineEdit> lineEdit(ManagedState<std::string>, LineEditConfig);

  /** @brief Owning constructor adapter using the existing checked managed mount. */
  using Factory = std::move_only_function<std::expected<Node<T>, ManagedError>(ManagedScene &, Node<Widget>)>;
  /** @brief Owning child recipe adapter sharing the parent's transaction bookkeeping. */
  using Child =
    std::move_only_function<std::expected<void, ManagedError>(ManagedScene &, Node<Widget>, detail::MountContext &)>;

  /** @brief Unmounted recipe storage, consumed and destroyed during mount. */
  struct State {
    Factory create;                     ///< Constructs exactly one managed node.
    std::vector<Setup<T>> setup;        ///< Customization actions in modifier order.
    std::vector<Child> children;        ///< Consuming child adapters in insertion order.
    std::vector<NodeRef<T>> references; ///< Observing slots reserved for this node.
  };

  /** @brief Allocate recipe storage and take ownership of its constructor adapter.
   * @param create Owning checked
   * constructor callback, invoked only at mount.
   */
  explicit Description(Factory create) : state_(std::make_unique<State>()) {
    state_->create = std::move(create);
  }

  /** @brief Retain a shared observing slot without publishing it yet.
   * @param reference Slot to reserve when this
   * recipe is mounted.
   * @return This consuming recipe for modifier chaining.
   */
  Description &&storeReference(NodeRef<T> reference) && {
    if (state_) {
      state_->references.push_back(std::move(reference));
    }

    return std::move(*this);
  }

  /** @brief Consume this recipe into one existing managed transaction.
   * @param scene Borrowed UI owner receiving
   * the new subtree.
   * @param parent Live managed parent receiving this recipe's root.
   * @param context Borrowed
   * observations and ref reservations for the complete mount.
   * @return Created observer or unchanged error; the
   * outer mount owns subtree rollback.
   */
  std::expected<Node<T>, ManagedError> mountInto(ManagedScene &scene, Node<Widget> parent,
                                                 detail::MountContext &context) && {
    auto state = std::move(state_);

    if (!state || !state->create) {
      return std::unexpected(detail::error(parent, TreeError::INVALID_ARGUMENT));
    }

    auto node = state->create(scene, parent);

    if (!node) {
      return node;
    }

    if (context.root == Node<Widget>{}) {
      context.root = *node;
    }

    context.nodes.push_back(*node);

    for (auto &reference : state->references) {
      auto slot = reference.slot_;

      if (!slot || slot->node != Node<T>{} || slot->pending) {
        return std::unexpected(detail::error(*node, TreeError::INVALID_ARGUMENT));
      }

      // Reserve across reentrant mount calls too, but never publish a partial identity.
      // Allocate both callbacks before setting pending so exceptions cannot strand the slot.
      context.release.push_back([slot] {
        slot->pending = false;
      });
      context.publish.push_back([slot, value = *node] {
        slot->node = value;
      });
      slot->pending = true;
    }

    for (auto &setup : state->setup) {
      if (!setup) {
        return std::unexpected(detail::error(*node, TreeError::INVALID_ARGUMENT));
      }

      auto configured = setup(scene, *node);

      if (!configured) {
        return std::unexpected(configured.error());
      }
    }

    for (auto &child : state->children) {
      auto mounted = child(scene, *node, context);

      if (!mounted) {
        return std::unexpected(mounted.error());
      }
    }

    return node;
  }

  std::unique_ptr<State> state_;
};

namespace detail {
template <class T, class Factory> Description<T> Access::describe(Factory factory) {
  return Description<T>(std::move(factory));
}

template <class T>
std::expected<Node<T>, ManagedError> Access::mount(Description<T> &&description, ManagedScene &scene,
                                                   Node<Widget> parent, MountContext &context) {
  return std::move(description).mountInto(scene, parent, context);
}

/** @brief Materialize string-like inputs and otherwise retain a decayed argument value.
 * @tparam T Forwarded
 * constructor argument type; strings must convert to std::string_view.
 * @param value Source copied or moved into
 * recipe storage; nonstring borrows remain explicit.
 * @return Owning std::string for string-like inputs, otherwise a
 * decayed argument value.
 */
template <class T> auto own(T &&value) {
  if constexpr (std::convertible_to<T, std::string_view>) {
    return std::string(std::string_view(value));
  } else {
    return std::decay_t<T>(std::forward<T>(value));
  }
}
} // namespace detail

/** @brief Capture constructor arguments without requiring inheritance from a DSL widget.
 * @tparam T Public Widget subtype, including final types, constructible from the owned arguments.
 * @tparam Args Decayed owning argument types; string-like inputs are materialized as std::string.
 * @param args Values copied/moved now and moved into T at mount. Other pointers/reference_wrapper
 * remain explicit borrows: their referents must outlive the widget. No deep copy is inferred.
 * @return A single-use recipe. T must own any string_view it retains beyond its constructor.
 */
template <class T, class... Args> Description<T> custom(Args &&...args) {
  using OwnedArguments = std::tuple<decltype(detail::own(std::forward<Args>(args)))...>;

  // Explicit element types keep a tuple or pair argument intact instead of applying CTAD's copy deduction guide.
  auto owned = OwnedArguments(detail::own(std::forward<Args>(args))...);

  return Description<T>([owned = std::move(owned)](ManagedScene &scene, Node<Widget> parent) mutable {
    return std::apply(
      [&](auto &...values) {
        return scene.emplace<T>(parent, std::move(values)...);
      },
      owned);
  });
}

/** @brief Construct a standard text-control recipe with the scene's width policy.
 * @tparam T Label, Button or CheckBox-compatible text constructor.
 * @param text UTF-8 bytes copied now; invalid text returns INVALID_ARGUMENT at mount.
 * @param style Owning manual control appearances.
 * @param policy Owning layout constraints.
 * @return Single-use owning recipe; allocation while copying may throw.
 */
template <class T> Description<T> control(std::string_view text, ControlStyle style, SizePolicy policy) {
  return Description<T>([text = std::string(text), style, policy](ManagedScene &scene, Node<Widget> parent) {
    auto value = GraphemeText::from(text, scene.widthPolicy());

    if (!value) {
      const auto kind = value.error().kind;
      const bool limited = kind == TextErrorKind::INPUT_LIMIT || kind == TextErrorKind::CLUSTER_LIMIT ||
                           kind == TextErrorKind::GRAPHEME_LIMIT || kind == TextErrorKind::CELL_LIMIT;
      auto failure = detail::error(parent, limited ? TreeError::NODE_LIMIT : TreeError::INVALID_ARGUMENT);

      failure.text = value.error();

      return std::expected<Node<T>, ManagedError>(std::unexpected(failure));
    }

    return scene.emplace<T>(parent, std::move(*value), style, policy);
  });
}

/** @param text UTF-8 copied immediately, validated at mount under the scene's width policy.
 * @param style Owning manual appearances.
 * @param policy Owning layout constraints.
 * @return Single-use label recipe.
 */
inline Description<Label> label(std::string_view text, ControlStyle style = {}, SizePolicy policy = {}) {
  return control<Label>(text, style, policy);
}

/** @param text UTF-8 caption copied immediately.
 * @param action Standard managed activation callback; must be nonempty.
 * @param style Owning manual appearances.
 * @param policy Owning layout constraints.
 * @return Single-use button recipe retaining its callback until mount.
 */
inline Description<Button> button(std::string_view text, std::function<std::expected<void, TreeError>()> action,
                                  ControlStyle style = {}, SizePolicy policy = {}) {
  return control<Button>(text, style, policy)
    .setup([action = std::move(action)](ManagedScene &scene, Node<Button> node) mutable {
      return scene.onClick(node, std::move(action));
    });
}

/** @param text UTF-8 caption copied immediately.
 * @param model Observing managed state from the scene receiving this recipe.
 * @param changed Optional callback after the standard commit; cannot undo the commit.
 * @param style Owning manual appearances.
 * @param policy Owning layout constraints.
 * @return Single-use bound checkbox recipe.
 */
inline Description<CheckBox> checkbox(std::string_view text, ManagedState<bool> model,
                                      std::function<std::expected<void, TreeError>(CheckBoxResult)> changed = {},
                                      ControlStyle style = {}, SizePolicy policy = {}) {
  return control<CheckBox>(text, style, policy)
    .setup([model, changed = std::move(changed)](ManagedScene &scene, Node<CheckBox> node) mutable {
      return scene.bind(node, model, std::move(changed));
    });
}

/** @param model Observing scene-owned text; existing draft/commit/conflict semantics are retained.
 * @param config Owning optional callbacks, limits, appearances, layout and clipboard service.
 * @return Single-use bound editor recipe; width policy always follows the scene.
 */
inline Description<LineEdit> lineEdit(ManagedState<std::string> model, LineEditConfig config = {}) {
  auto description = Description<LineEdit>(
    [options = config.options, policy = config.policy](ManagedScene &scene, Node<Widget> parent) mutable {
      options.widthPolicy = scene.widthPolicy();

      return scene.emplace<LineEdit>(parent, options, policy);
    });

  return std::move(description)
    .setup([model, result = std::move(config.result), clipboard = std::move(config.clipboard),
            validate = std::move(config.validate)](ManagedScene &scene, Node<LineEdit> node) mutable {
      return scene.bind(node, model, std::move(result), std::move(clipboard), std::move(validate));
    });
}

/// @brief Describe a managed virtual table without opening a range until mount.
/// @tparam T Immutable model row accepted by the column formatters; deduced from model.
/// @param model Weak UI-affine handle belonging to the scene receiving this recipe.
/// @param columns Descriptions copied now, including owning headers and formatter objects.
/// @param options Existing table settings, validated by managed preparation.
/// @param policy Existing layout constraints for the table.
/// @param prepared Owning provider callback after preparation, including retries.
/// Empty is valid for ready snapshots; pending work without it returns INVALID_ARGUMENT.
/// @return Single-use recipe with standard input, styles and automatic preparation.
/// Construction may throw while copying columns or callbacks. Mount preserves typed errors
/// and subtree rollback. The scene owns the model; reference captures remain borrowed.
template <class T>
Description<VirtualTable<T>> virtualTable(ManagedModel<T> model,
                                          std::type_identity_t<std::span<const TableColumn<T>>> columns,
                                          VirtualTableOptions options = {}, SizePolicy policy = {},
                                          std::type_identity_t<ManagedViewHandler<T>> prepared = {}) {
  // A recipe may outlive the caller's array or vector. Copy before retaining a mount callback.
  auto ownedColumns = std::vector<TableColumn<T>>(columns.begin(), columns.end());

  return detail::Access::describe<VirtualTable<T>>(
    [model, columns = std::move(ownedColumns), options, policy,
     prepared = std::move(prepared)](ManagedScene &scene, Node<Widget> parent) mutable {
      return scene.virtualTable(parent, model, columns, options, policy, std::move(prepared));
    });
}

/// @brief Describe a managed virtual list without opening a range until mount.
/// @tparam T Immutable model row accepted by the formatter; deduced from model.
/// @param model Weak UI-affine handle belonging to the receiving scene.
/// @param delegate Owning formatter invoked for visible rows during preparation.
/// @param options Existing list settings, validated by managed preparation.
/// @param policy Existing layout constraints for the list.
/// @param prepared Owning provider callback after preparation. Empty rejects pending work
/// with INVALID_ARGUMENT. Captured references must remain valid until retirement.
/// @return Single-use recipe retaining callbacks and standard managed input/style behavior.
/// Capture construction may throw; mount preserves typed errors and subtree rollback.
template <class T>
Description<VirtualList<T>> virtualList(ManagedModel<T> model, std::type_identity_t<ManagedListFormatter<T>> delegate,
                                        VirtualListOptions options = {}, SizePolicy policy = {},
                                        std::type_identity_t<ManagedViewHandler<T>> prepared = {}) {
  return detail::Access::describe<VirtualList<T>>(
    [model, delegate = std::move(delegate), options, policy,
     prepared = std::move(prepared)](ManagedScene &scene, Node<Widget> parent) mutable {
      return scene.virtualList(parent, model, std::move(delegate), options, policy, std::move(prepared));
    });
}

/// @brief Describe a managed chart using the existing frame preparation order.
/// @tparam T Immutable model row projected into optional OHLC values; deduced from model.
/// @param model Weak UI-affine handle belonging to the receiving scene.
/// @param accessor Owning projection; nullopt represents a gap, as in the managed chart API.
/// @param options Existing chart mode, width and glyph configuration.
/// @param first Owning origin callback, invoked in composition registration order during
/// preparation. Empty means row zero. NodeRef observations are already published then.
/// @param prepared Owning provider callback after preparation, including retries. Empty
/// rejects pending work with INVALID_ARGUMENT. Explicit reference captures remain borrowed.
/// @return Single-use recipe with automatic style/preparation and no input binding.
/// Capture construction may throw; mount preserves typed errors and subtree rollback.
template <class T>
Description<Chart<T>> chart(ManagedModel<T> model, std::type_identity_t<ManagedChartAccessor<T>> accessor,
                            ChartOptions options = {}, ManagedChartOrigin first = {},
                            std::type_identity_t<ManagedViewHandler<T>> prepared = {}) {
  return detail::Access::describe<Chart<T>>(
    [model, accessor = std::move(accessor), options, first = std::move(first),
     prepared = std::move(prepared)](ManagedScene &scene, Node<Widget> parent) mutable {
      return scene.chart(parent, model, std::move(accessor), options, std::move(first), std::move(prepared));
    });
}

/// @brief Named owning settings for a table recipe; defaults match the positional factory.
/// @tparam T Immutable model row type, matching the receiving ManagedModel.
/// Copying captures may throw; explicit reference captures remain borrowed until retirement.
template <class T> struct VirtualTableConfig {
  VirtualTableOptions options{};    ///< Existing dimensions, width policy, budgets and styles.
  SizePolicy policy{};              ///< Layout constraints in terminal cells.
  ManagedViewHandler<T> prepared{}; ///< Provider callback; empty rejects pending work with INVALID_ARGUMENT.
};

/// @brief Named owning settings for a list recipe; defaults match the positional factory.
/// @tparam T Immutable model row type, matching the receiving ManagedModel.
/// Copying captures may throw; explicit reference captures remain borrowed until retirement.
template <class T> struct VirtualListConfig {
  VirtualListOptions options{};     ///< Existing row height, overscan, width policy and styles.
  SizePolicy policy{};              ///< Layout constraints in terminal cells.
  ManagedViewHandler<T> prepared{}; ///< Provider callback; empty rejects pending work with INVALID_ARGUMENT.
};

/// @brief Named owning settings for a chart recipe; defaults match the positional factory.
/// @tparam T Immutable model row type projected by the chart accessor.
/// Copying captures may throw; explicit reference captures remain borrowed until retirement.
template <class T> struct ChartConfig {
  ChartOptions options{};           ///< Existing plot mode, width, glyphs and styles.
  ManagedChartOrigin first{};       ///< Origin evaluated during preparation; empty starts at row zero.
  ManagedViewHandler<T> prepared{}; ///< Provider callback; empty rejects pending work with INVALID_ARGUMENT.
};

/// @brief Describe a table using named settings and the same owning mount transaction.
/// @tparam T Immutable row type deduced from model.
/// @tparam Config Exactly VirtualTableConfig<T>, optionally cv/ref qualified.
/// @param model Weak UI-affine handle belonging to the receiving scene.
/// @param columns Definitions copied now, including headers and formatter captures.
/// @param config Settings copied from lvalues or consumed from rvalues; captures may throw.
/// @return Single-use recipe; mount and preparation preserve the positional factory's typed errors.
/// Use an explicitly typed aggregate: bare {} continues to mean legacy table options.
template <class T, class Config>
  requires std::same_as<std::remove_cvref_t<Config>, VirtualTableConfig<T>>
Description<VirtualTable<T>>
virtualTable(ManagedModel<T> model, std::type_identity_t<std::span<const TableColumn<T>>> columns, Config &&config) {
  return virtualTable(model, columns, config.options, config.policy, std::forward<Config>(config).prepared);
}

/// @brief Describe a list using named settings without changing its managed lifecycle.
/// @tparam T Immutable row type deduced from model.
/// @tparam Config Exactly VirtualListConfig<T>, optionally cv/ref qualified.
/// @param model Weak UI-affine handle belonging to the receiving scene.
/// @param delegate Owning visible-row formatter; reference captures remain borrowed.
/// @param config Settings copied from lvalues or consumed from rvalues; captures may throw.
/// @return Single-use recipe preserving positional mount, preparation and error semantics.
/// Use an explicitly typed aggregate: bare {} continues to mean legacy list options.
template <class T, class Config>
  requires std::same_as<std::remove_cvref_t<Config>, VirtualListConfig<T>>
Description<VirtualList<T>> virtualList(ManagedModel<T> model, std::type_identity_t<ManagedListFormatter<T>> delegate,
                                        Config &&config) {
  return virtualList(model, std::move(delegate), config.options, config.policy, std::forward<Config>(config).prepared);
}

/// @brief Describe a chart using named settings without changing preparation order.
/// @tparam T Immutable row type deduced from model.
/// @tparam Config Exactly ChartConfig<T>, optionally cv/ref qualified.
/// @param model Weak UI-affine handle belonging to the receiving scene.
/// @param accessor Owning projection to optional OHLC; nullopt represents a gap.
/// @param config Settings copied from lvalues or consumed from rvalues; captures may throw.
/// @return Single-use recipe preserving positional mount, preparation and error semantics.
/// Use an explicitly typed aggregate: bare {} continues to mean legacy chart options.
template <class T, class Config>
  requires std::same_as<std::remove_cvref_t<Config>, ChartConfig<T>>
Description<Chart<T>> chart(ManagedModel<T> model, std::type_identity_t<ManagedChartAccessor<T>> accessor,
                            Config &&config) {
  return chart(model, std::move(accessor), config.options, std::forward<Config>(config).first,
               std::forward<Config>(config).prepared);
}

/** @brief Compose a retained linear layout with consuming child recipes.
 * @tparam T Existing Row or Column subtype
 * accepting LinearOptions.
 * @tparam Children Description types, passed as rvalues.
 * @param options Owning layout settings, validated by the existing layout engine.
 * @param children Owning recipes, mounted in argument order after the parent setup.
 * @return Single-use parent recipe; no second runtime or retained description tree is created.
 */
template <class T, detail::DescriptionInput... Children>
Description<T> linear(LinearOptions options, Children &&...children) {
  auto description = custom<T>(options);

  (description.state_->children.push_back(
     [child =
        std::forward<Children>(children)](ManagedScene &scene, Node<Widget> parent,
                                          detail::MountContext &context) mutable -> std::expected<void, ManagedError> {
       auto mounted = detail::Access::mount(std::move(child), scene, parent, context);

       return mounted ? std::expected<void, ManagedError>{} : std::unexpected(mounted.error());
     }),
   ...);

  return description;
}

/** @tparam Children Rvalue Description types.
 * @param options Owning horizontal layout settings.
 * @param children Owning child recipes in left-to-right order.
 * @return Single-use row recipe.
 */
template <detail::DescriptionInput... Children> Description<Row> row(LinearOptions options, Children &&...children) {
  return linear<Row>(options, std::forward<Children>(children)...);
}

/** @brief Compose an explicit grid using the existing managed mount transaction.
 * @tparam Children Consuming child Description types, including custom final widgets.
 * @param children Owning recipes mounted in sibling order; no placement is inferred.
 * @return Single-use grid recipe; failed child mount rolls back the whole subtree.
 * Configure tracks and identity-based placements with setGridOptions after successful
 * mount, before running the scene. Published NodeRef observers supply those identities.
 * Construction may allocate; mounting retains the existing typed error and cleanup rules.
 */
template <detail::DescriptionInput... Children> Description<Grid> grid(Children &&...children) {
  auto description = custom<Grid>();

  (description.state_->children.push_back(
     [child =
        std::forward<Children>(children)](ManagedScene &scene, Node<Widget> parent,
                                          detail::MountContext &context) mutable -> std::expected<void, ManagedError> {
       auto mounted = detail::Access::mount(std::move(child), scene, parent, context);

       return mounted ? std::expected<void, ManagedError>{} : std::unexpected(mounted.error());
     }),
   ...);

  return description;
}

/** @tparam Children Rvalue Description types.
 * @param children Owning child recipes in left-to-right order.
 * @return Single-use row recipe with default layout settings.
 */
template <detail::DescriptionInput... Children> Description<Row> row(Children &&...children) {
  return linear<Row>({}, std::forward<Children>(children)...);
}

/** @tparam Children Rvalue Description types.
 * @param options Owning vertical layout settings.
 * @param children Owning child recipes in top-to-bottom order.
 * @return Single-use column recipe.
 */
template <detail::DescriptionInput... Children>
Description<Column> column(LinearOptions options, Children &&...children) {
  return linear<Column>(options, std::forward<Children>(children)...);
}

/** @tparam Children Rvalue Description types.
 * @param children Owning child recipes in top-to-bottom order.
 * @return Single-use column recipe with default layout settings.
 */
template <detail::DescriptionInput... Children> Description<Column> column(Children &&...children) {
  return linear<Column>({}, std::forward<Children>(children)...);
}

/** @brief Consume a recipe into the existing managed subtree, with failure cleanup.
 * @tparam T Concrete root Widget type constructed by the recipe.
 * @param scene Existing UI-affine owner, outliving all mounted widgets.
 * @param description Owning rvalue recipe, consumed even when mounting fails.
 * @return Observing root or original typed failure plus independent cleanup error. Earlier
 * scene nodes and states survive. Reusing an empty/moved/consumed recipe returns INVALID_ARGUMENT.
 * References publish only after all nodes remain live. External callback effects are not undone.
 */
template <class T> std::expected<Node<T>, ManagedError> mount(ManagedScene &scene, Description<T> &&description) {
  detail::MountContext context;
  std::expected<Node<T>, ManagedError> result =
    std::unexpected(detail::error(scene.root(), TreeError::INVALID_ARGUMENT));

  try {
    result = detail::Access::mount(std::move(description), scene, scene.root(), context);

    if (result) {
      for (auto node : context.nodes) {
        auto live = detail::Access::check(scene, node);

        if (!live) {
          result = std::unexpected(live.error());

          break;
        }
      }
    }
  } catch (const std::bad_alloc &) {
    result = std::unexpected(detail::error(context.root, TreeError::NO_MEMORY));
  } catch (...) {
    result = std::unexpected(detail::error(context.root, TreeError::CALLBACK_FAILED));
  }

  if (!result && context.root != Node<Widget>{}) {
    auto removed = scene.remove(context.root);

    if (!removed) {
      if (const auto *cause = std::get_if<TreeError>(&removed.error().cause)) {
        if (*cause != TreeError::STALE_NODE && *cause != TreeError::CLOSED) {
          result.error().cleanup = *cause;
        }
      }
    }
  }

  if (result) {
    for (auto &publish : context.publish) {
      publish();
    }
  }

  return result;
}
} // namespace portals::ui
