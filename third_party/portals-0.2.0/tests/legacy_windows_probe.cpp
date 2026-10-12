// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0

#include <portals/legacy.hpp>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <array>
#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

namespace {

constexpr DWORD CHILD_TIMEOUT_MS = 5000;
constexpr DWORD REQUIRED_OUTPUT_MODE = ENABLE_PROCESSED_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING;
constexpr DWORD OTHER_OUTPUT_MODE = ENABLE_WRAP_AT_EOL_OUTPUT;

struct Handle {
  HANDLE value{};

  ~Handle() {
    if (value && value != INVALID_HANDLE_VALUE) {
      CloseHandle(value);
    }
  }

  Handle(const Handle &) = delete;
  Handle &operator=(const Handle &) = delete;
  Handle() = default;
};

struct StdoutRestore {
  HANDLE original = GetStdHandle(STD_OUTPUT_HANDLE);

  ~StdoutRestore() {
    SetStdHandle(STD_OUTPUT_HANDLE, original);
  }
};

int child() {
  if (IsWindowVisible(GetConsoleWindow())) {
    return 10;
  }

  Handle privateBuffer;

  privateBuffer.value = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                                                  nullptr, CONSOLE_TEXTMODE_BUFFER, nullptr);

  if (privateBuffer.value == INVALID_HANDLE_VALUE) {
    return 11;
  }

  StdoutRestore restore;

  if (!SetStdHandle(STD_OUTPUT_HANDLE, privateBuffer.value)) {
    return 12;
  }

  const HANDLE input = GetStdHandle(STD_INPUT_HANDLE);
  DWORD inputBefore{};

  if (!input || input == INVALID_HANDLE_VALUE || !GetConsoleMode(input, &inputBefore)) {
    return 13;
  }

  const DWORD cookedInput =
    (inputBefore | ENABLE_PROCESSED_INPUT | ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT) & ~ENABLE_VIRTUAL_TERMINAL_INPUT;

  if (!SetConsoleMode(input, cookedInput) || !GetConsoleMode(input, &inputBefore) || inputBefore != cookedInput) {
    return 14;
  }

  const std::array<DWORD, 4> cases{0, ENABLE_PROCESSED_OUTPUT, ENABLE_VIRTUAL_TERMINAL_PROCESSING,
                                   REQUIRED_OUTPUT_MODE};

  for (std::size_t index = 0; index < cases.size(); ++index) {
    const DWORD before = OTHER_OUTPUT_MODE | cases[index];

    if (!SetConsoleMode(privateBuffer.value, before)) {
      return 20 + static_cast<int>(index);
    }

    DWORD actualBefore{};

    if (!GetConsoleMode(privateBuffer.value, &actualBefore) || actualBefore != before) {
      return 30 + static_cast<int>(index);
    }

    const auto result = portals::legacy::VirtualTerminal::tryEnable();

    if (!result || GetStdHandle(STD_OUTPUT_HANDLE) != privateBuffer.value) {
      return 40 + static_cast<int>(index);
    }

    DWORD actualAfter{};

    if (!GetConsoleMode(privateBuffer.value, &actualAfter) || actualAfter != (before | REQUIRED_OUTPUT_MODE)) {
      return 50 + static_cast<int>(index);
    }

    if (!portals::legacy::VirtualTerminal::enable()) {
      return 60 + static_cast<int>(index);
    }

    DWORD repeatedMode{};

    if (!GetConsoleMode(privateBuffer.value, &repeatedMode) || repeatedMode != actualAfter) {
      return 70 + static_cast<int>(index);
    }

    DWORD inputAfter{};

    if (!GetConsoleMode(input, &inputAfter) || inputAfter != inputBefore) {
      return 80 + static_cast<int>(index);
    }
  }

  if (!SetStdHandle(STD_OUTPUT_HANDLE, nullptr)) {
    return 90;
  }

  const auto missing = portals::legacy::VirtualTerminal::tryEnable();

  if (missing || missing.error().kind != portals::TerminalErrorKind::INVALID_OPTIONS ||
      missing.error().native_code != 0 || portals::legacy::VirtualTerminal::enable()) {
    return 91;
  }

  if (!SetStdHandle(STD_OUTPUT_HANDLE, restore.original)) {
    return 92;
  }

  return GetStdHandle(STD_OUTPUT_HANDLE) == restore.original ? 0 : 93;
}

int parent() {
  Handle inputRead;
  Handle inputWrite;
  Handle outputRead;
  Handle outputWrite;

  if (!CreatePipe(&inputRead.value, &inputWrite.value, nullptr, 0) ||
      !CreatePipe(&outputRead.value, &outputWrite.value, nullptr, 0)) {
    return 100;
  }

  HPCON console{};

  if (FAILED(CreatePseudoConsole(COORD{80, 25}, inputRead.value, outputWrite.value, 0, &console))) {
    return 101;
  }

  SIZE_T attributeBytes{};
  InitializeProcThreadAttributeList(nullptr, 1, 0, &attributeBytes);
  std::vector<unsigned char> attributeStorage(attributeBytes);
  auto *attributes = reinterpret_cast<PPROC_THREAD_ATTRIBUTE_LIST>(attributeStorage.data());

  if (!InitializeProcThreadAttributeList(attributes, 1, 0, &attributeBytes)) {
    ClosePseudoConsole(console);

    return 102;
  }

  if (!UpdateProcThreadAttribute(attributes, 0, PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE, console, sizeof(console), nullptr,
                                 nullptr)) {
    DeleteProcThreadAttributeList(attributes);
    ClosePseudoConsole(console);

    return 103;
  }

  std::array<wchar_t, 32768> path{};
  const DWORD length = GetModuleFileNameW(nullptr, path.data(), static_cast<DWORD>(path.size()));

  if (length == 0 || length == path.size()) {
    DeleteProcThreadAttributeList(attributes);
    ClosePseudoConsole(console);

    return 104;
  }

  std::wstring command = L"\"" + std::wstring(path.data(), length) + L"\" --child";
  STARTUPINFOEXW startup{};
  startup.StartupInfo.cb = sizeof(startup);
  startup.lpAttributeList = attributes;
  PROCESS_INFORMATION process{};

  const BOOL created =
    CreateProcessW(path.data(), command.data(), nullptr, nullptr, FALSE,
                   CREATE_NO_WINDOW | EXTENDED_STARTUPINFO_PRESENT, nullptr, nullptr, &startup.StartupInfo, &process);
  DeleteProcThreadAttributeList(attributes);

  if (!created) {
    ClosePseudoConsole(console);

    return 105;
  }

  Handle childProcess;
  Handle childThread;
  childProcess.value = process.hProcess;
  childThread.value = process.hThread;

  const DWORD wait = WaitForSingleObject(childProcess.value, CHILD_TIMEOUT_MS);

  if (wait != WAIT_OBJECT_0) {
    TerminateProcess(childProcess.value, 106);
    WaitForSingleObject(childProcess.value, CHILD_TIMEOUT_MS);
    ClosePseudoConsole(console);

    return 106;
  }

  DWORD exitCode{};
  const BOOL hasExitCode = GetExitCodeProcess(childProcess.value, &exitCode);
  ClosePseudoConsole(console);

  if (!hasExitCode) {
    return 107;
  }

  std::printf("legacy Windows probe child exit=%lu\n", exitCode);

  return static_cast<int>(exitCode);
}

} // namespace

int wmain(int argc, wchar_t **argv) {
  if (argc == 2 && std::wstring_view(argv[1]) == L"--child") {
    return child();
  }

  return parent();
}
