// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

// The text functions of the Diagnose tool that need no network: the hosts and ports of an address, the hiding of
// logins, passwords and tokens, and the verdict over the summary of a run.

#include "../../../../include/dxfeed_graal_cpp_api/api/ConnectionError.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace dxfcpp::tools::diagnose {

/// A host and port of an address that the network probe checks, or the reason why the part is not probed.
struct ProbeTarget {
    /// The part of the address as written (with its properties), for the output.
    std::string part{};
    std::string host{};
    std::uint16_t port{};
    /// Empty when the part is a host and port; otherwise the reason why the probe skips it.
    std::string skipped{};
};

namespace detail {

inline std::string trimmed(std::string_view text) {
    const auto isSpace = [](char c) {
        return std::isspace(static_cast<unsigned char>(c)) != 0;
    };

    while (!text.empty() && isSpace(text.front())) {
        text.remove_prefix(1);
    }

    while (!text.empty() && isSpace(text.back())) {
        text.remove_suffix(1);
    }

    return std::string(text);
}

inline std::string lowered(std::string_view text) {
    std::string result(text);

    std::transform(result.begin(), result.end(), result.begin(), [](char c) {
        return static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    });

    return result;
}

// Splits the text by the separator outside of (...) and [...].
inline std::vector<std::string> splitTopLevel(std::string_view text, char separator) {
    std::vector<std::string> result{};
    std::size_t depth = 0;
    std::size_t start = 0;

    for (std::size_t i = 0; i < text.size(); i++) {
        const char c = text[i];

        if (c == '(' || c == '[') {
            depth++;
        } else if ((c == ')' || c == ']') && depth > 0) {
            depth--;
        } else if (c == separator && depth == 0) {
            result.emplace_back(text.substr(start, i - start));
            start = i + 1;
        }
    }

    result.emplace_back(text.substr(start));

    return result;
}

// The position of the last separator outside of (...) and [...], or npos.
inline std::size_t findLastTopLevel(std::string_view text, char separator) {
    std::size_t depth = 0;
    std::size_t found = std::string_view::npos;

    for (std::size_t i = 0; i < text.size(); i++) {
        const char c = text[i];

        if (c == '(' || c == '[') {
            depth++;
        } else if ((c == ')' || c == ']') && depth > 0) {
            depth--;
        } else if (c == separator && depth == 0) {
            found = i;
        }
    }

    return found;
}

inline void addTarget(std::vector<ProbeTarget> &targets, const std::string &part, std::string_view text);

// Adds the targets of one part of an address: a group "(a,b)[properties]" or a single address.
inline void addPart(std::vector<ProbeTarget> &targets, const std::string &part) {
    const auto text = trimmed(part);

    if (text.empty()) {
        return;
    }

    if (text.front() == '(') {
        std::size_t depth = 0;
        std::size_t close = std::string::npos;

        for (std::size_t i = 0; i < text.size(); i++) {
            if (text[i] == '(' || text[i] == '[') {
                depth++;
            } else if ((text[i] == ')' || text[i] == ']') && depth > 0) {
                if (--depth == 0 && text[i] == ')') {
                    close = i;

                    break;
                }
            }
        }

        if (close != std::string::npos) {
            // The properties after the group belong to each of its addresses: the group is split again.
            for (const auto &inner : splitTopLevel(std::string_view(text).substr(1, close - 1), ',')) {
                addPart(targets, inner);
            }

            return;
        }
    }

    addTarget(targets, text, text);
}

inline void addTarget(std::vector<ProbeTarget> &targets, const std::string &part, std::string_view text) {
    ProbeTarget target{part};
    std::string address{};
    bool ipv6 = false;

    // The property blocks are dropped; a block without "=" is an IPv6 literal such as [::1].
    for (std::size_t i = 0; i < text.size(); i++) {
        if (text[i] == '[') {
            const auto close = text.find(']', i);

            if (close == std::string_view::npos) {
                break;
            }

            if (text.substr(i, close - i).find('=') == std::string_view::npos) {
                ipv6 = true;
            }

            i = close;

            continue;
        }

        address += text[i];
    }

    if (ipv6) {
        target.skipped = "an IPv6 literal is not probed";
        targets.push_back(std::move(target));

        return;
    }

    if (address.find("://") != std::string::npos) {
        target.skipped = "not a host:port socket address";
        targets.push_back(std::move(target));

        return;
    }

    // The codecs before the last "+" (tls+, ssl+, gzip+) and the "user:password@" before the host are dropped.
    if (const auto plus = findLastTopLevel(address, '+'); plus != std::string::npos) {
        address = address.substr(plus + 1);
    }

    if (const auto at = address.rfind('@'); at != std::string::npos) {
        address = address.substr(at + 1);
    }

    address = trimmed(address);

    const auto colon = address.rfind(':');

    if (colon == std::string::npos) {
        target.skipped = "no port";
    } else if (colon == 0) {
        target.skipped = "a server address (no host)";
    } else {
        const auto host = address.substr(0, colon);
        const auto port = address.substr(colon + 1);

        if (host.find(':') != std::string::npos || host.find('/') != std::string::npos) {
            target.skipped = "not a host:port socket address";
        } else if (port.find('-') != std::string::npos) {
            target.skipped = "a range of ports";
        } else if (port.empty() || port.size() > 5 || !std::all_of(port.begin(), port.end(), [](char c) {
                       return std::isdigit(static_cast<unsigned char>(c)) != 0;
                   })) {
            target.skipped = "not a host:port socket address";
        } else if (const auto value = std::stoul(port); value == 0 || value > 65535) {
            target.skipped = "the port is out of range";
        } else {
            target.host = host;
            target.port = static_cast<std::uint16_t>(value);
        }
    }

    targets.push_back(std::move(target));
}

inline bool endsWith(std::string_view text, std::string_view suffix) {
    return text.size() >= suffix.size() && text.substr(text.size() - suffix.size()) == suffix;
}

inline bool isSecretKey(std::string_view key) {
    static constexpr std::array<std::string_view, 5> SECRET_NAMES{"login", "user", "username", "auth",
                                                                  "authorization"};
    // Any key that ends so: "keyStorePassword", "authToken", "apiKey", "clientSecret".
    static constexpr std::array<std::string_view, 7> SECRET_SUFFIXES{"password", "passwd",     "pwd",       "token",
                                                                     "secret",   "credentials", "apikey"};

    // The last segment of a dotted key: "dxfeed.password" is a password too.
    if (const auto dot = key.rfind('.'); dot != std::string_view::npos) {
        key = key.substr(dot + 1);
    }

    const auto lowerKey = lowered(key);

    return std::find(SECRET_NAMES.begin(), SECRET_NAMES.end(), lowerKey) != SECRET_NAMES.end() ||
           std::any_of(SECRET_SUFFIXES.begin(), SECRET_SUFFIXES.end(),
                       [&lowerKey](std::string_view suffix) { return endsWith(lowerKey, suffix); });
}

inline bool isKeyChar(char c) {
    return std::isalnum(static_cast<unsigned char>(c)) != 0 || c == '.' || c == '_' || c == '-';
}

inline bool isQuote(char c) {
    return c == '"' || c == '\'';
}

/// The end of a value that is not quoted: the next property, the end of a property block, a space or a quote.
inline bool isValueEnd(char c) {
    return c == ',' || c == ']' || isQuote(c) || std::isspace(static_cast<unsigned char>(c)) != 0;
}

/// The characters that cannot be inside "user:password@": the text around an address.
inline bool isUserInfoBoundary(char c) {
    return c == '(' || c == ')' || c == ',' || c == '=' || c == '[' || c == ']' || isQuote(c) ||
           std::isspace(static_cast<unsigned char>(c)) != 0;
}

/**
 * Returns the start of "user:password" in the fragment of text before "@", or npos when it has no password. The
 * fragment is the text after a boundary; it may start with a scheme ("wss://") or with codecs ("tls+"). The user name
 * has no ":", "/" and "+"; the password may have any of them.
 */
inline std::size_t findUserInfoStart(std::string_view fragment) {
    std::size_t start = 0;

    if (const auto scheme = fragment.rfind("//"); scheme != std::string_view::npos) {
        start = scheme + 2;
    }

    const auto colon = fragment.find(':', start);

    if (colon == std::string_view::npos) {
        return std::string_view::npos;
    }

    if (const auto codec = fragment.substr(0, colon).find_last_of("+/"); codec != std::string_view::npos &&
                                                                          codec + 1 > start) {
        start = codec + 1;
    }

    return start;
}

} // namespace detail

