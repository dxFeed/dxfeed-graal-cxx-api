// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0
#include "../examples/gallery/gallery.hpp"
#include "support/test_terminal.hpp"
#include <portals/line_edit.hpp>

#include <array>
#include <atomic>
#include <cstdlib>
#include <iostream>
#include <thread>
#ifdef _WIN32
#  define WIN32_LEAN_AND_MEAN
#  define NOMINMAX
#  include <windows.h>
#else
#  include <fcntl.h>
#  include <poll.h>
#  include <sys/wait.h>
#  include <unistd.h>
#endif

namespace allocation_test {
thread_local std::ptrdiff_t remaining = -1;
}

void *operator new(std::size_t size) {
  if (allocation_test::remaining == 0) {
    throw std::bad_alloc();
  }

  if (allocation_test::remaining > 0) {
    --allocation_test::remaining;
  }

  if (void *value = std::malloc(size ? size : 1)) {
    return value;
  }

  throw std::bad_alloc();
}

void operator delete(void *value) noexcept {
  std::free(value);
}

void operator delete(void *value, std::size_t) noexcept {
  std::free(value);
}

void *operator new[](std::size_t size) {
  return ::operator new(size);
}

void operator delete[](void *value) noexcept {
  ::operator delete(value);
}

void operator delete[](void *value, std::size_t) noexcept {
  ::operator delete(value);
}

void *operator new(std::size_t size, const std::nothrow_t &) noexcept {
  try {
    return ::operator new(size);
  } catch (...) {
    return nullptr;
  }
}

void *operator new[](std::size_t size, const std::nothrow_t &) noexcept {
  return ::operator new(size, std::nothrow);
}

void operator delete(void *value, const std::nothrow_t &) noexcept {
  ::operator delete(value);
}

void operator delete[](void *value, const std::nothrow_t &) noexcept {
  ::operator delete(value);
}

namespace {
using namespace portals;
using gallery::take;
int failures{};
constexpr auto COMBINING = "e\xcc\x81";
constexpr auto EMOJI = "\xf0\x9f\x91\xa9\xe2\x80\x8d\xf0\x9f\x92\xbb";
constexpr auto CJK = "\xe7\x95\x8c";

void check(bool condition, const char *message) {
  if (!condition) {
    std::cerr << message << '\n';
    ++failures;
  }
}

struct FakeClipboard : Clipboard {
  std::string bytes;
  bool fail{};
  int reads{};
  int writes{};
  std::function<void()> reenter;

  std::expected<std::string, TreeError> read(std::size_t) override {
    ++reads;

    if (reenter) {
      reenter();
    }

    if (fail) {
      return std::unexpected(TreeError::CLOSED);
    }

    return bytes;
  }

  std::expected<void, TreeError> write(std::string_view selected) override {
    ++writes;

    if (reenter) {
      reenter();
    }

    if (fail) {
      return std::unexpected(TreeError::CLOSED);
    }

    bytes = selected;

    return {};
  }
};

struct Fixture {
  std::unique_ptr<Scheduler> scheduler = take(Scheduler::create({.data_capacity = 1}));
  std::unique_ptr<WidgetTree> tree = take(WidgetTree::create(*scheduler));
  Node<Widget> root = take(tree->root());
  std::unique_ptr<EventRouter> router = take(EventRouter::create(*tree));
  Node<Column> parent = take(tree->emplace<Column>(root));
  Node<LineEdit> node;
  std::shared_ptr<Property<std::string>> model;
  LineEditResult result;
  std::function<void(LineEditResult)> callback;
  std::optional<LineEditBinding> binding;
  InputDecoder decoder = take(InputDecoder::create());

  explicit Fixture(std::string initial = "", LineEditOptions options = {}, std::shared_ptr<Clipboard> clipboard = {},
                   std::function<bool(std::string_view)> validate = {}) {
    node = take(tree->emplace<LineEdit>(parent, options));
    model = take(Property<std::string>::create(*tree, root, std::move(initial), Invalidation::LAYOUT));
    binding.emplace(take(bindLineEdit(
      *tree, *router, node, model,
      [&](LineEditResult value) {
        result = value;

        if (callback) {
          callback(std::move(value));
        }
      },
      std::move(clipboard), std::move(validate))));
  }

