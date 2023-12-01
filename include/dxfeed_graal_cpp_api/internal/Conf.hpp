// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#ifdef _WINNT_
#    error Please, include windows.h or winnt.h after dxFeed Graal CXX API headers
#endif

#ifndef DXFCXX_VERSION
#define DXFCXX_VERSION "0.0.0"
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

#ifndef DXFCPP_CXX20_CONSTEXPR_STRING
#    if defined(__cpp_lib_constexpr_string)
#        define DXFCPP_CXX20_CONSTEXPR_STRING constexpr
#    else
#        define DXFCPP_CXX20_CONSTEXPR_STRING
#    endif
#endif