/**
 * Returns the hosts and ports of a QD address that the network probe checks. The address is split by the commas
 * outside of (...) and [...]; a group "(a,b)" is split again; the property blocks [...], the codecs before the last
 * "+" (tls+, ssl[...]+, gzip+) and "user:password@" are dropped. A part that is not "host:port" (a server address
 * ":7300", "tape:", "file:", "dxlink:wss://...", a range of ports, an IPv6 literal) is returned with the reason why
 * it is skipped.
 */
inline std::vector<ProbeTarget> splitAddress(std::string_view address) {
    std::vector<ProbeTarget> targets{};

    for (const auto &part : detail::splitTopLevel(address, ',')) {
        detail::addPart(targets, part);
    }

    return targets;
}

/**
 * Hides the logins, passwords and tokens in the text: the value after "=" of a key login, user, username, auth,
 * authorization or of a key that ends with password, passwd, pwd, token, secret, credentials or apikey (with any prefix
 * before a dot, any case; a quoted value up to its closing quote) becomes "***", and "user:password@" before a host
 * becomes "***@" (the password may have ":", "/", "+" and "@").
 */
inline std::string redact(std::string_view text) {
    std::string result{};
    result.reserve(text.size());

    for (std::size_t i = 0; i < text.size(); i++) {
        const char c = text[i];

        if (c == '=') {
            std::size_t keyStart = i;

            while (keyStart > 0 && detail::isKeyChar(text[keyStart - 1])) {
                keyStart--;
            }

            if (keyStart < i && detail::isSecretKey(text.substr(keyStart, i - keyStart))) {
                result += "=***";

                if (i + 1 < text.size() && detail::isQuote(text[i + 1])) {
                    // A quoted value ends with the same quote (or with the text).
                    const auto closing = text.find(text[i + 1], i + 2);

                    i = closing == std::string_view::npos ? text.size() - 1 : closing;
                } else {
                    while (i + 1 < text.size() && !detail::isValueEnd(text[i + 1])) {
                        i++;
                    }
                }

                continue;
            }
        } else if (c == '@') {
            // "user:password@host": the fragment back to a boundary, which the result holds as is, has a password.
            std::size_t fragmentStart = i;

            while (fragmentStart > 0 && !detail::isUserInfoBoundary(text[fragmentStart - 1])) {
                fragmentStart--;
            }

            const auto fragment = text.substr(fragmentStart, i - fragmentStart);

            if (const auto infoStart = detail::findUserInfoStart(fragment); infoStart != std::string_view::npos) {
                // The password may have "@" too: the user information ends with the last "@" before a boundary.
                std::size_t lastAt = i;

                for (std::size_t j = i + 1; j < text.size() && !detail::isUserInfoBoundary(text[j]); j++) {
                    if (text[j] == '@') {
                        lastAt = j;
                    }
                }

                result.erase(result.size() - (fragment.size() - infoStart));
                result += "***@";
                i = lastAt;

                continue;
            }
        }

        result += c;
    }

    return result;
}

