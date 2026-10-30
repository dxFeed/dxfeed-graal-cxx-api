// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

// The arguments of the Graal isolate: they are given before the isolate is created, so each scenario is a separate
// process. A scenario that fails returns a non-zero code and writes the reason to stderr. Usage: <executable> <scenario>.

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include <atomic>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#ifndef _WIN32
#    include <signal.h>
#endif

using namespace dxfcpp;
using namespace std::literals;

namespace {

void check(bool condition, const std::string &message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

// An argument of the code reaches the isolate.
void argsVisible() {
    check(System::setIsolateArguments({"-Dmdapi428.p=cxx"}), "the setter returned false before the creation");
    check(!Isolate::isCreated(), "the setter created the isolate");
    check(System::getProperty("mdapi428.p") == "cxx",
          "the property is '" + System::getProperty("mdapi428.p") + "' instead of 'cxx'");
}

// The second call replaces the first.
void setterTwice() {
    check(System::setIsolateArguments({"-Dmdapi428.p=1"}), "the first call returned false");
    check(System::setIsolateArguments({"-Dmdapi428.q=2"}), "the second call returned false");
    check(System::getProperty("mdapi428.p").empty(), "the property of the first call is still set");
    check(System::getProperty("mdapi428.q") == "2", "the property of the second call is not set");
}

// An empty vector clears the arguments.
void setterEmpty() {
    check(System::setIsolateArguments({"-Dmdapi428.p=1"}), "the first call returned false");
    check(System::setIsolateArguments({}), "the empty call returned false");
    check(System::getProperty("mdapi428.p").empty(), "the cleared property is set");
}

// After the creation the setter returns false and changes nothing.
void setterAfterCreation() {
    check(System::getProperty("mdapi428.warm").empty(), "unexpected property");
    check(Isolate::isCreated(), "the first call did not create the isolate");
    check(!System::setIsolateArguments({"-Dmdapi428.p=late"}), "the setter returned true after the creation");
    check(System::getProperty("mdapi428.p").empty(), "the argument given after the creation is applied");
}

// A string with a NUL is rejected; the stored arguments stay.
void setterNul() {
    check(System::setIsolateArguments({"-Dmdapi428.p=kept"}), "the first call returned false");

    bool thrown = false;

    try {
        System::setIsolateArguments({"-Dmdapi428.q=1", "-Dmdapi428.r=a\0b"s});
    } catch (const InvalidArgumentException &e) {
        thrown = true;

        check(std::string_view{e.what()}.find("-Dmdapi428.r") == std::string_view::npos,
              "the message contains the value of the argument");
    }

    check(thrown, "no InvalidArgumentException");
    check(!Isolate::isCreated(), "the setter created the isolate");
    check(System::getProperty("mdapi428.p") == "kept", "the stored argument was changed");
    check(System::getProperty("mdapi428.q").empty(), "the arguments of the rejected call are applied");
}

// The setters race with the creation of the isolate: each returns true or false, and the isolate gets the arguments
// of one of the calls that returned true, or none if all returned false.
void setterConcurrent() {
    constexpr std::size_t threadCount = 8;

    std::atomic<bool> go{false};
    std::atomic<std::size_t> ready{0};
    std::vector<int> results(threadCount, -1);
    std::vector<std::string> errors(threadCount);
    std::vector<std::thread> threads;

    for (std::size_t i = 0; i < threadCount; i++) {
        threads.emplace_back([&, i] {
            ready++;

            while (!go.load()) {
                std::this_thread::yield();
            }

            try {
                results[i] = System::setIsolateArguments({"-Dmdapi428.t" + std::to_string(i) + "=1"}) ? 1 : 0;
            } catch (const std::exception &e) {
                errors[i] = e.what();
            }
        });
    }

    std::thread api{[&] {
        ready++;

        while (!go.load()) {
            std::this_thread::yield();
        }

        // Creates the isolate.
        System::getProperty("mdapi428.warm");
    }};

    while (ready.load() < threadCount + 1) {
        std::this_thread::yield();
    }

    go = true;
    api.join();

    for (auto &thread : threads) {
        thread.join();
    }

    int visible = 0;
    std::size_t visibleIndex = 0;
    int trueResults = 0;

    for (std::size_t i = 0; i < threadCount; i++) {
        check(errors[i].empty(), "the setter threw: " + errors[i]);
        check(results[i] == 0 || results[i] == 1, "the setter did not return");

        trueResults += results[i];

        if (System::getProperty("mdapi428.t" + std::to_string(i)) == "1") {
            visible++;
            visibleIndex = i;
        }
    }

    check(visible <= 1, "the arguments of several calls are applied: " + std::to_string(visible));

    if (visible == 1) {
        check(results[visibleIndex] == 1, "the applied arguments are from a call that returned false");
    } else {
        check(trueResults == 0, "no arguments are applied, but " + std::to_string(trueResults) + " calls returned true");
    }

    check(Isolate::isCreated(), "the isolate was not created");
    check(!System::setIsolateArguments({"-Dmdapi428.late=1"}), "the setter returned true after the creation");
}

constexpr const char *ARGUMENTS_VARIABLE = "DXFCPP_ISOLATE_ARGUMENTS";

// The variable as the process sees it, or `nullopt` if it is not set.
std::optional<std::string> readArgumentsVariable() {
#ifdef _WIN32
    std::size_t size = 0;

    if (getenv_s(&size, nullptr, 0, ARGUMENTS_VARIABLE) != 0 || size == 0) {
        return std::nullopt;
    }

    std::string result(size, '\0');

    if (getenv_s(&size, result.data(), size, ARGUMENTS_VARIABLE) != 0) {
        return std::nullopt;
    }

    result.resize(size - 1);

    return result;
#else
    const char *value = std::getenv(ARGUMENTS_VARIABLE);

    return value ? std::optional<std::string>{value} : std::nullopt;
#endif
}

std::string hex(const std::string &s) {
    std::string result;
    char buffer[4];

    for (const auto c : s) {
        std::snprintf(buffer, sizeof(buffer), "%02X", static_cast<unsigned>(static_cast<unsigned char>(c)));
        result += buffer;
        result += ' ';
    }

    return result;
}

// The test is only valid if the variable reached the process exactly: a value lost on the way by ctest or the shell is
// not a defect of the library. The message has the lengths and the bytes of the value of the test, not of the user.
void expectArgumentsVariable(const std::string &expected) {
    const auto actual = readArgumentsVariable();

    check(actual.has_value(), "the variable is not set");
    check(*actual == expected, "the variable is " + std::to_string(actual->size()) + " bytes instead of " +
                                   std::to_string(expected.size()) + ": " + hex(*actual));
}

// The argument of the variable reaches the isolate.
void envApplied() {
    expectArgumentsVariable("-Dmdapi428.e=env");
    check(System::getProperty("mdapi428.e") == "env",
          "the property is '" + System::getProperty("mdapi428.e") + "' instead of 'env'");
}

// The variable comes after the arguments of the code and wins for the same option.
void envWins() {
    check(System::setIsolateArguments({"-Dmdapi428.p=code"}), "the setter returned false before the creation");
    expectArgumentsVariable("-Dmdapi428.p=env");
    check(System::getProperty("mdapi428.p") == "env",
          "the property is '" + System::getProperty("mdapi428.p") + "' instead of 'env'");
}

// A variable of whitespace gives no arguments and no error.
void envWhitespace() {
    expectArgumentsVariable("  \t  ");
    check(System::setProperty("mdapi428.w", "1"), "the isolate does not work");
    check(Isolate::isCreated(), "the isolate was not created");
    check(System::getProperty("mdapi428.w") == "1", "the isolate does not work");
}

// The quotes group the value with spaces and are removed.
void envQuoted() {
    expectArgumentsVariable("-Dmdapi428.q=\"a b\"");
    check(System::getProperty("mdapi428.q") == "a b",
          "the property is '" + System::getProperty("mdapi428.q") + "' instead of 'a b'");
}

// A non-ASCII value (UTF-8). The variable is set by the scenario itself, before the first call: the way a process
// gets a non-ASCII environment differs by the platform, and ctest does not pass it reliably on Windows.
void envNonAscii() {
    // Cyrillic and a Latin letter with an accent, in UTF-8.
    const std::string value = "\xD0\x9F\xD1\x80\xD0\xB8\xD0\xB2\xD0\xB5\xD1\x82-\xC3\xA9";
    bool utf8Variable = true;
#ifdef _WIN32
    // The environment of Windows is UTF-16.
    check(_wputenv_s(L"DXFCPP_ISOLATE_ARGUMENTS", L"-Dmdapi428.u=\x41F\x440\x438\x432\x435\x442-\xE9") == 0,
          "the variable cannot be set");

    // The library reads the variable with getenv_s, in the ANSI code page of the system. With UTF-8 as the code page
    // the value is kept; with another one (1251, 1252) the characters outside of it are lost: the documented limit.
    const auto received = readArgumentsVariable();

    utf8Variable = received && *received == "-Dmdapi428.u=" + value;

    if (!utf8Variable) {
        std::printf("NOTE the ANSI code page does not keep the UTF-8 value of the variable: %zu bytes\n",
                    received ? received->size() : 0);
    }
#else
    check(setenv(ARGUMENTS_VARIABLE, ("-Dmdapi428.u=" + value).c_str(), 1) == 0, "the variable cannot be set");
#endif
    const auto property = System::getProperty("mdapi428.u");

    if (utf8Variable) {
        check(property == value, "the property is " + std::to_string(property.size()) + " bytes instead of " +
                                     std::to_string(value.size()) + ": " + hex(property));
    } else {
        // The creation succeeded and the isolate works.
        check(Isolate::isCreated() && System::setProperty("mdapi428.w", "1"), "the isolate does not work");
    }
}

bool contains(const std::string &text, std::string_view part) {
    return text.find(part) != std::string::npos;
}

// The message of the GraalException of the first call that needs the isolate. The call must throw it.
std::string firstCreateError() {
    try {
        const auto endpoint = DXEndpoint::create();
    } catch (const GraalException &e) {
        return e.what();
    }

    throw std::runtime_error("DXEndpoint::create() did not throw GraalException");
}

// The message tells that the creation failed with the code 22, and has no part of the value that caused it.
void expectCreationError(const std::string &message, std::string_view forbidden) {
    const std::string codeText = CEntryPointErrorsEnumToStr(CEntryPointErrorsEnum::ARGUMENT_PARSING_FAILED);

    check(contains(message, "22"), "no code 22 in the message: " + message);
    check(contains(message, codeText), "no text of the code in the message: " + message);
    check(!contains(message, forbidden), "the message has the value of the argument: " + message);
}

// A wrong value of an argument fails the creation: the first call throws, and the message has the code but not the value.
void badValue() {
    check(System::setIsolateArguments({"-Xmx64q"}), "the setter returned false before the creation");
    expectCreationError(firstCreateError(), "64q");
}

// An unknown option of the isolate fails the creation as well.
void unknownOption() {
    check(System::setIsolateArguments({"-XX:+NoSuchOption"}), "the setter returned false before the creation");
    expectCreationError(firstCreateError(), "NoSuchOption");
}

// The creation is not retried: the next calls, on this thread and on the others, get the same message.
void repeatSame() {
    check(System::setIsolateArguments({"-Xmx64q"}), "the setter returned false before the creation");

    const auto first = firstCreateError();

    expectCreationError(first, "64q");
    check(firstCreateError() == first, "the second message differs");

    constexpr std::size_t threadCount = 4;
    std::vector<std::string> messages(threadCount);
    std::vector<std::thread> threads;

    for (std::size_t i = 0; i < threadCount; i++) {
        threads.emplace_back([&messages, i] {
            try {
                messages[i] = firstCreateError();
            } catch (const std::exception &e) {
                messages[i] = std::string("exception: ") + e.what();
            }
        });
    }

    for (auto &thread : threads) {
        thread.join();
    }

    for (std::size_t i = 0; i < threadCount; i++) {
        check(messages[i] == first, "the message of the thread " + std::to_string(i) + " differs: " + messages[i]);
    }
}

// A quote that is not closed in the variable fails the creation. The message names the variable and the reason.
void envUnmatchedQuote() {
    expectArgumentsVariable("-Dp=\"a");

    const auto message = firstCreateError();

    expectCreationError(message, "-Dp");
    check(contains(message, ARGUMENTS_VARIABLE), "the message does not name the variable: " + message);
    check(contains(message, "quote"), "the message does not name the reason: " + message);
}

// An argument file is not supported.
void envArgFile() {
    expectArgumentsVariable("@args.txt");

    const auto message = firstCreateError();

    expectCreationError(message, "args.txt");
    check(contains(message, ARGUMENTS_VARIABLE), "the message does not name the variable: " + message);
    check(contains(message, "@file"), "the message does not name the reason: " + message);
}

// The system properties throw after the failed creation, the C functions return the error.
void systemProperty() {
    check(System::setIsolateArguments({"-Xmx64q"}), "the setter returned false before the creation");

    bool getThrown = false;
    bool setThrown = false;

    try {
        System::getProperty("x");
    } catch (const GraalException &) {
        getThrown = true;
    }

    try {
        System::setProperty("x", "y");
    } catch (const GraalException &) {
        setThrown = true;
    }

    check(getThrown, "System::getProperty() did not throw GraalException");
    check(setThrown, "System::setProperty() did not throw GraalException");

    char buffer[16] = {'z', 0};

    check(dxfc_system_get_property("x", buffer, sizeof(buffer)) == DXFC_EC_ERROR,
          "dxfc_system_get_property() did not return the error");
    check(dxfc_system_set_property("x", "y") == DXFC_EC_ERROR, "dxfc_system_set_property() did not return the error");
}

// The first call is of the C API: the error does not leave it as an exception.
void cApi() {
    expectArgumentsVariable("-Xmx64q");

    dxfc_dxendpoint_t endpoint = nullptr;

    check(dxfc_dxendpoint_create(nullptr, &endpoint) == DXFC_EC_ERROR, "dxfc_dxendpoint_create() did not fail");
    check(endpoint == nullptr, "dxfc_dxendpoint_create() returned a handle");
}

// The process shuts the library down and exits after the failed creation: no hang, no crash.
void errorShutdown() {
    check(System::setIsolateArguments({"-Xmx64q"}), "the setter returned false before the creation");
    expectCreationError(firstCreateError(), "64q");
    dxfcpp::shutdown();
}

#ifndef _WIN32
// The handler of SIGPIPE that the isolate installs: its presence after the creation, without raising the signal.
bool sigpipeHandlerInstalled() {
    struct sigaction action {};

    check(sigaction(SIGPIPE, nullptr, &action) == 0, "sigaction() failed");

    return action.sa_handler != SIG_DFL;
}

void setSigpipeDefault() {
    struct sigaction action {};

    action.sa_handler = SIG_DFL;
    sigemptyset(&action.sa_mask);

    check(sigaction(SIGPIPE, &action, nullptr) == 0, "sigaction() failed");
}

// By default the isolate installs its handlers of the signals.
void signals() {
    setSigpipeDefault();
    check(System::getProperty("mdapi428.warm").empty(), "unexpected property");
    check(sigpipeHandlerInstalled(), "the isolate did not install the handler of SIGPIPE");
}

// With -XX:-EnableSignalHandling the handlers are not installed.
void signalsDisabled() {
    setSigpipeDefault();
    check(System::setIsolateArguments({"-XX:-EnableSignalHandling"}), "the setter returned false");
    check(System::getProperty("mdapi428.warm").empty(), "unexpected property");
    check(!sigpipeHandlerInstalled(), "the isolate installed the handler of SIGPIPE");
}
#endif

} // namespace

int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::fprintf(stderr, "Usage: %s <scenario>\n", argv[0]);

