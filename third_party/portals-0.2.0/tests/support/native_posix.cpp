// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0
#include "native_cleanup.hpp"
#include <array>
#include <charconv>
#include <chrono>
#include <cstdlib>
#include <fcntl.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <termios.h>
#include <thread>
#include <unistd.h>
#ifdef PORTALS_NATIVE_X11
#  include <X11/Xlib.h>
#  include <X11/Xutil.h>
#  include <X11/extensions/XTest.h>
#  include <X11/keysym.h>
#endif

namespace native {
namespace {
struct Snapshot {
  termios modes{};
  std::array<int, 3> flags{};
  bool valid{};
};

Snapshot snapshot(std::ostream &log, const char *phase) {
  Snapshot value;
  value.valid = tcgetattr(STDIN_FILENO, &value.modes) == 0;

  for (int fd = 0; fd < 3; ++fd) {
    value.flags[fd] = fcntl(fd, F_GETFL);
    value.valid = value.valid && value.flags[fd] >= 0;
  }

  log << "phase=" << phase << " valid=" << value.valid << " iflag=" << value.modes.c_iflag
      << " oflag=" << value.modes.c_oflag << " cflag=" << value.modes.c_cflag << " lflag=" << value.modes.c_lflag
      << " flags=" << value.flags[0] << ',' << value.flags[1] << ',' << value.flags[2] << '\n';

  return value;
}

bool restored(const Snapshot &before, const Snapshot &after) {
  const auto &a = before.modes;
  const auto &b = after.modes;

  return before.valid && after.valid && before.flags == after.flags && a.c_iflag == b.c_iflag &&
         a.c_oflag == b.c_oflag && a.c_cflag == b.c_cflag && a.c_lflag == b.c_lflag &&
#ifdef __linux__
         a.c_line == b.c_line &&
#endif
         cfgetispeed(&a) == cfgetispeed(&b) && cfgetospeed(&a) == cfgetospeed(&b) &&
         std::equal(std::begin(a.c_cc), std::end(a.c_cc), std::begin(b.c_cc));
}

class Driver {
public:
  explicit Driver(const Options &options) {
#ifdef PORTALS_NATIVE_X11
    if (!options.drive) {
      return;
    }

    // XTest targets the whole server. Admit only the launcher's private display,
    // before opening any connection or changing focus, mapping, or pointer state.
    const auto *isolatedDisplay = std::getenv("PORTALS_ISOLATED_X11_DISPLAY");
    const auto *displayName = std::getenv("DISPLAY");
    const auto *isolatedTitle = std::getenv("PORTALS_ISOLATED_X11_TITLE");
    const auto *authority = std::getenv("XAUTHORITY");

    if (!isolatedDisplay || !displayName || !isolatedTitle || !authority || !*authority ||
        std::string_view(displayName) != isolatedDisplay || options.title.empty() || options.title != isolatedTitle) {
      return;
    }

    struct stat authorityFile{};

    if (lstat(authority, &authorityFile) != 0 || !S_ISREG(authorityFile.st_mode) || authorityFile.st_uid != geteuid() ||
        (authorityFile.st_mode & 077) != 0 || authorityFile.st_size == 0) {
      return;
    }

    constexpr unsigned MAX_DISPLAY_NUMBER = 65535;
    const std::string_view name(displayName);
    unsigned displayNumber{};

    if (name.size() < 2 || name.front() != ':') {
      return;
    }

    const auto parsed = std::from_chars(name.data() + 1, name.data() + name.size(), displayNumber);

    if (parsed.ec != std::errc{} || parsed.ptr != name.data() + name.size() || displayNumber == 0 ||
        displayNumber > MAX_DISPLAY_NUMBER) {
      return;
    }

    const auto *id = std::getenv("WINDOWID");

    if (!id) {
      return;
    }

    const std::string_view windowId(id);
    const auto windowParsed = std::from_chars(windowId.data(), windowId.data() + windowId.size(), window_);

    if (windowParsed.ec != std::errc{} || windowParsed.ptr != windowId.data() + windowId.size() || !window_) {
      return;
    }

    display_ = XOpenDisplay(nullptr);

    if (!display_) {
      return;
    }

    char *title{};
    const bool titleFetched = XFetchName(display_, window_, &title);
    const bool ownWindow = titleFetched && title && options.title == title;

    if (title) {
      XFree(title);
    }

    int eventBase{}, errorBase{}, majorVersion{}, minorVersion{};

    if (!ownWindow || !XTestQueryExtension(display_, &eventBase, &errorBase, &majorVersion, &minorVersion)) {
      return;
    }

    XMapWindow(display_, window_);
    XSetInputFocus(display_, window_, RevertToParent, CurrentTime);
    XSync(display_, False);
    valid_ = true;
#else
    (void)options;
#endif
  }