/// The verdict of a run, from the best to the worst.
enum class VerdictKind : std::int32_t {
    DATA,
    PARTIAL_DATA,
    CONNECTED_NO_DATA,
    CANNOT_CONNECT,
    CANNOT_RESOLVE,
};

inline const char *verdictToString(VerdictKind kind) noexcept {
    switch (kind) {
    case VerdictKind::DATA:
        return "DATA";
    case VerdictKind::PARTIAL_DATA:
        return "PARTIAL DATA";
    case VerdictKind::CONNECTED_NO_DATA:
        return "CONNECTED, NO DATA";
    case VerdictKind::CANNOT_CONNECT:
        return "CANNOT CONNECT";
    case VerdictKind::CANNOT_RESOLVE:
        return "CANNOT RESOLVE";
    }

    return "UNKNOWN";
}

/// The exit code of the tool for a verdict: 0 for data, 2..5 for the problems (1 is a wrong argument or an error).
inline int exitCode(VerdictKind kind) noexcept {
    switch (kind) {
    case VerdictKind::DATA:
        return 0;
    case VerdictKind::PARTIAL_DATA:
        return 2;
    case VerdictKind::CONNECTED_NO_DATA:
        return 3;
    case VerdictKind::CANNOT_CONNECT:
        return 4;
    case VerdictKind::CANNOT_RESOLVE:
        return 5;
    }

    return 1;
}

