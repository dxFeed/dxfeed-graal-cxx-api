// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0
#pragma once

#include <portals/style_types.hpp>
#include <portals/widget.hpp>

#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace portals {
/** @brief Presentation states; ANY matches every state without changing behavior. */
enum class StyleState {
  ANY,      ///< Match all presentation variants.
  NORMAL,   ///< Ordinary content.
  FOCUSED,  ///< Synchronized control focus.
  DISABLED, ///< Synchronized control ineligibility.
  SELECTED, ///< Selected row or editor range.
  HEADER,   ///< Fixed table heading.
  CARET     ///< Editor software caret.
};

/** @brief Owned subject metadata, supplied explicitly for built-in or custom widget types. */
struct StyleTarget {
  /** @brief Destroy owned values. Deliberately suppress implicit noexcept moves: checked MSVC
   * container moves can allocate; copy-based transfers allow allocation failure to propagate.
   */
  ~StyleTarget() = default;

  std::string type{""}; ///< Case-sensitive type name, for example Button or Table.
  std::string id{""};   ///< Optional case-sensitive application identity, independent of NodeId.
  std::vector<std::string> classes =
    std::vector<std::string>(0); ///< Case-sensitive class names; all selector classes must match.
};

/** @brief Owned metadata for one checked target in an atomic binding batch. */
struct StyleRegistration {
  Node<Widget> node;  ///< Observing handle; the tree retains widget ownership.
  StyleTarget target; ///< Matching metadata copied into the engine on success.
};

/** @brief Conjunction of optional type, id, classes and one state; no hierarchy combinators. */
struct StyleSelector {
  /** @brief Destroy owned values. Deliberately suppress implicit noexcept moves: checked MSVC
   * container moves can allocate; copy-based transfers allow allocation failure to propagate.
   */
  ~StyleSelector() = default;

  std::string type{""};                                           ///< Empty or * means any type.
  std::string id{""};                                             ///< Empty means any identity.
  std::vector<std::string> classes = std::vector<std::string>(0); ///< Every named class is required.
  StyleState state{StyleState::ANY};                              ///< Optional presentation-state predicate.
};

/** @brief Typed declarations; absent values preserve less-specific rules or widget defaults. */
struct StyleDeclarations {
  std::optional<RgbColor> foreground;   ///< RGB text color.
  std::optional<RgbColor> background;   ///< RGB fill color.
  std::optional<bool> bold;             ///< Explicit true/false bold request.
  std::optional<bool> italic;           ///< Explicit true/false italic request.
  std::optional<bool> underline;        ///< Explicit true/false underline request.
  std::optional<std::int32_t> minWidth; ///< Nonnegative cell minimum; allowed only on state-independent selectors.
};

/** @brief One owned selector/declaration pair in source order. */
struct StyleRule {
  /** @brief Destroy owned values. Deliberately suppress implicit noexcept moves: checked MSVC
   * container moves can allocate; copy-based transfers allow allocation failure to propagate.
   */
  ~StyleRule() = default;

  StyleSelector selector;         ///< Matching and specificity criteria.
  StyleDeclarations declarations; ///< Values overriding less-specific rules.
};

/** @brief Owned immutable-at-use rules; constructing typed values may allocate and throw. */
struct StyleSheet {
  /** @brief Destroy owned values. Deliberately suppress implicit noexcept moves: checked MSVC
   * container moves can allocate; copy-based transfers allow allocation failure to propagate.
   */
  ~StyleSheet() = default;

  std::vector<StyleRule> rules =
    std::vector<StyleRule>(0); ///< Source-order rules, at most MAX_STYLE_RULES when installed.
};

/** @brief Finite typed/PSS rule budget. */
inline constexpr std::size_t MAX_STYLE_RULES = 1024;
/** @brief Finite parser input byte budget. */
inline constexpr std::size_t MAX_PSS_BYTES = 1024 * 1024;

/** @brief Allocation-free error; message borrows a static string literal. */
struct StyleError {
  TreeError cause{TreeError::INVALID_ARGUMENT}; ///< Tree/validation/allocation failure category.
  std::size_t line{1};                          ///< One-based PSS line, or one for typed/tree errors.
  std::size_t column{1};                        ///< One-based byte column, or one for typed/tree errors.
  std::string_view message;                     ///< Static diagnostic; never borrows source input.
};

/** @brief Parse strict PSS v0 without I/O or mutation, on any thread.
 * @param source Borrowed UTF-8 source; identifiers and grammar are ASCII, copied on success.
 * @return Complete owned sheet or precise error; no partial sheet escapes. Allocation failure is NO_MEMORY.
 */
std::expected<StyleSheet, StyleError> parsePss(std::string_view source);

/** @brief Resolve one subject through the same engine used by typed and PSS installation.
 * @param sheet Valid rules (nonnegative minWidth, at most MAX_STYLE_RULES); immutable during this call.
 * @param target Borrowed matching metadata; no data are retained.
 * @param defaults Owning built-in/custom widget defaults beneath all matching declarations.
 * @return Complete owning style. No allocation, I/O or thread affinity; caller owns input stability.
 */
