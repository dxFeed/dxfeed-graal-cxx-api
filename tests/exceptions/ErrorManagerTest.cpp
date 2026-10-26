// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <doctest.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include "../support/TestSupport.hpp"

#include <atomic>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <functional>
#include <memory>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#ifdef _WIN32
#    include <io.h>
#else
#    include <unistd.h>
#endif

using namespace dxfcpp;
using dxfcpp::test::ErrorManagerGuard;

namespace {

// Redirects the file descriptor of stderr to a file while it lives. The name of the file is unique, so that runs of
// the test in parallel (for example, of several builds) do not share it.
struct StderrCapture {
    StderrCapture()
        : path_(std::filesystem::temp_directory_path() /
                ("dxfcpp-ErrorManagerTest-stderr-" +
                 std::to_string(std::chrono::system_clock::now().time_since_epoch().count()) + "-" +
                 std::to_string(std::hash<std::thread::id>{}(std::this_thread::get_id())) + ".txt")) {
        std::fflush(stderr);
        file_ = std::fopen(path_.string().c_str(), "w");
        REQUIRE(file_ != nullptr);
#ifdef _WIN32
        saved_ = _dup(_fileno(stderr));
        _dup2(_fileno(file_), _fileno(stderr));
#else
        saved_ = dup(fileno(stderr));
        dup2(fileno(file_), fileno(stderr));
#endif
    }

    std::string stop() {
        if (file_ != nullptr) {
            std::fflush(stderr);
#ifdef _WIN32
            _dup2(saved_, _fileno(stderr));
            _close(saved_);
#else
            dup2(saved_, fileno(stderr));
            close(saved_);
#endif
            std::fclose(file_);
            file_ = nullptr;
        }

        std::ifstream in(path_);
        std::stringstream ss;

        ss << in.rdbuf();

        return ss.str();
    }

    ~StderrCapture() {
        stop();
        std::error_code ec;
        std::filesystem::remove(path_, ec);
    }

    private:
    std::filesystem::path path_;
    std::FILE *file_{};
    int saved_{-1};
};

std::uint64_t currentThreadId() {
    return static_cast<std::uint64_t>(std::hash<std::thread::id>{}(std::this_thread::get_id()));
}

} // namespace

TEST_CASE("ErrorManager::report fills the fields of the error and increases the id") {
    ErrorManagerGuard guard{};

    const auto before = std::chrono::duration_cast<std::chrono::milliseconds>(
                            std::chrono::system_clock::now().time_since_epoch())
                            .count();
    const auto line = __LINE__ + 1;
    ErrorManager::report("Component", "message 1");
    ErrorManager::report("Component", "message 2");

    const auto errors = ErrorManager::getErrors();

    REQUIRE(errors.size() == 2);
    CHECK(errors[0].component == "Component");
    CHECK(errors[0].message == "message 1");
    CHECK(errors[0].location == "ErrorManagerTest.cpp:" + std::to_string(line));
    CHECK(errors[0].time >= before);
    CHECK(errors[0].threadId == currentThreadId());
    CHECK(errors[0].javaClassName.empty());
    CHECK(errors[0].javaStackTrace.empty());
    CHECK(errors[1].id > errors[0].id);
    CHECK(errors[0].id > 0);

    const auto last = ErrorManager::getLastError();

    REQUIRE(last.has_value());
    CHECK(last->id == errors[1].id);
    CHECK(last->toString().find("Component") != std::string::npos);
    CHECK(last->toString().find("message 2") != std::string::npos);
    CHECK(last->toString().find('\n') == std::string::npos);
}

