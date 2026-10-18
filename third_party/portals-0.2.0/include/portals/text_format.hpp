// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0
#pragma once

#include <portals/text.hpp>

#include <format>
#include <utility>

namespace portals {

/** @brief Format and validate an owning text value with an explicit width policy.
 * @tparam Args Argument types accepted by the compile-time checked standard format string.
 * @param policy Display policy captured by the returned text; use the receiving view's policy.
 * @param pattern Standard format string, checked against Args at compilation.
 * @param args Values borrowed only during this synchronous call.
 * @return Independent validated text or the original TextError, without replacement characters.
 * @throws std::bad_alloc If formatting or text storage cannot be allocated.
 * @throws std::format_error If a user-defined formatter fails.
 */
template <class... Args>
std::expected<GraphemeText, TextError> formatText(WidthPolicy policy, std::format_string<Args...> pattern,
                                                  Args &&...args) {
  return GraphemeText::from(std::format(pattern, std::forward<Args>(args)...), policy);
}

/** @brief Format checked text under the default display policy.
 * @tparam Args Types accepted by the standard compile-time checked format string.
 * @param pattern Standard format string checked against Args.
 * @param args Values borrowed only for this call; no references are retained.
 * @return Owning text or its unchanged validation error.
 * @throws std::bad_alloc If formatting or validation storage allocation fails.
 * @throws std::format_error If a custom formatter fails.
 */
template <class... Args>
std::expected<GraphemeText, TextError> formatText(std::format_string<Args...> pattern, Args &&...args) {
  return formatText(WidthPolicy{}, pattern, std::forward<Args>(args)...);
}

/** @brief Owning single-value formatter for the existing table callback contract.
 * @tparam T Standard-formattable field type, borrowed as const T & during invocation.
 * The format string and policy are owned. Direct calls propagate formatting/allocation
 * exceptions and bad_expected_access<TextError>; table preparation retains its existing
 * CALLBACK_FAILED translation. Use formatText for explicit TextError handling instead.
 */
template <class T> class CellFormat {
public:
  /** @param pattern Format specification checked at compile time for const T &.
   * @param policy Captured display policy, which must match the receiving table.
   */
  explicit CellFormat(std::format_string<const T &> pattern, WidthPolicy policy = {})
      : pattern_(pattern.get()), policy_(policy) {
  }

  /** @param value Field borrowed for this synchronous invocation; never retained.
   * @return Independent formatted and validated text.
   * @throws std::bad_expected_access<TextError> If formatted bytes fail text validation.
   * @throws std::format_error If a custom standard formatter fails.
   * @throws std::bad_alloc If formatting or text allocation fails.
   */
  GraphemeText operator()(const T &value) const {
    return GraphemeText::from(std::vformat(pattern_, std::make_format_args(value)), policy_).value();
  }

private:
  std::string pattern_;
  WidthPolicy policy_;
};

/** @brief Build a reusable column formatter without a formatting lambda.
 * @tparam T Standard-formattable field type borrowed as const T &.
 * @param pattern Compile-time checked format specification for that field type.
 * @param policy Captured width policy matching the table.
 * @return Owning CellFormat<T>; copying its pattern may allocate.
 */
template <class T> CellFormat<T> cellFormat(std::format_string<const T &> pattern, WidthPolicy policy = {}) {
  return CellFormat<T>(pattern, policy);
}

} // namespace portals
