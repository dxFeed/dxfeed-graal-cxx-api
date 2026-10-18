// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0
// Include the private implementation to exercise its real mode guard without a production test hook.
#include "../tools/terminal_probe/native_windows.cpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace {
constexpr DWORD CHILD_TIMEOUT_MS = 5000;
constexpr DWORD CLEANUP_TIMEOUT_MS = 2000;
constexpr DWORD WATCHDOG_EXIT = 3;
constexpr std::string_view DA_REPLY = "\x1b[?1;2c";
constexpr std::string_view CPR_REPLY = "\x1b[1;1R";
constexpr std::string_view READY_MARKER = "host-cpr-ready";
constexpr char REPLY_BARRIER = '~';

void require(bool condition, const char *message) {
  if (!condition) {
    throw std::runtime_error(std::string(message) + " (Win32 error " + std::to_string(GetLastError()) + ")");
  }
}

void eventDescriptions() {
  portals::probe::RunResult result;
  INPUT_RECORD record{};

  record.EventType = KEY_EVENT;
  record.Event.KeyEvent.bKeyDown = TRUE;
  record.Event.KeyEvent.uChar.UnicodeChar = static_cast<WCHAR>(0xffff);
  record.Event.KeyEvent.wRepeatCount = 3;
  portals::probe::recordEvent(result, record);
  require(result.event_summary == " type=1 key_down=1 utf16=65535 repeat=3", "numeric key event description");

  record.Event.KeyEvent.bKeyDown = FALSE;
  record.Event.KeyEvent.uChar.UnicodeChar = 0;
  record.Event.KeyEvent.wRepeatCount = 0;
  portals::probe::recordEvent(result, record);
  require(result.event_summary == " type=1 key_down=1 utf16=65535 repeat=3 type=1 key_down=0 utf16=0 repeat=0",
          "key-up description remains numeric");

  result.event_summary.assign(portals::probe::EVENT_SUMMARY_LIMIT - 4, 'x');
  portals::probe::recordEvent(result, record);
  require(result.event_summary.size() == portals::probe::EVENT_SUMMARY_LIMIT && result.event_summary.ends_with(" typ"),
          "event summary truncates at the remaining byte limit");
  portals::probe::recordEvent(result, record);
  require(result.event_summary.size() == portals::probe::EVENT_SUMMARY_LIMIT && result.non_byte_events == 4,
          "full summary still counts events without appending");

  SetLastError(ERROR_ACCESS_DENIED);
  require(portals::probe::error("fixture") == "fixture: Win32 error 5",
          "native error code is sampled before formatting");
}

struct Handle {
  HANDLE value = nullptr;

  ~Handle() {
    reset();
  }

  void reset(HANDLE next = nullptr) {
    if (value != nullptr && value != INVALID_HANDLE_VALUE) {
      CloseHandle(value);
    }

    value = next;
  }
};