  ~Driver() {
#ifdef PORTALS_NATIVE_X11
    if (display_) {
      XCloseDisplay(display_);
    }
#endif
  }

  bool ready() const {
#ifdef PORTALS_NATIVE_X11
    return valid_;
#else
    return false;
#endif
  }

  bool send(int stage, std::ostream &log) {
    bool passed = false;
#ifdef PORTALS_NATIVE_X11
    if (!valid_) {
      return false;
    }

    if (stage == 0) {
      passed = key(XK_F2);
    } else if (stage == 1) {
      passed = key(XK_Tab) && key(XK_a, ControlMask);

      for (const char character : std::string_view("Native note")) {
        passed = key(static_cast<KeySym>(character), character == 'N' ? ShiftMask : 0) && passed;
      }

      // This child xterm alone maps F8 to the UTF-8 suffix. The server keymap
      // and the user's desktop clipboard remain untouched.
      passed = key(XK_F8) && key(XK_Return) && passed;
    } else if (stage == 2) {
      passed = key(XK_Tab) && key(XK_Tab) && key(XK_Down) && key(XK_Return);
    } else if (stage == 3) {
      passed = click();
    } else if (stage == 4 || stage == 5) {
      XSizeHints hints{};
      long supplied{};
      passed = XGetWMNormalHints(display_, window_, &hints, &supplied);
      const int columns = stage == 4 ? 100 : 120;
      const int rows = stage == 4 ? 32 : 40;

      if (passed && hints.width_inc > 0 && hints.height_inc > 0) {
        XResizeWindow(display_, window_, static_cast<unsigned>(hints.base_width + columns * hints.width_inc),
                      static_cast<unsigned>(hints.base_height + rows * hints.height_inc));
      } else {
        passed = false;
      }
    } else {
      passed = key(XK_F10);
    }

    XSync(display_, False);
#endif
    log << "driver_stage=" << stage << " passed=" << passed << '\n';
    log.flush();

    return passed;
  }

private:
#ifdef PORTALS_NATIVE_X11
  bool click() {
    constexpr int CLICK_COLUMN = 8;
    constexpr int CLICK_ROW = 7;
    XSizeHints hints{};
    long supplied{};

    if (!XGetWMNormalHints(display_, window_, &hints, &supplied) || hints.width_inc <= 0 || hints.height_inc <= 0) {
      return false;
    }

    int rootX{}, rootY{};
    Window child{};

    if (!XTranslateCoordinates(display_, window_, DefaultRootWindow(display_),
                               hints.base_width + hints.width_inc * CLICK_COLUMN,
                               hints.base_height + hints.height_inc * CLICK_ROW, &rootX, &rootY, &child)) {
      return false;
    }

    bool passed = XTestFakeMotionEvent(display_, DefaultScreen(display_), rootX, rootY, CurrentTime);
    passed = XTestFakeButtonEvent(display_, Button1, True, CurrentTime) && passed;
    passed = XTestFakeButtonEvent(display_, Button1, False, CurrentTime) && passed;

    return passed;
  }