  void input(std::string_view bytes) {
    do {
      auto step = take(decoder.next(bytes, 0));

      bytes.remove_prefix(step.consumed);

      if (step.event) {
        take(router->dispatch(*step.event));
      }

      if (bytes.empty() && !step.event) {
        break;
      }
    } while (true);
  }

  void command(Action action, bool shift = false) {
    take(router->invoke(node, {action, {.shift = shift}}));
  }

  std::string draft() {
    std::string value;

    take(tree->read(node, [&](const LineEdit &edit) {
      value = edit.text().bytes();
    }));

    return value;
  }

  CellBuffer render(CellSize size) {
    take(binding->sync(take(router->focused())));

    const auto viewport = take(CellRect::from({}, size));
    std::optional<CellBuffer> buffer;

    take(tree->frame([&] {
      take(layoutTree(*tree, root, viewport, viewport));

      std::vector<RenderCommand> commands;
      gallery::SurfaceSink sink(take(tree->inspect(node)).geometry, commands);

      take(tree->paint(node, sink));
      buffer.emplace(take(renderFullFrame(size, commands)));
    }));

    return std::move(*buffer);
  }
};

void graphemeEditing() {
  Fixture f;
  const auto text = std::string(COMBINING) + EMOJI + CJK;

  f.input(text);
  check(f.draft() == text, "decoded Unicode remains exact in local draft");
  check(take(f.model->snapshot())->value.empty(), "typing does not commit Property");
  f.input("\x1b[D\x7f");
  check(f.draft() == std::string(COMBINING) + CJK, "left plus Backspace erases whole ZWJ emoji");
  f.input("\x1b[3~");
  check(f.draft() == COMBINING, "Delete erases whole CJK grapheme");
  f.input("\x7f");
  check(f.draft().empty(), "Backspace erases combining sequence atomically");
  f.input(text);
  f.command(Action::MOVE_HOME);
  f.command(Action::MOVE_RIGHT, true);
  f.command(Action::MOVE_RIGHT, true);
  take(f.tree->read(f.node, [&](const LineEdit &edit) {
    check(edit.anchor() == 0 && edit.cursor() == std::string(COMBINING).size() + std::string(EMOJI).size(),
          "Shift+arrows select byte boundaries of entire combining and emoji clusters");
  }));
  f.input("X");
  check(f.draft() == std::string("X") + CJK, "typing replaces selected complete graphemes");
  f.command(Action::MOVE_HOME);
  f.command(Action::MOVE_END, true);
  f.command(Action::MOVE_LEFT);
  take(f.tree->read(f.node, [&](const LineEdit &edit) {
    check(edit.cursor() == 0 && edit.anchor() == 0, "unmodified left collapses selection to its beginning");
  }));
  f.command(Action::MOVE_END, true);
  f.command(Action::ERASE_FORWARD);
  check(f.draft().empty(), "Delete erases selected full range");
  take(f.router->dispatch(TextInputEvent{U'A', {.shift = true}}));
  take(f.router->dispatch(KeyEvent{InputKey::BACKSPACE, 0, {}, KeyAction::RELEASE}));
  check(f.draft() == "A", "native shifted text inserts while key releases do not edit");
}

void conflictAndOverflow() {
  Fixture f("old");

  take(f.model->set("new"));
  f.input("!");
  check(f.draft() == "new!", "first edit adopts latest model even before queued notification");
  take(f.scheduler->dispatch());
  check(f.draft() == "new!", "old notification cannot erase uncommitted input");
  take(f.model->set("external"));
  take(f.model->set("latest"));
  take(f.binding->sync(take(f.router->focused())));
  check(f.draft() == "new!", "external synchronization preserves dirty draft");
  f.input("\r");
  check(f.result.status == LineEditStatus::CONFLICT && f.draft() == "new!" &&
          take(f.model->snapshot())->value == "latest",
        "Enter conflict leaves both draft and model intact");
  f.command(Action::CANCEL);
  check(f.draft() == "latest", "Escape adopts newest model, including rejected notifications");
  f.input("!\r");
  check(f.result.status == LineEditStatus::COMMITTED && f.result.commit && f.result.commit->rejected == 1 &&
          f.result.commit->snapshot->value == "latest!",
        "commit remains successful when observer queue rejects notification");
  take(f.scheduler->dispatch());
  check(f.draft() == "latest!", "old accepted event reads latest snapshot after successful commit");
  take(f.model->set("again"));
  take(f.scheduler->dispatch());
  check(f.draft() == "again", "ordinary subscription refreshes a clean editor");

  bool wrongThread{};
  std::thread worker([&] {
    auto result = f.binding->sync({});

    wrongThread = !result && result.error() == TreeError::WRONG_THREAD;
  });

  worker.join();
  check(wrongThread, "binding synchronization rejects foreign thread");
  take(f.tree->frame([&] {
    auto result = f.binding->sync({});

    check(!result && result.error() == TreeError::WOULD_BLOCK, "frame cannot mutate editor presentation");
  }));
}

void clipboardAndValidation() {
  auto clipboard = std::make_shared<FakeClipboard>();
  Fixture f("start", {}, clipboard, [](std::string_view text) {
    return text != "bad";
  });

  f.input("\x01\x03");
  check(clipboard->bytes == "start" && f.draft() == "start", "Ctrl+A/C copy exact selected bytes");
  clipboard->fail = true;
  f.input("\x18");
  check(f.result.status == LineEditStatus::CLIPBOARD_FAILED && f.draft() == "start",
        "failed cut never deletes selection");
  clipboard->fail = false;
  f.input("\x18");
  check(f.draft().empty() && clipboard->bytes == "start", "successful cut removes selected draft only");
  f.input("\x16");
  check(f.draft() == "start", "Ctrl+V inserts owning service payload");
  f.input("\x01"
          "bad\r");
  check(f.result.status == LineEditStatus::VALIDATION_FAILED && f.draft() == "bad" &&
          take(f.model->snapshot())->value == "start",
        "validator rejection preserves uncommitted candidate");
  f.command(Action::CANCEL);
  check(f.draft() == "start", "Escape discards invalid draft");
  clipboard->bytes = "\xff";
  f.input("\x16");
  check(f.result.status == LineEditStatus::INVALID_TEXT && f.draft() == "start",
        "invalid clipboard UTF-8 is rejected whole");
  clipboard->bytes.assign(5000, 'x');
  f.input("\x16");
  check(f.result.status == LineEditStatus::LIMIT_EXCEEDED && f.draft() == "start", "service cannot bypass byte bound");

  const auto reads = clipboard->reads;
  const auto writes = clipboard->writes;

  f.render({5, 1});
  check(clipboard->reads == reads && clipboard->writes == writes,
        "measure arrange and paint perform no clipboard calls");
}

void pasteAndRendering() {
  Fixture f;

  for (const auto bytes : {"a\nb", "a\rb",
                           "a\xe2\x80\xa8"
                           "b",
                           "a\xe2\x80\xa9"
                           "b",
                           "\xff"}) {
    take(f.router->dispatch(PasteInputEvent{bytes}));
    check(f.result.status == LineEditStatus::INVALID_TEXT && f.draft().empty(),
          "invalid or multiline paste preserves empty draft");
  }

  std::string large(4097, 'x');

  take(f.router->dispatch(PasteInputEvent{large}));
  check(f.result.status == LineEditStatus::LIMIT_EXCEEDED && f.draft().empty(),
        "over-limit paste is explicitly rejected without truncation");
  f.input("\x1b[200~A\x1b[31m\x01Z\x1b[201~");
  check(f.draft() == "A\x1b[31m\x01Z", "bracketed paste retains ANSI and C0 as logical data");
  f.command(Action::MOVE_HOME);

  test_support::FakeClock clock;
  test_support::TestTerminal terminal(clock, {16, 1});
  FrameWriter writer(terminal);

  take(writer.submit(f.render({16, 1})));
  check(!writer.flush(TerminalDeadline::max()).error, "safe pasted data passes real frame writer");

  const auto snapshot = terminal.snapshot();
  std::string painted;

  for (const auto &cell : snapshot.cells) {
    if (!cell.tail) {
      painted += cell.glyph;
    }
  }

  check(painted.starts_with("A?[31m?Z"), "VT oracle sees control placeholders, never injected terminal escapes");
  check(!snapshot.cells[1].style.foreground, "pasted ANSI cannot change terminal color");

  Fixture unicode(std::string(COMBINING) + EMOJI + CJK);

  unicode.command(Action::MOVE_HOME);
  unicode.command(Action::MOVE_RIGHT);

  auto narrow = unicode.render({2, 1});

  check(take(narrow.at({0, 0})).glyph == EMOJI && take(narrow.at({1, 0})).cell.kind == CellKind::CONTINUATION &&
          take(narrow.at({0, 0})).style.underline,
        "narrow viewport reveals whole wide emoji under caret");
  unicode.command(Action::MOVE_END);
  narrow = unicode.render({2, 1});
  check(take(narrow.at({0, 0})).style.underline || take(narrow.at({1, 0})).style.underline,
        "caret at text end stays visible in narrow viewport");
  unicode.command(Action::MOVE_LEFT);
  narrow = unicode.render({1, 1});
  check(take(narrow.at({0, 0})).glyph == " " && take(narrow.at({0, 0})).style.underline,
        "one-cell viewport shows caret without half a CJK glyph");
  unicode.command(Action::MOVE_HOME);
  unicode.command(Action::MOVE_RIGHT, true);

  const auto selected = unicode.render({8, 1});

  check(take(selected.at({0, 0})).glyph == COMBINING &&
          take(selected.at({0, 0})).style.background == RgbColor{180, 200, 255},
        "selection paint includes the complete combining grapheme");
}

void callbackLifetimes() {
  Fixture f;
  int calls{};

  f.callback = [&](LineEditResult result) {
    ++calls;

    if (result.status == LineEditStatus::COMMITTED) {
      f.binding.reset();
      take(f.tree->remove(f.parent));
    }
  };
  f.input("x\r");
  take(f.scheduler->dispatch());
  check(calls == 2 && take(f.model->snapshot())->value == "x",
        "destructive commit callback removes ancestor without later delivery");

  auto clipboard = std::make_shared<FakeClipboard>();
  Fixture removed("text", {}, clipboard);

  clipboard->reenter = [&] {
    take(removed.tree->remove(removed.node));
  };
  removed.input("\x01\x18");
  check(removed.result.status == LineEditStatus::FAILED && removed.result.error == TreeError::STALE_NODE &&
          take(removed.model->snapshot())->value == "text",
        "clipboard reentry removing node cannot mutate dead presentation");

  Fixture *owner{};
  Fixture changed("old", {}, {}, [&](std::string_view) {
    take(owner->model->set("external"));

    return true;
  });

  owner = &changed;
  changed.input("!\r");
  check(changed.result.status == LineEditStatus::CONFLICT && changed.draft() == "old!",
        "validator reentry rechecks model revision before set");
}

struct OnDestroy {
  std::function<void()> action;