int child() {
  Handle input;
  Handle output;

  input.reset(CreateFileW(L"CONIN$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                          OPEN_EXISTING, 0, nullptr));
  output.reset(CreateFileW(L"CONOUT$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                           OPEN_EXISTING, 0, nullptr));
  require(SetStdHandle(STD_INPUT_HANDLE, input.value) && SetStdHandle(STD_OUTPUT_HANDLE, output.value) &&
            SetStdHandle(STD_ERROR_HANDLE, output.value),
          "install child console handles");
  require(!IsWindowVisible(GetConsoleWindow()), "headless ConPTY child");

  DWORD input_mode = 0;
  DWORD output_mode = 0;

  require(GetConsoleMode(input.value, &input_mode) && GetConsoleMode(output.value, &output_mode),
          "initial console modes");
  // Start without VT input, as a normal console consumer may leave it disabled.
  input_mode = (input_mode | ENABLE_EXTENDED_FLAGS | ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT | ENABLE_PROCESSED_INPUT |
                ENABLE_QUICK_EDIT_MODE) &
               ~ENABLE_VIRTUAL_TERMINAL_INPUT;
  require(SetConsoleMode(input.value, input_mode), "baseline input mode");

  const UINT input_page = GetConsoleCP();
  const UINT output_page = GetConsoleOutputCP();

  require(portals::probe::inspectNative().query_eligible, "native inspection eligibility");
  require(SetConsoleCursorPosition(output.value, {19, 9}) != FALSE, "distinct system CPR position");

  portals::probe::ModeGuard query_modes(input.value, output.value);

  require(query_modes.acquire(), "acquire actual native query modes");

  DWORD written = 0;

  require(WriteFile(output.value, READY_MARKER.data(), static_cast<DWORD>(READY_MARKER.size()), &written, nullptr) &&
            written == READY_MARKER.size(),
          "announce host-input barrier");

  const auto barrier_deadline = std::chrono::steady_clock::now() + std::chrono::seconds(1);
  bool reply_queued = false;

  while (!reply_queued && std::chrono::steady_clock::now() < barrier_deadline) {
    require(WaitForSingleObject(input.value, 1000) == WAIT_OBJECT_0, "host reply readiness");

    INPUT_RECORD records[64];
    DWORD count = 0;

    require(PeekConsoleInputW(input.value, records, 64, &count) != FALSE, "peek host reply barrier");

    for (DWORD index = 0; index < count; ++index) {
      reply_queued |= records[index].EventType == KEY_EVENT && records[index].Event.KeyEvent.bKeyDown &&
                      records[index].Event.KeyEvent.uChar.UnicodeChar == REPLY_BARRIER;
    }
  }

  require(reply_queued, "entire host reply passed through ConPTY input parsing");

  const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(1);
  portals::probe::QueryCollector collector(deadline, 1024);
  const auto result = portals::probe::runNativeQueries(collector, deadline, 1024);

  collector.finish();
  require(query_modes.restore(), "restore outer query modes");

  DWORD restored_input = 0;
  DWORD restored_output = 0;

  require(GetConsoleMode(input.value, &restored_input) && GetConsoleMode(output.value, &restored_output) &&
            restored_input == input_mode && restored_output == output_mode && GetConsoleCP() == input_page &&
            GetConsoleOutputCP() == output_page,
          "exact mode/code-page restoration");

  const std::string report =
    "stop=" + result.stop + " da=" + portals::probe::escapeBytes(collector.deviceAttributes().value_or("missing")) +
    " cpr=" + portals::probe::escapeBytes(collector.cursorPosition().value_or("missing")) +
    " non_byte_events=" + std::to_string(result.non_byte_events) + " records=" + result.event_summary + " restored=1\n";

  require(WriteFile(output.value, report.data(), static_cast<DWORD>(report.size()), &written, nullptr) != FALSE,
          "child diagnostic report");
  require(result.error.empty(), "native query error");
  require(collector.deviceAttributes().has_value(), "DA response collected through native records");
  require(collector.cursorPosition() == CPR_REPLY, "CPR response preserved through native records");
  require(result.stop == "replies", "both replies complete the query window");

  return 0;
}

struct Session {
  Handle input_read, input_write, output_read, output_write;
  Handle process, process_thread, job;
  HPCON console = nullptr;
  std::atomic<bool> stop{false};
  std::thread reader;
  std::string transcript;
  std::string reader_error;
  bool da_sent = false;
  bool cpr_sent = false;

  ~Session() {
    close();
  }

  void close() {
    if (process.value && WaitForSingleObject(process.value, 0) == WAIT_TIMEOUT) {
      TerminateProcess(process.value, WATCHDOG_EXIT);
      TerminateJobObject(job.value, WATCHDOG_EXIT);
      WaitForSingleObject(process.value, CLEANUP_TIMEOUT_MS);
    }

    if (console) {
      // ConPTY teardown may write final output: keep the drain alive until it returns.
      ClosePseudoConsole(console);
      console = nullptr;
    }

    stop.store(true);

    if (reader.joinable()) {
      reader.join();
    }

    input_write.reset();
  }

  void drain() {
    char buffer[4096];

    try {
      while (true) {
        DWORD available = 0;

        if (!PeekNamedPipe(output_read.value, nullptr, 0, nullptr, &available, nullptr)) {
          break;
        }

        if (available == 0) {
          if (stop.load()) {
            break;
          }

          Sleep(1);

          continue;
        }

        DWORD count = 0;

        require(ReadFile(output_read.value, buffer, std::min(available, static_cast<DWORD>(sizeof(buffer))), &count,
                         nullptr) != FALSE,
                "drain child output");
        transcript.append(buffer, count);

        if (!da_sent && transcript.find(READY_MARKER) != std::string::npos) {
          respond(std::string(DA_REPLY) + std::string(CPR_REPLY) + REPLY_BARRIER);
          da_sent = true;
          cpr_sent = true;
        }
      }
    } catch (const std::exception &error) {
      reader_error = error.what();
    }
  }

  void respond(std::string_view reply) {
    DWORD written = 0;

    require(WriteFile(input_write.value, reply.data(), static_cast<DWORD>(reply.size()), &written, nullptr) &&
              written == reply.size(),
            "host query reply");
  }
};

void launch(Session &session) {
  HANDLE read = nullptr;
  HANDLE write = nullptr;

  require(CreatePipe(&read, &write, nullptr, 4096), "ConPTY input pipe");
  session.input_read.reset(read);
  session.input_write.reset(write);
  require(CreatePipe(&read, &write, nullptr, 4096), "ConPTY output pipe");
  session.output_read.reset(read);
  session.output_write.reset(write);
  require(
    SUCCEEDED(CreatePseudoConsole({80, 25}, session.input_read.value, session.output_write.value, 0, &session.console)),
    "CreatePseudoConsole");
  session.reader = std::thread([&session] {
    session.drain();
  });
  session.job.reset(CreateJobObjectW(nullptr, nullptr));
  require(session.job.value != nullptr, "child job");

  JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits{};

  limits.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
  require(SetInformationJobObject(session.job.value, JobObjectExtendedLimitInformation, &limits, sizeof(limits)) !=
            FALSE,
          "child job kill-on-close");

  SIZE_T bytes = 0;

  InitializeProcThreadAttributeList(nullptr, 1, 0, &bytes);

  std::vector<unsigned char> storage(bytes);
  const auto attributes = reinterpret_cast<PPROC_THREAD_ATTRIBUTE_LIST>(storage.data());

  require(InitializeProcThreadAttributeList(attributes, 1, 0, &bytes) != FALSE, "child attributes");

  struct AttributeCleanup {
    PPROC_THREAD_ATTRIBUTE_LIST value;

    ~AttributeCleanup() {
      DeleteProcThreadAttributeList(value);
    }
  } cleanup{attributes};

  require(UpdateProcThreadAttribute(attributes, 0, PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE, session.console,
                                    sizeof(session.console), nullptr, nullptr) != FALSE,
          "child pseudoconsole attribute");

  wchar_t path[32768];
  const DWORD length = GetModuleFileNameW(nullptr, path, 32768);

  require(length != 0 && length < 32768, "self executable path");

  std::wstring command = L"\"" + std::wstring(path, length) + L"\" --child";
  STARTUPINFOEXW startup{};

  startup.StartupInfo.cb = sizeof(startup);
  startup.lpAttributeList = attributes;

  PROCESS_INFORMATION process{};

  require(CreateProcessW(path, command.data(), nullptr, nullptr, FALSE, EXTENDED_STARTUPINFO_PRESENT | CREATE_SUSPENDED,
                         nullptr, nullptr, &startup.StartupInfo, &process) != FALSE,
          "ConPTY child process");
  session.process.reset(process.hProcess);
  session.process_thread.reset(process.hThread);
  require(AssignProcessToJobObject(session.job.value, session.process.value) != FALSE, "assign owned child to job");
  require(ResumeThread(session.process_thread.value) != static_cast<DWORD>(-1), "resume child");
  session.input_read.reset();
  session.output_write.reset();
}
} // namespace

int wmain(int argc, wchar_t **argv) {
  try {
    if (argc == 2 && std::wstring_view(argv[1]) == L"--child") {
      return child();
    }

    eventDescriptions();

    Session session;

    launch(session);

    const DWORD waited = WaitForSingleObject(session.process.value, CHILD_TIMEOUT_MS);
    DWORD child_exit = STILL_ACTIVE;

    require(GetExitCodeProcess(session.process.value, &child_exit) != FALSE, "child exit code");
    session.close();
    std::cout << "child_exit=" << child_exit << " da_sent=" << session.da_sent << " cpr_sent=" << session.cpr_sent
              << " output=" << portals::probe::escapeBytes(session.transcript) << '\n';
    require(waited == WAIT_OBJECT_0, "child wall-clock watchdog");
    require(session.reader_error.empty(), session.reader_error.c_str());
    require(session.da_sent && session.cpr_sent, "host injected DA/CPR at the native mode barrier");
    require(child_exit == 0, "native DA/CPR and restoration assertions");

    return 0;
  } catch (const std::exception &error) {
    std::cerr << "FAIL: " << error.what() << '\n';

    return 1;
  }
}
