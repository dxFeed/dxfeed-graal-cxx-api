// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "native_cleanup.hpp"
#include <array>
#include <windows.h>

namespace native {
namespace {
struct Handle {
  HANDLE value{};

  ~Handle() {
    if (value && value != INVALID_HANDLE_VALUE) {
      CloseHandle(value);
    }
  }
};

struct Snapshot {
  DWORD input{}, output{};
  UINT inputPage{}, outputPage{};
  bool valid{};
  bool operator==(const Snapshot &) const = default;
};

Snapshot snapshot(HANDLE input, HANDLE output, std::ostream &log, const char *phase) {
  Snapshot value;
  value.valid = GetConsoleMode(input, &value.input) && GetConsoleMode(output, &value.output);
  value.inputPage = GetConsoleCP();
  value.outputPage = GetConsoleOutputCP();
  log << "phase=" << phase << " valid=" << value.valid << " input_mode=" << value.input
      << " output_mode=" << value.output << " input_page=" << value.inputPage << " output_page=" << value.outputPage
      << '\n';

  return value;
}

std::wstring wide(const std::string &value) {
  const int count =
    MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value.data(), static_cast<int>(value.size()), nullptr, 0);
  std::wstring result(static_cast<std::size_t>(count), L'\0');
  MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value.data(), static_cast<int>(value.size()), result.data(),
                      count);

  return result;
}

// Match CommandLineToArgvW/CRT rules, including backslashes before quotes.
std::wstring quote(const std::wstring &value) {
  std::wstring result = L"\"";
  std::size_t slashes{};

  for (const auto character : value) {
    if (character == L'\\') {
      ++slashes;

      continue;
    }

    result.append(slashes * (character == L'"' ? 2 : 1), L'\\');
    slashes = 0;

    if (character == L'"') {
      result += L'\\';
    }

    result += character;
  }

  result.append(slashes * 2, L'\\');
  result += L'"';

  return result;
}

struct WindowSearch {
  std::wstring title;
  HWND window{};
  unsigned count{};
};

BOOL CALLBACK findWindow(HWND window, LPARAM argument) {
  auto &search = *reinterpret_cast<WindowSearch *>(argument);
  std::array<wchar_t, 512> title{};
  GetWindowTextW(window, title.data(), static_cast<int>(title.size()));

  if (search.title == title.data()) {
    search.window = window;
    ++search.count;
  }

  return TRUE;
}

bool key(HANDLE input, WORD code, wchar_t character = 0, DWORD modifiers = 0) {
  std::array<INPUT_RECORD, 2> events{};

  for (std::size_t i = 0; i < events.size(); ++i) {
    events[i].EventType = KEY_EVENT;
    events[i].Event.KeyEvent = {i == 0, 1, code, 0, {character}, modifiers};
  }

  DWORD written{};

  return WriteConsoleInputW(input, events.data(), static_cast<DWORD>(events.size()), &written) &&
         written == events.size();
}

bool drive(HANDLE input, const Options &options, int stage, std::ostream &log) {
  bool passed = true;

  if (stage == 0) {
    passed = key(input, VK_F2);
  } else if (stage == 1) {
    passed = key(input, VK_TAB, L'\t') && key(input, 'A', 1, LEFT_CTRL_PRESSED);

    // Exercise UTF-16 surrogate assembly as well as combining and wide input.
    for (const wchar_t character : std::wstring_view(L"Native note \u754c e\u0301 \U0001f642")) {
      passed = key(input, 0, character) && passed;
    }

    passed = key(input, VK_RETURN, L'\r') && passed;
  } else if (stage == 2) {
    passed =
      key(input, VK_TAB, L'\t') && key(input, VK_TAB, L'\t') && key(input, VK_DOWN) && key(input, VK_RETURN, L'\r');
  } else if (stage == 3) {
    std::array<INPUT_RECORD, 2> events{};

    for (std::size_t i = 0; i < events.size(); ++i) {
      events[i].EventType = MOUSE_EVENT;
      events[i].Event.MouseEvent = {{8, 7}, i == 0 ? DWORD(FROM_LEFT_1ST_BUTTON_PRESSED) : DWORD(0), 0, 0};
    }

    DWORD written{};
    passed = WriteConsoleInputW(input, events.data(), 2, &written) && written == 2;
  } else if (stage == 4 || stage == 5) {
    WindowSearch search{wide(options.title)};
    EnumWindows(findWindow, reinterpret_cast<LPARAM>(&search));
    RECT rectangle{};
    DWORD pid{};

    if (search.count != 1 || !GetWindowRect(search.window, &rectangle)) {
      passed = false;
    } else {
      GetWindowThreadProcessId(search.window, &pid);
      const int change = stage == 4 ? -120 : 120;
      passed = SetWindowPos(search.window, nullptr, 0, 0, rectangle.right - rectangle.left + change,
                            rectangle.bottom - rectangle.top + change / 2, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
      log << "window_pid=" << pid << " resize_delta=" << change << '\n';
    }
  } else {
    passed = key(input, VK_F10);
  }

  log << "driver_stage=" << stage << " passed=" << passed << '\n';
  log.flush();

  return passed;
}
} // namespace

