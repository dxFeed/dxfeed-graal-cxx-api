// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "NetProbe.hpp"

#ifdef _WIN32
#    ifndef WIN32_LEAN_AND_MEAN
#        define WIN32_LEAN_AND_MEAN
#    endif
#    ifndef NOMINMAX
#        define NOMINMAX
#    endif
#    include <winsock2.h>
#    include <ws2tcpip.h>
#else
#    include <cerrno>
#    include <fcntl.h>
#    include <netdb.h>
#    include <poll.h>
#    include <sys/socket.h>
#    include <sys/types.h>
#    include <unistd.h>
#endif

#include <string>
#include <system_error>

namespace dxfcpp::tools::diagnose {

namespace {

#ifdef _WIN32
using Socket = SOCKET;
const Socket INVALID_SOCKET_VALUE = INVALID_SOCKET;

int lastSocketError() {
    return WSAGetLastError();
}

void closeSocket(Socket socket) {
    closesocket(socket);
}

bool initSockets() {
    static const bool initialized = [] {
        WSADATA data{};

        return WSAStartup(MAKEWORD(2, 2), &data) == 0;
    }();

    return initialized;
}

std::string resolveErrorText(int code) {
    // getaddrinfo returns a Winsock error code on Windows.
    return std::system_category().message(code);
}

bool setNonBlocking(Socket socket) {
    u_long mode = 1;

    // FIONBIO is an unsigned long constant; the command parameter is a long.
    return ioctlsocket(socket, static_cast<long>(FIONBIO), &mode) == 0;
}

bool isConnectInProgress(int error) {
    return error == WSAEWOULDBLOCK;
}

// Waits for the end of a non-blocking connect: 0 on success, -1 on the timeout, otherwise the error code.
int waitConnected(Socket socket, std::chrono::milliseconds timeout) {
    // The sets are filled without FD_SET, whose do-while(0) is a warning of MSVC.
    fd_set writeSet{};
    fd_set errorSet{};
    writeSet.fd_count = 1;
    writeSet.fd_array[0] = socket;
    errorSet.fd_count = 1;
    errorSet.fd_array[0] = socket;

    timeval time{};
    time.tv_sec = static_cast<long>(timeout.count() / 1000);
    time.tv_usec = static_cast<long>((timeout.count() % 1000) * 1000);

    const int ready = select(0, nullptr, &writeSet, &errorSet, &time);

    if (ready == 0) {
        return -1;
    }

    if (ready == SOCKET_ERROR) {
        return lastSocketError();
    }

    int error = 0;
    int length = sizeof(error);

    if (getsockopt(socket, SOL_SOCKET, SO_ERROR, reinterpret_cast<char *>(&error), &length) != 0) {
        return lastSocketError();
    }

    // A failed connect is reported in the error set; SO_ERROR holds its code.
    if (error == 0 && errorSet.fd_count > 0) {
        return WSAECONNREFUSED;
    }

    return error;
}

int toAddressLength(std::size_t length) {
    return static_cast<int>(length);
}
#else
using Socket = int;
const Socket INVALID_SOCKET_VALUE = -1;

int lastSocketError() {
    return errno;
}

void closeSocket(Socket socket) {
    ::close(socket);
}

bool initSockets() {
    return true;
}

std::string resolveErrorText(int code) {
    if (code == EAI_SYSTEM) {
        return std::system_category().message(errno);
    }

    return gai_strerror(code);
}

bool setNonBlocking(Socket socket) {
    const int flags = fcntl(socket, F_GETFL, 0);

    return flags >= 0 && fcntl(socket, F_SETFL, flags | O_NONBLOCK) == 0;
}

bool isConnectInProgress(int error) {
    return error == EINPROGRESS;
}

// Waits for the end of a non-blocking connect: 0 on success, -1 on the timeout, otherwise the error code.
int waitConnected(Socket socket, std::chrono::milliseconds timeout) {
    pollfd descriptor{};
    descriptor.fd = socket;
    descriptor.events = POLLOUT;

    int ready{};

    do {
        ready = poll(&descriptor, 1, static_cast<int>(timeout.count()));
    } while (ready < 0 && errno == EINTR);

    if (ready == 0) {
        return -1;
    }

    if (ready < 0) {
        return lastSocketError();
    }

    int error = 0;
    socklen_t length = sizeof(error);

    if (getsockopt(socket, SOL_SOCKET, SO_ERROR, &error, &length) != 0) {
        return lastSocketError();
    }

    return error;
}

socklen_t toAddressLength(socklen_t length) {
    return length;
}
#endif

std::string numericAddress(const addrinfo *info) {
    char buffer[NI_MAXHOST]{};

    if (getnameinfo(info->ai_addr, toAddressLength(info->ai_addrlen), buffer, sizeof(buffer), nullptr, 0,
                    NI_NUMERICHOST) != 0) {
        return "?";
    }

    return buffer;
}

IpProbe connectTo(const addrinfo *info, std::chrono::milliseconds timeout) {
    IpProbe result{numericAddress(info)};
    const auto start = std::chrono::steady_clock::now();
    const auto elapsed = [&start] {
        return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start)
            .count();
    };

    const Socket socket = ::socket(info->ai_family, info->ai_socktype, info->ai_protocol);

    if (socket == INVALID_SOCKET_VALUE) {
        result.error = std::system_category().message(lastSocketError());

        return result;
    }

    int error = 0;

    if (!setNonBlocking(socket)) {
        error = lastSocketError();
    } else if (::connect(socket, info->ai_addr, toAddressLength(info->ai_addrlen)) != 0) {
        const int connectError = lastSocketError();

        error = isConnectInProgress(connectError) ? waitConnected(socket, timeout) : connectError;
    }

    result.millis = elapsed();

    if (error == 0) {
        result.connected = true;
    } else if (error == -1) {
        result.error = "timed out after " + std::to_string(timeout.count()) + " ms";
    } else {
        result.error = std::system_category().message(error);
    }

    closeSocket(socket);

    return result;
}

} // namespace

HostProbe probeHost(const std::string &host, std::uint16_t port, std::chrono::milliseconds timeout) {
    HostProbe result{host, port};

    if (!initSockets()) {
        result.error = "the sockets are not available";

        return result;
    }

    addrinfo hints{};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    addrinfo *list = nullptr;
    const auto service = std::to_string(port);

    if (const int code = getaddrinfo(host.c_str(), service.c_str(), &hints, &list); code != 0 || list == nullptr) {
        result.error = code != 0 ? resolveErrorText(code) : std::string("no addresses");

        if (list != nullptr) {
            freeaddrinfo(list);
        }

        return result;
    }

    result.resolved = true;

    for (const addrinfo *info = list; info != nullptr && result.ips.size() < MAX_PROBED_IPS; info = info->ai_next) {
        result.ips.push_back(connectTo(info, timeout));
    }

    freeaddrinfo(list);

    return result;
}

} // namespace dxfcpp::tools::diagnose