  bool key(KeySym symbol, unsigned modifiers = 0) {
    const auto code = XKeysymToKeycode(display_, symbol);
    const auto shift = XKeysymToKeycode(display_, XK_Shift_L);
    const auto control = XKeysymToKeycode(display_, XK_Control_L);

    if (!code || ((modifiers & ShiftMask) && !shift) || ((modifiers & ControlMask) && !control)) {
      return false;
    }

    bool passed = true;

    if (modifiers & ShiftMask) {
      passed = XTestFakeKeyEvent(display_, shift, True, CurrentTime) && passed;
    }

    if (modifiers & ControlMask) {
      passed = XTestFakeKeyEvent(display_, control, True, CurrentTime) && passed;
    }

    passed = XTestFakeKeyEvent(display_, code, True, CurrentTime) && passed;
    passed = XTestFakeKeyEvent(display_, code, False, CurrentTime) && passed;

    if (modifiers & ControlMask) {
      passed = XTestFakeKeyEvent(display_, control, False, CurrentTime) && passed;
    }

    if (modifiers & ShiftMask) {
      passed = XTestFakeKeyEvent(display_, shift, False, CurrentTime) && passed;
    }

    return passed;
  }

  Display *display_{};
  Window window_{};
  bool valid_{};
#endif
};
} // namespace

bool supervise(const Options &options, std::ostream &log) {
  const auto before = snapshot(log, "before");
  winsize viewport{};
  ioctl(STDIN_FILENO, TIOCGWINSZ, &viewport);
  log << "harness_pid=" << getpid() << " terminal_parent_pid=" << getppid() << " initial_viewport=" << viewport.ws_col
      << 'x' << viewport.ws_row << '\n';

  for (const auto *name : {"DISPLAY", "TERM", "WINDOWID"}) {
    const auto *value = std::getenv(name);
    log << name << '=' << (value ? value : "") << '\n';
  }

  if (!before.valid) {
    return false;
  }

  Driver driver(options);

  if (options.drive && !driver.ready()) {
    log << "driver_ready=0 input_not_sent=1\n";

    return false;
  }

  std::vector<char *> arguments;

  for (const auto &argument : options.child) {
    arguments.push_back(const_cast<char *>(argument.c_str()));
  }

  arguments.push_back(nullptr);
  const pid_t child = fork();

  if (child == 0) {
    execvp(arguments[0], arguments.data());
    _exit(127);
  }

  if (child < 0) {
    return false;
  }

  log << "child_pid=" << child << '\n';
  log.flush();
  const auto start = std::chrono::steady_clock::now();
  const std::array<int, 7> schedule{1000, 1300, 1800, 2200, 2600, 3200, options.quitMs};
  std::size_t stage{};
  bool driverPassed = true;
  int status{};
  pid_t waited{};
  winsize previousSize = viewport;

  while (std::chrono::steady_clock::now() - start < std::chrono::milliseconds(options.deadlineMs)) {
    waited = waitpid(child, &status, WNOHANG);

    if (waited != 0) {
      break;
    }

    if (options.drive && stage < schedule.size() &&
        std::chrono::steady_clock::now() - start >= std::chrono::milliseconds(schedule[stage])) {
      driverPassed = driver.send(static_cast<int>(stage++), log) && driverPassed;
    }

    if (ioctl(STDIN_FILENO, TIOCGWINSZ, &viewport) == 0 &&
        (viewport.ws_col != previousSize.ws_col || viewport.ws_row != previousSize.ws_row)) {
      log << "observed_viewport=" << viewport.ws_col << 'x' << viewport.ws_row << '\n';
      previousSize = viewport;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(20));
  }

  const bool watchdog = waited != child;

  if (watchdog) {
    // Keep the child in xterm's foreground group so native reads are permitted.
    // The fixture owns this exact PID; it never signals the surrounding group.
    kill(child, SIGKILL);
    waited = waitpid(child, &status, 0);
  }

  const int code = WIFEXITED(status) ? WEXITSTATUS(status) : 128 + WTERMSIG(status);
  const auto after = snapshot(log, "after");
  const bool exact = restored(before, after);
  log << "child_exit=" << code << " watchdog=" << watchdog << " exact_restore=" << exact
      << " own_child_gone=" << (waited == child) << '\n';

  return code == options.expectedExit && !watchdog && exact && driverPassed &&
         (!options.drive || stage == schedule.size());
}
} // namespace native
