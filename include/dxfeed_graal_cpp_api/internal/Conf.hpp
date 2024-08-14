// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#ifdef _WINNT_
#    error Please, include windows.h or winnt.h after dxFeed Graal CXX API headers
#endif

#ifndef DXFCXX_VERSION
#    define DXFCXX_VERSION "0.0.0"
#endif

#if defined(_MSC_VER)
#    ifndef DXFCXX_DISABLE_MSC_WARNINGS_PUSH
#        define DXFCXX_DISABLE_MSC_WARNINGS_PUSH(...) __pragma(warning(push)) __pragma(warning(disable : __VA_ARGS__))
#        define DXFCXX_DISABLE_MSC_WARNINGS_POP() __pragma(warning(pop))
#    endif
#else
#    ifndef DXFCXX_DISABLE_MSC_WARNINGS_PUSH
#        define DXFCXX_DISABLE_MSC_WARNINGS_PUSH(warnings)
#        define DXFCXX_DISABLE_MSC_WARNINGS_POP()
#    endif
#endif

#ifdef DXFCPP_EXPORT
#    error DXFCPP_EXPORT was previously defined
#endif

#if defined(DXFCPP_USE_DLLS) && defined(_MSC_VER)
#    if defined(LIBDXFCPP_EXPORTS)
#        define DXFCPP_EXPORT __declspec(dllexport)
#        define DXFCPP_EXPORT_TEMPLATE_DECLARE
#        define DXFCPP_EXPORT_TEMPLATE_DEFINE __declspec(dllexport)
#    else
#        define DXFCPP_EXPORT __declspec(dllimport)
#        define DXFCPP_EXPORT_TEMPLATE_DECLARE
#        define DXFCPP_EXPORT_TEMPLATE_DEFINE __declspec(dllimport)
#    endif // defined(LIBDXFCPP_EXPORTS)
#elif defined(DXFCPP_USE_DLLS) && defined(LIBDXFCPP_EXPORTS)
#    define DXFCPP_EXPORT __attribute__((visibility("default")))
#    define DXFCPP_EXPORT_TEMPLATE_DECLARE __attribute__((visibility("default")))
#    define DXFCPP_EXPORT_TEMPLATE_DEFINE
#else
#    define DXFCPP_EXPORT
#    define DXFCPP_EXPORT_TEMPLATE_DECLARE
#    define DXFCPP_EXPORT_TEMPLATE_DEFINE
#endif

#ifndef DXFCPP_BEGIN_NAMESPACE
#    define DXFCPP_BEGIN_NAMESPACE                                                                                     \
        namespace dxfcpp {                                                                                             \
        inline namespace v3 {
#    define DXFCPP_END_NAMESPACE                                                                                       \
        }                                                                                                              \
        }
#endif

#ifdef __has_include
#    if __has_include(<ciso646> )
#        include <ciso646>
#    elif __has_include(<iso646.h>)
#        include <iso646.h>
#    endif
#else
#    include <ciso646>
#endif

DXFCPP_BEGIN_NAMESPACE

#if defined(__clang__)
constexpr bool isClangFlavouredCompiler = true;
#else
constexpr bool isClangFlavouredCompiler = false;
#endif

#ifdef _LIBCPP_VERSION
constexpr bool isLibCPP = true;
constexpr bool isLibCXX = false;
constexpr bool isMSSTL = false;
constexpr bool isUnknownSTL = false;
#elif __GLIBCXX__ // Note: only version 6.1 or newer define this in ciso646
constexpr bool isLibCPP = false;
constexpr bool isLibCXX = true;
constexpr bool isMSSTL = false;
constexpr bool isUnknownSTL = false;
#elif _CPPLIB_VER // Note: used by Visual Studio
constexpr bool isLibCPP = false;
constexpr bool isLibCXX = false;
constexpr bool isMSSTL = true;
constexpr bool isUnknownSTL = false;
#else
constexpr bool isLibCPP = false;
constexpr bool isLibCXX = false;
constexpr bool isMSSTL = false;
constexpr bool isUnknownSTL = true;
#endif

#ifndef DXFCPP_CXX20_CONSTEXPR_STRING
#    if defined(__cpp_lib_constexpr_string) && ((defined(__GNUC__) && __GNUC__ >= 12) || (defined(__clang_major__) && __clang_major__ >= 15) || (defined(_MSC_VER) && _MSC_VER >= 1929))
#        define DXFCPP_CXX20_CONSTEXPR_STRING constexpr
#    else
#        define DXFCPP_CXX20_CONSTEXPR_STRING
#    endif
#endif

DXFCPP_END_NAMESPACE