TEST_CASE("ErrorManager::reportException classifies the exception") {
    ErrorManagerGuard guard{};

    SUBCASE("JavaException gives the Java class, message and stack trace") {
        ErrorManager::reportException(
            "Java", std::make_exception_ptr(JavaException("java message", "java.lang.IllegalStateException",
                                                          "\tat Foo.bar(Foo.java:1)")));

        const auto error = ErrorManager::getLastError();

        REQUIRE(error.has_value());
        CHECK(error->component == "Java");
        CHECK(error->message == "java message");
        CHECK(error->javaClassName == "java.lang.IllegalStateException");
        CHECK(error->javaStackTrace == "\tat Foo.bar(Foo.java:1)");
        CHECK(error->toString().find("java.lang.IllegalStateException") != std::string::npos);
    }

    SUBCASE("std::exception gives what()") {
        const auto line = __LINE__ + 1;
        ErrorManager::reportException("Std", std::make_exception_ptr(std::runtime_error("x")));

        const auto error = ErrorManager::getLastError();

        REQUIRE(error.has_value());
        CHECK(error->component == "Std");
        CHECK(error->message == "x");
        CHECK(error->location == "ErrorManagerTest.cpp:" + std::to_string(line));
        CHECK(error->javaClassName.empty());
    }

    SUBCASE("Another exception gives 'unknown exception'") {
        ErrorManager::reportException("Int", std::make_exception_ptr(42));

        const auto error = ErrorManager::getLastError();

        REQUIRE(error.has_value());
        CHECK(error->message == "unknown exception");
    }

    SUBCASE("A null exception gives 'unknown exception'") {
        ErrorManager::reportException("Null", nullptr);

        const auto error = ErrorManager::getLastError();

        REQUIRE(error.has_value());
        CHECK(error->message == "unknown exception");
    }
}

TEST_CASE("JavaException keeps the Java data apart from what()") {
    const JavaException e("java message", "java.lang.IllegalStateException", "trace");
    const auto copy = e; // NOLINT(*-unnecessary-copy-initialization)

    CHECK(copy.getJavaMessage() == "java message");
    CHECK(copy.getJavaClassName() == "java.lang.IllegalStateException");
    CHECK(copy.getJavaStackTrace() == "trace");
    CHECK(std::string(copy.what()) ==
          "Java exception of type 'java.lang.IllegalStateException' was thrown. java message");
}

TEST_CASE("The journal keeps the last errors up to the capacity") {
    ErrorManagerGuard guard{};

    ErrorManager::setCapacity(3);
    CHECK(ErrorManager::getCapacity() == 3);

    for (int i = 1; i <= 5; ++i) {
        ErrorManager::report("C", std::to_string(i));
    }

    auto errors = ErrorManager::getErrors();

    REQUIRE(errors.size() == 3);
    CHECK(errors[0].message == "3");
    CHECK(errors[1].message == "4");
    CHECK(errors[2].message == "5");

    ErrorManager::setCapacity(1);
    errors = ErrorManager::getErrors();

    REQUIRE(errors.size() == 1);
    CHECK(errors[0].message == "5");

    ErrorManager::setCapacity(0);

    CHECK(ErrorManager::getErrors().empty());
    CHECK_FALSE(ErrorManager::getLastError().has_value());

    std::vector<std::string> handled{};

    ErrorManager::setHandler([&](const ErrorInfo &error) {
        handled.push_back(error.message);
    });
    ErrorManager::report("C", "6");

    CHECK(ErrorManager::getErrors().empty());
    CHECK(handled == std::vector<std::string>{"6"});
}

TEST_CASE("A copy of the journal does not change after new reports and clearErrors") {
    ErrorManagerGuard guard{};

    ErrorManager::report("C", "1");

    const auto copy = ErrorManager::getErrors();
    const auto last = ErrorManager::getLastError();

    ErrorManager::report("C", "2");
    ErrorManager::clearErrors();

    REQUIRE(copy.size() == 1);
    CHECK(copy[0].message == "1");
    REQUIRE(last.has_value());
    CHECK(last->message == "1");
    CHECK(ErrorManager::getErrors().empty());

    ErrorManager::report("C", "3");

    const auto next = ErrorManager::getLastError();

    REQUIRE(next.has_value());
    CHECK(next->id > last->id + 1);
}

TEST_CASE("An exception of the handler does not leave report") {
    ErrorManagerGuard guard{};

    ErrorManager::setHandler([](const ErrorInfo &) {
        throw std::runtime_error("handler");
    });

    CHECK_NOTHROW(ErrorManager::report("C", "1"));
    CHECK_NOTHROW(ErrorManager::reportException("C", std::make_exception_ptr(std::runtime_error("2"))));
    CHECK(ErrorManager::getErrors().size() == 2);
}

TEST_CASE("An error reported from the handler goes to the journal only") {
    ErrorManagerGuard guard{};
    int calls = 0;

    ErrorManager::setHandler([&](const ErrorInfo &) {
        ++calls;
        ErrorManager::report("Handler", "nested");
    });
    ErrorManager::report("C", "outer");

    CHECK(calls == 1);

    const auto errors = ErrorManager::getErrors();

    REQUIRE(errors.size() == 2);
    CHECK(errors[0].message == "outer");
    CHECK(errors[1].message == "nested");

    // The guard of the handler is per call: the next report reaches the handler again.
    ErrorManager::report("C", "next");

    CHECK(calls == 2);
}

