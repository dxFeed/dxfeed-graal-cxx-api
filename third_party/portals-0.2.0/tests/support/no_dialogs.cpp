// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0
// Test-build process setup only; this object is never linked into portals_core.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <cstdlib>
#ifdef _MSC_VER
#  include <crtdbg.h>
#endif

namespace {
constexpr DWORD FAILURE_EXIT = 3;

void diagnostic(const char *message) noexcept {
  DWORD length = 0;

  while (message[length] != '\0') {
    ++length;
  }

  DWORD written = 0;

  WriteFile(GetStdHandle(STD_ERROR_HANDLE), message, length, &written, nullptr);
}

[[noreturn]] void failProcess() noexcept {
  TerminateProcess(GetCurrentProcess(), FAILURE_EXIT);
  // TerminateProcess on the current process does not return on success.
  std::_Exit(FAILURE_EXIT);
}

LONG WINAPI unhandled(EXCEPTION_POINTERS *) noexcept {
  diagnostic("Portals test: unhandled native exception; no interactive debugger.\n");

  return EXCEPTION_EXECUTE_HANDLER;
}

#if defined(_MSC_VER) && defined(_DEBUG)
int __cdecl report(int type, char *message, int *) {
  if (type != _CRT_ASSERT && type != _CRT_ERROR) {
    return FALSE;
  }

  diagnostic("Portals test: fatal CRT report.\n");
  diagnostic(message);
  failProcess();
}

int __cdecl reportWide(int type, wchar_t *message, int *) {
  if (type != _CRT_ASSERT && type != _CRT_ERROR) {
    return FALSE;
  }

  diagnostic("Portals test: fatal wide CRT report.\n");

  char text[4096]{};

  if (WideCharToMultiByte(CP_UTF8, 0, message, -1, text, sizeof(text), nullptr, nullptr) > 0) {
    diagnostic(text);
  }

  failProcess();
}
#endif

struct TestProcessSetup {
  TestProcessSetup() noexcept {
    SetErrorMode(GetErrorMode() | SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX);
    SetUnhandledExceptionFilter(unhandled);
#ifdef _MSC_VER
    _set_error_mode(_OUT_TO_STDERR);
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
#  ifdef _DEBUG
    constexpr int REPORT_TYPES[]{_CRT_WARN, _CRT_ERROR, _CRT_ASSERT};

    for (int type : REPORT_TYPES) {
      _CrtSetReportMode(type, _CRTDBG_MODE_FILE);
      _CrtSetReportFile(type, _CRTDBG_FILE_STDERR);
    }

    if (_CrtSetReportHook2(_CRT_RPTHOOK_INSTALL, report) < 0 ||
        _CrtSetReportHookW2(_CRT_RPTHOOK_INSTALL, reportWide) < 0) {
      diagnostic("Portals test: cannot install noninteractive CRT reporting.\n");
      failProcess();
    }
#  endif
#endif
  }
};

const TestProcessSetup PROCESS_SETUP;
} // namespace