  ~OnDestroy() {
    action();
  }
};

void captureReentry() {
  Fixture f;
  auto released = std::make_shared<OnDestroy>();
  bool destroyed{};

  f.binding.reset();
  take(f.router->focused());
  released->action = [&] {
    destroyed = true;
    f.binding.reset();
    take(f.tree->invalidate(f.root, Invalidation::PAINT));
  };
  f.binding.emplace(take(bindLineEdit(*f.tree, *f.router, f.node, f.model, [released](LineEditResult) {
  })));
  released.reset();

  const auto second = take(f.tree->emplace<LineEdit>(f.parent));
  auto replacement = take(bindLineEdit(*f.tree, *f.router, second, f.model, [](LineEditResult) {
  }));

  *f.binding = std::move(replacement);
  take(f.router->focused());
  check(destroyed && !f.binding, "retired callback state can destroy fully installed replacement binding");
  check(take(f.router->invoke(second, {Action::ACTIVATE})).status == DispatchStatus::UNHANDLED,
        "reentrant destruction disconnects replacement action");
}

void allocationFailures() {
  constexpr std::ptrdiff_t MAX_BUDGET = 160;
  bool succeeded{};
  bool observedFailure{};

  for (std::ptrdiff_t budget = 0; budget < MAX_BUDGET; ++budget) {
    Fixture f("original");
    InputEvent paste = PasteInputEvent{std::string(80, 'x')};

    take(f.router->setFocus(f.node));
    take(f.tree->consumeFrameRequest());
    allocation_test::remaining = budget;

    auto result = f.router->dispatch(paste);

    allocation_test::remaining = -1;

    if (result && f.result.status == LineEditStatus::EDITED) {
      succeeded = true;

      break;
    }

    observedFailure = true;
    check((!result && result.error() == TreeError::NO_MEMORY) || (result && f.result.error == TreeError::NO_MEMORY),
          "allocation failure is explicit at router or editor boundary");
    check(f.draft() == "original" && take(f.model->snapshot())->value == "original",
          "failed edit allocation preserves model and draft");
  }

  check(succeeded && observedFailure, "allocation sweep reaches failure and eventual successful edit");
}

void scalarAndCommitAllocationFailures() {
  constexpr std::ptrdiff_t MAX_BUDGET = 160;

  for (const bool commit : {false, true}) {
    bool succeeded{};
    bool observedFailure{};

    for (std::ptrdiff_t budget = 0; budget < MAX_BUDGET; ++budget) {
      Fixture f("original");

      take(f.router->setFocus(f.node));

      if (commit) {
        take(f.router->dispatch(TextInputEvent{U'x', {}}));
      }

      const InputEvent event = commit ? InputEvent{KeyEvent{InputKey::ENTER}} : InputEvent{TextInputEvent{U'x', {}}};

      take(f.tree->consumeFrameRequest());
      allocation_test::remaining = budget;

      auto result = f.router->dispatch(event);

      allocation_test::remaining = -1;

      if (result && f.result.status == (commit ? LineEditStatus::COMMITTED : LineEditStatus::EDITED)) {
        succeeded = true;
        check(f.draft() == "originalx", "successful scalar/commit keeps exact edited bytes");

        if (commit) {
          const auto snapshot = take(f.model->snapshot());

          check(snapshot->value == "originalx" && snapshot->revision == 1 && f.result.commit &&
                  f.result.commit->snapshot == snapshot,
                "successful commit reports its exact immutable revision");
        } else {
          check(take(f.model->snapshot())->value == "original", "scalar insertion changes only draft");
        }

        break;
      }

      observedFailure = true;
      check((!result && result.error() == TreeError::NO_MEMORY) || (result && f.result.error == TreeError::NO_MEMORY),
            "scalar/commit allocation failure is observable instead of terminating");
      check(f.draft() == (commit ? "originalx" : "original") && take(f.model->snapshot())->value == "original" &&
              take(f.model->snapshot())->revision == 0,
            "rejected scalar/commit preserves draft and model revision");
    }

    check(succeeded && observedFailure, "scalar/commit sweep observes failure then success");
  }
}

int nativeChild(bool injectRecords = true) {
  try {
    Fixture f;
#ifdef _WIN32
    const auto inputHandle = CreateFileW(L"CONIN$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                                         nullptr, OPEN_EXISTING, 0, nullptr);
    const auto outputHandle = CreateFileW(L"CONOUT$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                                          nullptr, OPEN_EXISTING, 0, nullptr);
    auto session = take(TerminalSession::open(
      {reinterpret_cast<std::intptr_t>(inputHandle), reinterpret_cast<std::intptr_t>(outputHandle)}));
    if (injectRecords) {
      std::vector<INPUT_RECORD> records;
      const std::wstring text = L"e\u0301\U0001f469\u200d\U0001f4bb\u754c";

      for (const auto character : text) {
        INPUT_RECORD record{};

        record.EventType = KEY_EVENT;
        record.Event.KeyEvent = {TRUE, 1, 0, 0, {character}, 0};
        records.push_back(record);
      }

      constexpr std::array<std::pair<WORD, wchar_t>, 3> COMMANDS{
        {{WORD{VK_LEFT}, L'\0'}, {WORD{VK_BACK}, L'\b'}, {WORD{VK_RETURN}, L'\r'}}};

      for (const auto [key, character] : COMMANDS) {
        INPUT_RECORD record{};

        record.EventType = KEY_EVENT;
        record.Event.KeyEvent = {TRUE, 1, key, 0, {character}, 0};
        records.push_back(record);
      }

      DWORD mode{}, written{};

      if (!GetConsoleMode(inputHandle, &mode) || !SetConsoleMode(inputHandle, mode & ~ENABLE_VIRTUAL_TERMINAL_INPUT) ||
          !WriteConsoleInputW(inputHandle, records.data(), static_cast<DWORD>(records.size()), &written) ||
          !SetConsoleMode(inputHandle, mode) || written != records.size()) {
        return 7;
      }
    }

    CloseHandle(inputHandle);
    CloseHandle(outputHandle);
#else
    (void)injectRecords;

    auto session = take(TerminalSession::open({STDIN_FILENO, STDOUT_FILENO}));
#endif
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);

    take(session.submit("READY\r\n"));

    if (session.flush(deadline).error) {
      return 2;
    }

    std::string eventTrace;

    while (f.result.status != LineEditStatus::COMMITTED && std::chrono::steady_clock::now() < deadline) {
      auto input = session.read(deadline);

      if (input.error) {
        return 3;
      }

      f.input(input.bytes);

      for (const auto &event : input.events) {
        if (const auto *text = std::get_if<TextInputEvent>(&event)) {
          eventTrace += "T" + std::to_string(static_cast<unsigned>(text->scalar)) + ":" +
                        std::to_string(text->modifiers.shift) + ";";
        }

        if (const auto *key = std::get_if<KeyEvent>(&event)) {
          eventTrace += "K" + std::to_string(static_cast<unsigned>(key->key)) + ":" + std::to_string(key->scalar) + ";";
        }

        take(f.router->dispatch(event));
      }
    }

    if (take(f.model->snapshot())->value != std::string(COMBINING) + CJK) {
      std::string diagnostic = "MISMATCH:";

      for (const auto byte : take(f.model->snapshot())->value) {
        diagnostic += std::to_string(static_cast<unsigned char>(byte)) + ",";
      }

      take(session.submit(diagnostic + " trace:" + eventTrace + "\r\n"));
      (void)session.flush(deadline);

      return 4;
    }

    TerminalFrameOutput output(session);
    FrameWriter writer(output);

    take(writer.submit(f.render({8, 1})));

    if (writer.flush(deadline).error || writer.pending()) {
      return 5;
    }

    take(session.close());

    return 0;
  } catch (...) {
    return 6;
  }
}

int nativeHost(bool pipeInput = false) {
  const auto inputBytes = std::string(COMBINING) + EMOJI + CJK + "\x1b[D\x7f\r";
#ifdef _WIN32
  struct Host {
    HANDLE inputRead{}, inputWrite{}, outputRead{}, outputWrite{}, process{}, thread{};
    HPCON console{};
    std::thread reader;
    std::atomic<bool> ready{};
    std::string output;

    ~Host() {
      if (process && WaitForSingleObject(process, 0) == WAIT_TIMEOUT) {
        TerminateProcess(process, 99);
        WaitForSingleObject(process, 2000);
      }

      if (console) {
        ClosePseudoConsole(console);
      }

      if (reader.joinable()) {
        reader.join();
      }

      for (auto handle : {inputRead, inputWrite, outputRead, outputWrite, process, thread}) {
        if (handle) {
          CloseHandle(handle);
        }
      }
    }
  } host;

  if (!CreatePipe(&host.inputRead, &host.inputWrite, nullptr, 0) ||
      !CreatePipe(&host.outputRead, &host.outputWrite, nullptr, 0) ||
      FAILED(CreatePseudoConsole({20, 4}, host.inputRead, host.outputWrite, 0, &host.console))) {
    return 10;
  }

  CloseHandle(host.inputRead);
  CloseHandle(host.outputWrite);
  host.inputRead = host.outputWrite = nullptr;

  SIZE_T size{};

  InitializeProcThreadAttributeList(nullptr, 1, 0, &size);

  std::vector<std::byte> attributes(size);
  auto *list = reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(attributes.data());

  if (!InitializeProcThreadAttributeList(list, 1, 0, &size)) {
    return 11;
  }

  const bool attached = UpdateProcThreadAttribute(list, 0, PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE, host.console,
                                                  sizeof(host.console), nullptr, nullptr) != FALSE;
  wchar_t executable[32768]{};

  GetModuleFileNameW(nullptr, executable, 32768);

  std::wstring command =
    std::wstring(L"\"") + executable + (pipeInput ? L"\" --native-child-pipe" : L"\" --native-child");
  STARTUPINFOEXW startup{};
  PROCESS_INFORMATION process{};

  startup.StartupInfo.cb = sizeof(startup);
  startup.lpAttributeList = list;

  const bool launched =
    attached && CreateProcessW(executable, command.data(), nullptr, nullptr, FALSE, EXTENDED_STARTUPINFO_PRESENT,
                               nullptr, nullptr, &startup.StartupInfo, &process);

  DeleteProcThreadAttributeList(list);

  if (!launched) {
    return 12;
  }

  host.process = process.hProcess;
  host.thread = process.hThread;
  host.reader = std::thread([&] {
    char bytes[4096];
    DWORD count{};

    while (ReadFile(host.outputRead, bytes, sizeof(bytes), &count, nullptr) && count) {
      if (host.output.size() + count <= 65536) {
        host.output.append(bytes, count);
      }

      if (host.output.contains("READY")) {
        host.ready = true;
      }
    }
  });

  const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);

  while (!host.ready && std::chrono::steady_clock::now() < deadline &&
         WaitForSingleObject(host.process, 10) == WAIT_TIMEOUT) {
  }

  DWORD written{};

  if (!host.ready ||
      (pipeInput &&
       (!WriteFile(host.inputWrite, inputBytes.data(), static_cast<DWORD>(inputBytes.size()), &written, nullptr) ||
        written != inputBytes.size())) ||
      WaitForSingleObject(host.process, 10000) != WAIT_OBJECT_0) {
    return 13;
  }

  DWORD code{};

  GetExitCodeProcess(host.process, &code);

  ClosePseudoConsole(host.console);
  host.console = nullptr;
  host.reader.join();

  if (code != 0) {
    const auto diagnostic = host.output.find("MISMATCH:");

    std::cerr << "native child exit=" << code << '\n';

    if (diagnostic != std::string::npos) {
      const auto detail = host.output.substr(diagnostic, host.output.find('\r', diagnostic) - diagnostic);

      for (const auto byte : detail) {
        if (byte >= 0x20 && byte <= 0x7e) {
          std::cerr << byte;
        }
      }

      std::cerr << '\n';
    }
  }

  return static_cast<int>(code);
#else
  (void)pipeInput;

  const int master = posix_openpt(O_RDWR | O_NOCTTY);

  if (master < 0 || grantpt(master) || unlockpt(master)) {
    return 10;
  }

  const int slave = open(ptsname(master), O_RDWR | O_NOCTTY);
  const auto child = fork();

  if (child == 0) {
    close(master);
    dup2(slave, STDIN_FILENO);
    dup2(slave, STDOUT_FILENO);
    close(slave);
    _exit(nativeChild());
  }

  close(slave);

  if (child < 0) {
    close(master);

    return 11;
  }

  bool sent{};
  bool finished{};
  int status{};
  std::string output;
  const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);

