// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#ifdef NDEBUG
#  undef NDEBUG
#endif
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#ifdef _MSC_VER
#  include <crtdbg.h>
#endif

int main(int argc, char **argv) {
  if (argc != 2) {
    return 2;
  }

  std::fprintf(stderr, "failure probe: %s\n", argv[1]);
  std::fflush(stderr);

  if (std::strcmp(argv[1], "assert") == 0) {
    assert(false && "intentional standard assertion");
  } else if (std::strcmp(argv[1], "abort") == 0) {
    std::abort();
  } else if (std::strcmp(argv[1], "exception") == 0) {
    RaiseException(0xe042504f, EXCEPTION_NONCONTINUABLE, 0, nullptr);
  } else {
#if defined(_MSC_VER) && defined(_DEBUG)
    if (std::strcmp(argv[1], "crt-assert") == 0) {
      _CrtDbgReport(_CRT_ASSERT, __FILE__, __LINE__, nullptr, "intentional CRT assertion");
    } else if (std::strcmp(argv[1], "crt-error") == 0) {
      _CrtDbgReport(_CRT_ERROR, __FILE__, __LINE__, nullptr, "intentional CRT error");
    } else if (std::strcmp(argv[1], "crt-wide") == 0) {
      _CrtDbgReportW(_CRT_ASSERT, L"no_dialogs.cpp", __LINE__, nullptr, L"intentional wide CRT assertion");
    } else {
      return 2;
    }
#else
    return 77;
#endif
  }

  // Continuing after a fatal report would silently accept a broken test.
  return 0;
}