/// What a run of one address found: the input of the verdict.
struct RunSummary {
    /// The requested event types (their names).
    std::vector<std::string> types{};
    /// The requested symbols; "*" stands for any symbol.
    std::vector<std::string> symbols{};
    /// The pairs (event type, symbol) that received events.
    std::set<std::pair<std::string, std::string>> received{};
    /// The endpoint was in the CONNECTED state during the run.
    bool wasConnected{};
    /// The last connection error of the endpoint.
    std::optional<ConnectionError> lastError{};
    /// The probe had host names and resolved none of them.
    bool noHostResolved{};
};

struct Verdict {
    VerdictKind kind{VerdictKind::CANNOT_CONNECT};
    std::string hint{};
    /// The pairs "Type Symbol" without events (for PARTIAL DATA).
    std::vector<std::string> missing{};
};

/// The verdict over the summary of a run.
inline Verdict computeVerdict(const RunSummary &summary) {
    Verdict verdict{};
    std::size_t present = 0;

    for (const auto &type : summary.types) {
        for (const auto &symbol : summary.symbols) {
            const bool hasData =
                symbol == "*"
                    ? std::any_of(summary.received.begin(), summary.received.end(),
                                  [&type](const auto &pair) {
                                      return pair.first == type;
                                  })
                    : summary.received.count({type, symbol}) > 0;

            if (hasData) {
                present++;
            } else {
                verdict.missing.push_back(type + " " + symbol);
            }
        }
    }

    if (present > 0 && verdict.missing.empty()) {
        verdict.kind = VerdictKind::DATA;

        return verdict;
    }

    if (present > 0) {
        verdict.kind = VerdictKind::PARTIAL_DATA;
        verdict.hint = "No events for some symbols: check the spelling of the symbols and the entitlements for them; "
                       "a symbol that does not trade now is silent too.";

        return verdict;
    }

    verdict.missing.clear();

    if (summary.wasConnected) {
        verdict.kind = VerdictKind::CONNECTED_NO_DATA;
        verdict.hint = "The endpoint connected but no events came: likely not entitled or not in the endpoint filter; "
                       "check the endpoint IPF. A symbol that does not trade now is silent too.";

        // A connection that broke after the login (a rejected login, a dropped link) is a likelier cause.
        if (summary.lastError) {
            verdict.hint += " Last connection error: " + summary.lastError->toString();
        }

        return verdict;
    }

    if (summary.lastError && summary.lastError->category == ConnectionError::Category::DNS) {
        verdict.kind = VerdictKind::CANNOT_RESOLVE;
        verdict.hint = "The host name does not resolve: check the address and the DNS of this machine. Last error: " +
                       summary.lastError->toString();

        return verdict;
    }

    if (!summary.lastError && summary.noHostResolved) {
        verdict.kind = VerdictKind::CANNOT_RESOLVE;
        verdict.hint = "The host name does not resolve: check the address and the DNS of this machine.";

        return verdict;
    }

    verdict.kind = VerdictKind::CANNOT_CONNECT;
    verdict.hint = summary.lastError ? "Last connection error: " + summary.lastError->toString()
                                     : std::string("The endpoint did not connect and reported no connection error.");

    return verdict;
}

} // namespace dxfcpp::tools::diagnose