        return 2;
    }

    const std::string_view scenario = argv[1];

    try {
        if (scenario == "args-visible") {
            argsVisible();
        } else if (scenario == "setter-twice") {
            setterTwice();
        } else if (scenario == "setter-empty") {
            setterEmpty();
        } else if (scenario == "setter-after-creation") {
            setterAfterCreation();
        } else if (scenario == "setter-nul") {
            setterNul();
        } else if (scenario == "setter-concurrent") {
            setterConcurrent();
        } else if (scenario == "env-applied") {
            envApplied();
        } else if (scenario == "env-wins") {
            envWins();
        } else if (scenario == "env-whitespace") {
            envWhitespace();
        } else if (scenario == "env-quoted") {
            envQuoted();
        } else if (scenario == "env-non-ascii") {
            envNonAscii();
        } else if (scenario == "bad-value") {
            badValue();
        } else if (scenario == "unknown-option") {
            unknownOption();
        } else if (scenario == "repeat-same") {
            repeatSame();
        } else if (scenario == "env-unmatched-quote") {
            envUnmatchedQuote();
        } else if (scenario == "env-argfile") {
            envArgFile();
        } else if (scenario == "system-property") {
            systemProperty();
        } else if (scenario == "c-api") {
            cApi();
        } else if (scenario == "error-shutdown") {
            errorShutdown();
#ifndef _WIN32
        } else if (scenario == "signals") {
            signals();
        } else if (scenario == "signals-disabled") {
            signalsDisabled();
#endif
        } else {
            std::fprintf(stderr, "Unknown scenario: %s\n", argv[1]);

            return 2;
        }
    } catch (const std::exception &e) {
        std::fprintf(stderr, "FAIL %s: %s\n", argv[1], e.what());

        return 1;
    }

    std::printf("OK   %s\n", argv[1]);

    return 0;
}