  while (std::chrono::steady_clock::now() < deadline) {
    pollfd poller{master, POLLIN, 0};

    if (poll(&poller, 1, 10) > 0 && (poller.revents & POLLIN)) {
      char bytes[4096];
      const auto count = read(master, bytes, sizeof(bytes));

      if (count > 0 && output.size() + static_cast<std::size_t>(count) <= 65536) {
        output.append(bytes, static_cast<std::size_t>(count));
      }
    }

    if (!sent && output.contains("READY")) {
      sent = write(master, inputBytes.data(), inputBytes.size()) == static_cast<ssize_t>(inputBytes.size());
    }

    if (waitpid(child, &status, WNOHANG) == child) {
      finished = true;

      break;
    }
  }

  if (!finished) {
    kill(child, SIGKILL);
    waitpid(child, &status, 0);
  }

  close(master);

  return sent && finished && WIFEXITED(status) ? WEXITSTATUS(status) : 12;
#endif
}
} // namespace

int main(int argc, char **argv) {
  if (argc == 2 && std::string_view(argv[1]) == "--native-child") {
    return nativeChild();
  }

  if (argc == 2 && std::string_view(argv[1]) == "--native-child-pipe") {
    return nativeChild(false);
  }

  if (argc == 2 && std::string_view(argv[1]) == "--native-pipe") {
    return nativeHost(true);
  }

  if (argc == 2 && std::string_view(argv[1]) == "--native") {
    return nativeHost();
  }
  graphemeEditing();
  conflictAndOverflow();
  clipboardAndValidation();
  pasteAndRendering();
  callbackLifetimes();
  captureReentry();
  allocationFailures();
  scalarAndCommitAllocationFailures();

  return failures ? 1 : 0;
}
