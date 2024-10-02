// Copyright (c) 2023 ttldtor.
// SPDX-License-Identifier: BSL-1.0

#include <console/console.hpp>

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#    include <windows.h>

namespace org {
namespace ttldtor {
namespace console {

std::pair<std::size_t, std::size_t> Console::getSize() noexcept {
    CONSOLE_SCREEN_BUFFER_INFO consoleScreenBufferInfo{};
    std::size_t columns{}, rows{};

    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &consoleScreenBufferInfo);
    columns =
        static_cast<std::size_t>(consoleScreenBufferInfo.srWindow.Right - consoleScreenBufferInfo.srWindow.Left + 1);
    rows = static_cast<std::size_t>(consoleScreenBufferInfo.srWindow.Bottom - consoleScreenBufferInfo.srWindow.Top + 1);

    return {columns, rows};
}

} // namespace console
} // namespace ttldtor
} // namespace org

#elif defined(__CYGWIN__) || defined(__ANDROID__) || defined(__linux__) || defined(__APPLE__)
#    include <fcntl.h>     // open(), O_EVTONLY, O_NONBLOCK
#    include <sys/ioctl.h> // ioctl()
#    include <unistd.h>    // close()

namespace org {
namespace ttldtor {
namespace console {

std::pair<std::size_t, std::size_t> Console::getSize() noexcept {
#    if defined(__APPLE__)
    auto ttyFileDescriptor = open("/dev/tty", O_EVTONLY | O_NONBLOCK);
#    else
    auto ttyFileDescriptor = open("/dev/tty", O_RDWR);
#    endif

    if (ttyFileDescriptor == -1) {
        return {80, 25};
    }

    winsize winSize{};

    auto result = ioctl(ttyFileDescriptor, TIOCGWINSZ, &winSize);

    close(ttyFileDescriptor);

    if (result == -1) {
        return {80, 25};
    }

    return {static_cast<std::size_t>(winSize.ws_col), static_cast<std::size_t>(winSize.ws_row)};
}

} // namespace console
} // namespace ttldtor
} // namespace org

#else

namespace org {
namespace ttldtor {
namespace console {

std::pair<std::size_t, std::size_t> Console::getSize() noexcept {
    return {80, 25};
}

} // namespace console
} // namespace ttldtor
} // namespace org

#endif