// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0

#include <portals/terminal.hpp>

#include <utility>

namespace portals {
struct TerminalSession::Impl {};

namespace {
TerminalError unsupported() {
  return {TerminalErrorKind::UNSUPPORTED, 0, 0};
}
} // namespace

TerminalSession::TerminalSession(std::unique_ptr<Impl> impl) noexcept : impl_(std::move(impl)) {
}

TerminalSession::TerminalSession(TerminalSession &&) noexcept = default;
TerminalSession &TerminalSession::operator=(TerminalSession &&) noexcept = default;
TerminalSession::~TerminalSession() noexcept = default;

std::expected<TerminalSession, TerminalError> TerminalSession::open(NativeTerminalHandles, TerminalOptions) {
  return std::unexpected(unsupported());
}

TerminalRead TerminalSession::read(TerminalDeadline, std::stop_token) {
  return {TerminalStatus::FAILED, {}, {}, {}, unsupported()};
}

std::expected<CellSize, TerminalError> TerminalSession::size() const noexcept {
  return std::unexpected(unsupported());
}

std::expected<void, TerminalError> TerminalSession::submit(std::string) {
  return std::unexpected(unsupported());
}

TerminalWrite TerminalSession::flush(TerminalDeadline, std::stop_token) {
  return {TerminalStatus::FAILED, 0, 0, unsupported()};
}

std::size_t TerminalSession::pendingBytes() const noexcept {
  return 0;
}

std::expected<void, TerminalError> TerminalSession::joinOutputWriter() noexcept {
  return {};
}

std::expected<void, TerminalError> TerminalSession::close() noexcept {
  impl_.reset();

  return {};
}
} // namespace portals
