// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include <vector>

DXFCPP_BEGIN_NAMESPACE

namespace isolated::internal {

namespace IsolatedString {
/**
 * Calls the Graal SDK function `dxfg_String_release` in isolation.
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
 * Calls the Graal SDK function `dxfg_CList_String_release` in isolation.
 * @param stringList The pointer to dxFeed Graal SDK' string list.
 * @return `true` if OK.
 *
 * @throws InvalidArgumentException if stringList is nullptr.
 * @throws JavaException if something happened with the dxFeed API backend.
 * @throws GraalException if something happened with the GraalVM.
 */
bool release(/* dxfg_string_list* */ void *stringList);

std::unique_ptr<void, decltype(&release)> toUnique(void *stringList);

} // namespace IsolatedStringList

template <typename L> struct NativeStringListWrapper final {
    explicit NativeStringListWrapper(const std::vector<std::string> &values) {
        if (values.empty()) {
            list = nullptr;
        } else {
            list = new L{};
            list->size = fitToType<decltype(L::size)>(values.size());
            list->elements = new const char *[list->size] {
                nullptr
            };

            for (int i = 0; i < list->size; i++) {
                if (!values[i].empty()) {
                    list->elements[i] = createCString(values[i]);
                }
            }
        }
    }

    ~NativeStringListWrapper() {
        if (list) {
            for (int i = 0; i < list->size; i++) {
                delete[] list->elements[i];
            }

            delete[] list->elements;
            delete list;
        }
    }

    L *list = nullptr;
};

} // namespace isolated::internal

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()