TEST_CASE("The default handler writes one line to stderr, a null handler writes nothing") {
    ErrorManagerGuard guard{};

    {
        StderrCapture capture{};

        ErrorManager::setHandler({});
        ErrorManager::report("Silent", "nothing");

        CHECK(capture.stop().empty());
    }

    {
        StderrCapture capture{};

        ErrorManager::resetHandler();
        ErrorManager::report("Loud", "something");

        const auto output = capture.stop();

        CHECK(output.find("Loud") != std::string::npos);
        CHECK(output.find("something") != std::string::npos);
        CHECK(output.find('\n') == output.size() - 1);
    }

    {
        StderrCapture capture{};

        ErrorManager::defaultHandler(ErrorInfo{1, 0, 0, "Direct", "File.cpp:1", "direct", "", ""});

        CHECK(capture.stop().find("direct") != std::string::npos);
    }
}

TEST_CASE("The handler runs on the reporting thread without the locks of ErrorManager") {
    ErrorManagerGuard guard{};
    std::uint64_t handlerThreadId{};
    std::size_t journalSize{};
    std::uint64_t reporterThreadId{};

    ErrorManager::setHandler([&](const ErrorInfo &) {
        handlerThreadId = currentThreadId();
        journalSize = ErrorManager::getErrors().size();
        ErrorManager::setHandler({});
    });

    std::thread reporter([&] {
        reporterThreadId = currentThreadId();
        ErrorManager::report("Thread", "from a thread");
    });

    reporter.join();

    CHECK(handlerThreadId == reporterThreadId);
    CHECK(journalSize == 1);

    // The handler replaced itself with a null one.
    ErrorManager::report("Thread", "second");

    CHECK(ErrorManager::getErrors().size() == 2);
}

TEST_CASE("ErrorManager is safe for concurrent reports, readers and handler changes") {
    ErrorManagerGuard guard{};
    std::atomic<bool> stop{false};
    std::atomic<std::uint64_t> handled{0};
    std::vector<std::thread> threads{};

    for (int i = 0; i < 4; ++i) {
        threads.emplace_back([&, i] {
            std::size_t n = 0;

            while (!stop.load()) {
                if (n % 2 == 0) {
                    ErrorManager::report("Reporter", std::to_string(i));
                } else {
                    ErrorManager::reportException("Reporter", std::make_exception_ptr(std::runtime_error("e")));
                }

                ++n;
            }
        });
    }

    threads.emplace_back([&] {
        std::size_t n = 0;

        while (!stop.load()) {
            if (n % 2 == 0) {
                ErrorManager::setHandler([&](const ErrorInfo &) {
                    handled.fetch_add(1);
                });
            } else {
                ErrorManager::setHandler({});
            }

            ErrorManager::setCapacity(n % 3 == 0 ? std::size_t{0} : std::size_t{16});
            ++n;
        }
    });

    std::atomic<bool> ordered{true};

    threads.emplace_back([&] {
        while (!stop.load()) {
            const auto errors = ErrorManager::getErrors();

            for (std::size_t j = 1; j < errors.size(); ++j) {
                if (errors[j].id <= errors[j - 1].id) {
                    ordered = false;
                }
            }

            (void)ErrorManager::getLastError();

            if (errors.size() > 8) {
                ErrorManager::clearErrors();
            }
        }
    });

    std::this_thread::sleep_for(std::chrono::seconds(1));
    stop = true;

    for (auto &thread : threads) {
        thread.join();
    }

    ErrorManager::setCapacity(16);

    CHECK(ordered.load());
    CHECK(handled.load() > 0);
    CHECK(ErrorManager::getErrors().size() <= 16);
}

TEST_CASE("The replaced handler is destroyed without the locks of ErrorManager") {
    ErrorManagerGuard guard{};

    // An object captured by the handler reports an error from its destructor, as a logger of the program might.
    struct ReportOnDestruction {
        ~ReportOnDestruction() {
            ErrorManager::report("Handler", "destroyed");
        }
    };

    auto capture = std::make_shared<ReportOnDestruction>();

    ErrorManager::setHandler([capture](const ErrorInfo &) {
    });
    capture.reset();

    // Under the lock, the report from the destructor would lock the mutex of ErrorManager again.
    ErrorManager::setHandler({});

    const auto error = ErrorManager::getLastError();

    REQUIRE(error.has_value());
    CHECK(error->message == "destroyed");
}