bool supervise(const Options &options, std::ostream &log) {
  Handle input{CreateFileW(L"CONIN$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                           OPEN_EXISTING, 0, nullptr)};
  Handle output{CreateFileW(L"CONOUT$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                            OPEN_EXISTING, 0, nullptr)};
  const auto before = snapshot(input.value, output.value, log, "before");
  CONSOLE_SCREEN_BUFFER_INFO buffer{};
  GetConsoleScreenBufferInfo(output.value, &buffer);
  log << "harness_pid=" << GetCurrentProcessId()
      << " initial_viewport=" << buffer.srWindow.Right - buffer.srWindow.Left + 1 << 'x'
      << buffer.srWindow.Bottom - buffer.srWindow.Top + 1 << '\n';
  Handle job{CreateJobObjectW(nullptr, nullptr)};
  JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits{};
  limits.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;

  if (!before.valid || !job.value ||
      !SetInformationJobObject(job.value, JobObjectExtendedLimitInformation, &limits, sizeof(limits))) {
    return false;
  }

  std::wstring command;

  for (const auto &argument : options.child) {
    command += quote(wide(argument)) + L' ';
  }

  STARTUPINFOW startup{sizeof(startup)};
  PROCESS_INFORMATION process{};

  if (!CreateProcessW(wide(options.child.front()).c_str(), command.data(), nullptr, nullptr, FALSE, CREATE_SUSPENDED,
                      nullptr, nullptr, &startup, &process)) {
    log << "create_error=" << GetLastError() << '\n';

    return false;
  }

  Handle child{process.hProcess};
  Handle thread{process.hThread};
  log << "child_pid=" << process.dwProcessId << '\n';
  log.flush();

  if (!AssignProcessToJobObject(job.value, child.value) || ResumeThread(thread.value) == DWORD(-1)) {
    TerminateProcess(child.value, 124);
    WaitForSingleObject(child.value, 2000);

    return false;
  }

  const auto start = GetTickCount64();
  const std::array<int, 7> schedule{1000, 1300, 1800, 2200, 2600, 3200, options.quitMs};
  std::size_t stage{};
  bool driverPassed = true;
  DWORD waited = WAIT_TIMEOUT;
  COORD previousSize{static_cast<SHORT>(buffer.srWindow.Right - buffer.srWindow.Left + 1),
                     static_cast<SHORT>(buffer.srWindow.Bottom - buffer.srWindow.Top + 1)};

  while (GetTickCount64() - start < static_cast<ULONGLONG>(options.deadlineMs)) {
    waited = WaitForSingleObject(child.value, 20);

    if (waited != WAIT_TIMEOUT) {
      break;
    }

    if (options.drive && stage < schedule.size() &&
        GetTickCount64() - start >= static_cast<ULONGLONG>(schedule[stage])) {
      driverPassed = drive(input.value, options, static_cast<int>(stage++), log) && driverPassed;
    }

    if (GetConsoleScreenBufferInfo(output.value, &buffer)) {
      const COORD size{static_cast<SHORT>(buffer.srWindow.Right - buffer.srWindow.Left + 1),
                       static_cast<SHORT>(buffer.srWindow.Bottom - buffer.srWindow.Top + 1)};

      if (size.X != previousSize.X || size.Y != previousSize.Y) {
        log << "observed_viewport=" << size.X << 'x' << size.Y << '\n';
        previousSize = size;
      }
    }
  }

  const bool watchdog = waited != WAIT_OBJECT_0;

  if (watchdog) {
    TerminateJobObject(job.value, 124);
    waited = WaitForSingleObject(child.value, 2000);
  }

  DWORD code = STILL_ACTIVE;
  GetExitCodeProcess(child.value, &code);
  const auto after = snapshot(input.value, output.value, log, "after");
  const bool restored = before.valid && after.valid && before == after;
  log << "child_exit=" << code << " watchdog=" << watchdog << " exact_restore=" << restored
      << " elapsed_ms=" << GetTickCount64() - start << " own_child_gone=" << (waited == WAIT_OBJECT_0) << '\n';

  return code == static_cast<DWORD>(options.expectedExit) && !watchdog && restored && driverPassed &&
         (!options.drive || stage == schedule.size());
}
} // namespace native
