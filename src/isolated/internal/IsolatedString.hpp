// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../../include/dxfeed_graal_cpp_api/detail/Conf.hpp"
#include "../../internal/utils/StringUtils.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include <vector>
#include <memory>

DXFCPP_BEGIN_NAMESPACE

namespace isolated::internal {

namespace IsolatedString {
/**
 * Calls the Graal SDK function `dxfg_v4_String_release` in isolation.
 * @param string The pointer to C-string
 * @return `true` if OK.
 *
 * @throws InvalidArgumentException if string is nullptr.
 * @throws JavaException if something happened with the dxFeed API backend.
 * @throws GraalException if something happened with the GraalVM.
 */
bool release(const char *string);

std::unique_ptr<const char, decltype(&release)> toUnique(const char *string);

} // namespace IsolatedString

namespace IsolatedStringList {
/**
 * Calls the Graal SDK function `dxfg_v4_string_list_release` in isolation.
 * @param stringList The pointer to dxFeed Graal SDK' string list.
 * @return `true` if OK.
 *
 * @throws InvalidArgumentException if stringList is nullptr.
 * @throws JavaException if something happened with the dxFeed API backend.
 * @throws GraalException if something happened with the GraalVM.
 */
bool release(/* dxfg_v4_string_list* */ void *stringList);

std::unique_ptr<void, decltype(&release)> toUnique(void *stringList);

} // namespace IsolatedStringList

template <typename L> struct NativeStringListWrapper final {
    /**
     * @param values The strings
     * @param label The place of the strings
     * @throws InvalidArgumentException if a string is not valid UTF-8; the strings created before it are freed.
     */
    NativeStringListWrapper(const std::vector<std::string> &values, const StringLabel &label) {
        if (values.empty()) {
            return;
        }

        list = new L{};

        try {
            list->size = fitToType<decltype(L::size)>(values.size());
            const auto size = static_cast<std::size_t>(list->size);

            list->elements = new const char *[size] {
                nullptr
            };

            for (std::size_t i = 0; i < size; i++) {
                if (!values[i].empty()) {
                    list->elements[i] = createCString(values[i], label);
                }
            }
        } catch (...) {
            free();

            throw;
        }
    }

    ~NativeStringListWrapper() {
        free();
    }

    L *list = nullptr;

    private:
    // The elements that are not created yet are nullptr.
    void free() noexcept {
        if (list) {
            if (list->elements) {
                for (int i = 0; i < list->size; i++) {
                    delete[] list->elements[i];
                }
            }

            delete[] list->elements;
            delete list;
            list = nullptr;
        }
    }
};

} // namespace isolated::internal

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()