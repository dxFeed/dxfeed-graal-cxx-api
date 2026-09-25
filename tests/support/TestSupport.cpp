// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "TestSupport.hpp"

#include <stdexcept>

#ifdef _WIN32
#    ifndef WIN32_LEAN_AND_MEAN
#        define WIN32_LEAN_AND_MEAN
#    endif
#    include <winsock2.h>
#    include <ws2tcpip.h>
#else
#    include <arpa/inet.h>
#    include <netinet/in.h>
#    include <sys/select.h>
#    include <sys/socket.h>
#    include <unistd.h>
#endif

namespace dxfcpp::test {

namespace {

#ifdef _WIN32
using NativeSocket = SOCKET;
constexpr NativeSocket INVALID_NATIVE_SOCKET = INVALID_SOCKET;

void closeNativeSocket(NativeSocket s) {
    closesocket(s);
}

struct SocketLibrary {
    SocketLibrary() {
        WSADATA wsaData{};

        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            throw std::runtime_error("WSAStartup failed");
        }
    }

    ~SocketLibrary() {
        WSACleanup();
    }
};
#else
using NativeSocket = int;
constexpr NativeSocket INVALID_NATIVE_SOCKET = -1;

void closeNativeSocket(NativeSocket s) {
    close(s);
}

struct SocketLibrary {};
#endif

SocketLibrary &socketLibrary() {
    static SocketLibrary library{};

    return library;
}

NativeSocket toNative(std::intptr_t s) {
    return static_cast<NativeSocket>(s);
}

} // namespace

LoopbackTcpServer::LoopbackTcpServer() {
    socketLibrary();

    const NativeSocket s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (s == INVALID_NATIVE_SOCKET) {
        throw std::runtime_error("LoopbackTcpServer: socket() failed");
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    address.sin_port = 0; // the OS picks a free port

    socklen_t length = sizeof(address);

    if (bind(s, reinterpret_cast<sockaddr *>(&address), sizeof(address)) != 0 || listen(s, 16) != 0 ||
        getsockname(s, reinterpret_cast<sockaddr *>(&address), &length) != 0) {
        closeNativeSocket(s);

        throw std::runtime_error("LoopbackTcpServer: bind() or listen() failed");
    }

    listenSocket_ = static_cast<std::intptr_t>(s);
    port_ = ntohs(address.sin_port);
    acceptThread_ = std::thread([this] {
        const auto listenSocket = toNative(listenSocket_);

        while (true) {
            {
                std::lock_guard lock{mutex_};

                if (isStopped_) {
                    return;
                }
            }

            // Wait with a timeout, so that the destructor does not depend on how a platform interrupts accept().
            fd_set readSet{};
            FD_ZERO(&readSet);
            FD_SET(listenSocket, &readSet);
            timeval timeout{0, 20'000};

            if (select(static_cast<int>(listenSocket) + 1, &readSet, nullptr, nullptr, &timeout) <= 0) {
                continue;
            }

            const NativeSocket client = accept(listenSocket, nullptr, nullptr);

            if (client == INVALID_NATIVE_SOCKET) {
                continue;
            }

            std::lock_guard lock{mutex_};
            clientSockets_.push_back(static_cast<std::intptr_t>(client));
        }
    });
}

LoopbackTcpServer::~LoopbackTcpServer() {
    {
        std::lock_guard lock{mutex_};
        isStopped_ = true;
    }

    acceptThread_.join();
    closeNativeSocket(toNative(listenSocket_));

    std::lock_guard lock{mutex_};

    for (const auto client : clientSockets_) {
        closeNativeSocket(toNative(client));
    }
}

std::size_t LoopbackTcpServer::getAcceptedConnectionCount() const {
    std::lock_guard lock{mutex_};

    return clientSockets_.size();
}

std::string LoopbackTcpServer::getAddress() const {
    return "127.0.0.1:" + std::to_string(port_);
}

} // namespace dxfcpp::test