ComputedStyle resolveStyle(const StyleSheet &sheet, const StyleTarget &target, ComputedStyle defaults = {}) noexcept;

/** @brief Typed palette tokens for the built-in theme factories. */
struct ThemeTokens {
  RgbColor foreground; ///< Normal foreground.
  RgbColor background; ///< Normal background.
  RgbColor accent;     ///< Focus background and contrasting caret foreground.
  RgbColor muted;      ///< Disabled foreground.
  RgbColor selection;  ///< Selected-row/range background.
  RgbColor heading;    ///< Heading foreground.
};

/** @param tokens Owning palette colors.
 * @return Typed theme rules; allocation failure throws before tree mutation.
 */
StyleSheet themeFromTokens(ThemeTokens tokens);
/** @return Typed Modern theme, equivalent to themes/modern.pss; may allocate. */
StyleSheet modernTheme();
/** @return Typed Turbo-like theme, equivalent to themes/turbo.pss; may allocate. */
StyleSheet turboTheme();

/** @return Typed Nord palette adapted from Textual through ThemeTokens; may allocate. */
StyleSheet nordTheme();

/** @return Typed Gruvbox palette adapted from Textual through ThemeTokens; may allocate. */
StyleSheet gruvboxTheme();

/** @return Typed Tokyo Night palette adapted from Textual through ThemeTokens; may allocate. */
StyleSheet tokyoNightTheme();

/** @return Typed light Catppuccin Latte palette adapted from Textual through ThemeTokens; may allocate. */
StyleSheet catppuccinLatteTheme();

/** @brief UI-owned style transaction coordinator; tree must outlive this nonmovable object.
 * Bindings own metadata and only observe Node handles. Stale bindings are pruned on the next
 * successful operation. Destruction leaves already computed widget values intact. One engine
 * may own a widget's style at a time; overlapping engines are unsupported. No notifications, I/O,
 * workers or hot reload. All mutations reject active frames and wrong-thread calls before work.
 */
class StyleEngine {
public:
  /** @param tree Borrowed UI owner, required to outlive the engine.
   * @param maxBindings Positive registry budget; default matches the tree's default capacity.
   */
  explicit StyleEngine(WidgetTree &tree, std::size_t maxBindings = 4096);
  StyleEngine(const StyleEngine &) = delete;
  StyleEngine &operator=(const StyleEngine &) = delete;

  /** @brief Register/update metadata and atomically recompute every bound widget.
   * @param node Checked target; no ownership transfer.
   * @param target Owned matching metadata, copied during staging.
   * @return Coalesced invalidation, or error retaining every previous value and binding.
   */
  std::expected<Invalidation, StyleError> bind(Node<Widget> node, StyleTarget target);

  /// @brief Update metadata in one transaction, then recompute every bound widget once.
  /// Duplicate input nodes return INVALID_ARGUMENT. Stale bindings are pruned before admission.
  /// Empty input performs the same refresh as refresh().
  /// With index allocation available, identity lookup takes O((B + R) log(R + 2))
  /// work and O(R) temporary space
  /// for B existing bindings and R input registrations. Metadata copies and style
  /// resolution are additional costs; existing callback order is preserved.
  /// If index allocation fails, an allocation-free O(R^2) validation preserves
  /// argument-error priority before returning NO_MEMORY for an otherwise valid batch.
  /// @param registrations Borrowed live targets and metadata, copied during staging.
  /// @return Coalesced invalidation, or an error preserving all values, bindings and revisions.
  std::expected<Invalidation, StyleError> bindBatch(std::span<const StyleRegistration> registrations);
  /** @brief Install a complete typed stylesheet atomically between frames.
   * @param sheet Owned replacement rules; validation and staging precede commit.
   * @return Actual paint/layout invalidation, or error retaining sheet/revision/widget values.
   */
  std::expected<Invalidation, StyleError> replace(StyleSheet sheet);
  /** @brief Parse and install without ever exposing a partial stylesheet.
   * @param source Borrowed PSS; copied by parsing before the atomic commit.
   * @return Actual invalidation or source/tree error retaining the prior revision.
   */
  std::expected<Invalidation, StyleError> replacePss(std::string_view source);
  /** @brief Recompute against current manual widget options, between frames.
   * @return Actual invalidation or error with no partial update.
   */
  std::expected<Invalidation, StyleError> refresh();
  /** @brief Inspect on UI; no allocation or tree access.
   * @return Last committed revision, initially zero; equal styles can still advance it.
   */
  std::uint64_t revision() const noexcept;

private:
  struct Binding {
    Node<Widget> node;
    StyleTarget target;
  };

  std::expected<Invalidation, StyleError> install(StyleSheet sheet, std::vector<Binding> &&bindings);
  std::expected<void, StyleError> checkBoundary();
  WidgetTree &tree_;
  std::size_t maxBindings_;
  StyleSheet sheet_;
  std::vector<Binding> bindings_ = std::vector<Binding>(0);
  std::uint64_t revision_{};
};
} // namespace portals
