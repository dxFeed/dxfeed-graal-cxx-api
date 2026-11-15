// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <doctest.h>

#include <dxfeed_graal_cpp_api/api.hpp>

#include "../../tools/Tools/src/Args/Args.hpp"
#include "../../tools/Tools/src/Diagnose/DiagnoseText.hpp"
#include "../../tools/Tools/src/Diagnose/DiagnoseTool.hpp"

#include <string>
#include <vector>

using namespace dxfcpp;
using namespace dxfcpp::tools;
using namespace dxfcpp::tools::diagnose;

TEST_CASE("Diagnose: the hosts and ports of an address") {
    SUBCASE("a single address with codecs, properties and a user") {
        const auto targets = splitAddress("tls+gzip+user:pass@demo.example.com:7300[login=secret]");

        REQUIRE(targets.size() == 1);
        REQUIRE(targets[0].skipped.empty());
        REQUIRE(targets[0].host == "demo.example.com");
        REQUIRE(targets[0].port == 7300);
    }

    SUBCASE("several addresses and a group with shared properties") {
        const auto targets = splitAddress("a.example.com:1,(b.example.com:2,c.example.com:3)[user=x,password=y]");

        REQUIRE(targets.size() == 3);
        REQUIRE(targets[0].host == "a.example.com");
        REQUIRE(targets[1].host == "b.example.com");
        REQUIRE(targets[1].port == 2);
        REQUIRE(targets[2].host == "c.example.com");
        REQUIRE(targets[2].port == 3);
    }

    SUBCASE("ssl properties before the plus") {
        const auto targets = splitAddress("ssl[trustStore=a.jks]+host.example.com:443");

        REQUIRE(targets.size() == 1);
        REQUIRE(targets[0].host == "host.example.com");
        REQUIRE(targets[0].port == 443);
    }

    SUBCASE("the parts that are not probed") {
        REQUIRE(splitAddress(":7300")[0].skipped == "a server address (no host)");
        REQUIRE(splitAddress("tape:file.txt")[0].skipped == "not a host:port socket address");
        REQUIRE(splitAddress("dxlink:wss://demo.example.com/dxlink-ws")[0].skipped ==
                "not a host:port socket address");
        REQUIRE(splitAddress("host.example.com:7300-7310")[0].skipped == "a range of ports");
        REQUIRE(splitAddress("host.example.com")[0].skipped == "no port");
        REQUIRE(splitAddress("host.example.com:70000")[0].skipped == "the port is out of range");
        REQUIRE(splitAddress("[::1]:7300")[0].skipped == "an IPv6 literal is not probed");
    }
}

TEST_CASE("Diagnose: logins, passwords and tokens are hidden") {
    REQUIRE(redact("host:7300[login=abc,password=s3cr3t]") == "host:7300[login=***,password=***]");
    REQUIRE(redact("host:7300[user=me,Password=s3cr3t,tls=true]") == "host:7300[user=***,Password=***,tls=true]");
    REQUIRE(redact("dxfeed.password=s3cr3t,dxfeed.address=host:7300") ==
            "dxfeed.password=***,dxfeed.address=host:7300");
    REQUIRE(redact("tls+me:s3cr3t@host:7300") == "tls+***@host:7300");
    REQUIRE(redact("connect to me:s3cr3t@host:7300 failed") == "connect to ***@host:7300 failed");
    REQUIRE(redact("token=abc def") == "token=*** def");
    REQUIRE(redact("Quote{AAPL, bid=1.5@Q}") == "Quote{AAPL, bid=1.5@Q}");
    REQUIRE(redact("no secrets: host:7300") == "no secrets: host:7300");

    SUBCASE("keys that end with a secret word") {
        REQUIRE(redact("ssl[trustStore=a.jks,trustStorePassword=x1]+host:443") ==
                "ssl[trustStore=a.jks,trustStorePassword=***]+host:443");
        REQUIRE(redact("authToken=x1 apiKey=x2 clientSecret=x3 keyStorePwd=x4") ==
                "authToken=*** apiKey=*** clientSecret=*** keyStorePwd=***");
        REQUIRE(redact("dxfeed.address=host:7300,keyStore=a.jks") == "dxfeed.address=host:7300,keyStore=a.jks");
    }

    SUBCASE("values with delimiters") {
        REQUIRE(redact("password=\"a b\" next") == "password=*** next");
        REQUIRE(redact("password='a,b'") == "password=***");
        REQUIRE(redact("host:7300[password=a;b)c,login=d]") == "host:7300[password=***,login=***]");
    }

    SUBCASE("user:password@ with delimiters in the password") {
        REQUIRE(redact("tls+me:p/ss@host:7300") == "tls+***@host:7300");
        REQUIRE(redact("me:pa+ss@host:7300") == "***@host:7300");
        REQUIRE(redact("me:p@ss@host:7300") == "***@host:7300");
        REQUIRE(redact("dxlink:wss://me:p:ss@host/dxlink-ws") == "dxlink:wss://***@host/dxlink-ws");
        REQUIRE(redact("ssl[trustStore=a.jks]+me:x@host:7300") == "ssl[trustStore=a.jks]+***@host:7300");
        REQUIRE(redact("(me:x@a:1,b:2)") == "(***@a:1,b:2)");
    }
}

TEST_CASE("Diagnose: the verdict") {
    RunSummary summary{};
    summary.types = {"Quote", "Trade"};
    summary.symbols = {"AAPL", "IBM"};

    SUBCASE("all pairs have events") {
        summary.received = {{"Quote", "AAPL"}, {"Quote", "IBM"}, {"Trade", "AAPL"}, {"Trade", "IBM"}};
        summary.wasConnected = true;

        REQUIRE(computeVerdict(summary).kind == VerdictKind::DATA);
        REQUIRE(exitCode(VerdictKind::DATA) == 0);
    }

    SUBCASE("some pairs have events") {
        summary.received = {{"Quote", "AAPL"}};
        summary.wasConnected = true;

        const auto verdict = computeVerdict(summary);

        REQUIRE(verdict.kind == VerdictKind::PARTIAL_DATA);
        REQUIRE(verdict.missing == std::vector<std::string>{"Quote IBM", "Trade AAPL", "Trade IBM"});
    }

    SUBCASE("the wildcard symbol matches any symbol") {
        summary.symbols = {"*"};
        summary.received = {{"Quote", "AAPL"}, {"Trade", "MSFT"}};

        REQUIRE(computeVerdict(summary).kind == VerdictKind::DATA);
    }

    SUBCASE("connected without events") {
        summary.wasConnected = true;

        const auto verdict = computeVerdict(summary);

        REQUIRE(verdict.kind == VerdictKind::CONNECTED_NO_DATA);
        REQUIRE(verdict.hint.find("check the endpoint IPF") != std::string::npos);
        REQUIRE(verdict.missing.empty());
        REQUIRE(verdict.hint.find("Last connection error") == std::string::npos);
    }

    SUBCASE("connected without events, then an error") {
        ConnectionError error{};
        error.category = ConnectionError::Category::CONNECT_FAILED;
        summary.lastError = error;
        summary.wasConnected = true;

        const auto verdict = computeVerdict(summary);

        REQUIRE(verdict.kind == VerdictKind::CONNECTED_NO_DATA);
        REQUIRE(verdict.hint.find("Last connection error") != std::string::npos);
    }

    SUBCASE("a DNS error") {
        ConnectionError error{};
        error.category = ConnectionError::Category::DNS;
        summary.lastError = error;

        REQUIRE(computeVerdict(summary).kind == VerdictKind::CANNOT_RESOLVE);
    }

    SUBCASE("no error and no host resolved by the probe") {
        summary.noHostResolved = true;

        REQUIRE(computeVerdict(summary).kind == VerdictKind::CANNOT_RESOLVE);
    }

    SUBCASE("a connect error") {
        ConnectionError error{};
        error.category = ConnectionError::Category::CONNECT_FAILED;
        summary.lastError = error;
        summary.noHostResolved = true;

        const auto verdict = computeVerdict(summary);

        REQUIRE(verdict.kind == VerdictKind::CANNOT_CONNECT);
        REQUIRE(verdict.hint.find("Last connection error") != std::string::npos);
        REQUIRE(exitCode(verdict.kind) == 4);
    }

    SUBCASE("no connect and no error") {
        REQUIRE(computeVerdict(summary).kind == VerdictKind::CANNOT_CONNECT);
    }
}

TEST_CASE("Diagnose: the arguments") {
    using Args = DiagnoseTool::Args;

    SUBCASE("the required arguments and the defaults") {
        const auto result = Args::parse({"demo.example.com:7300", "Quote", "AAPL"});

        REQUIRE_FALSE(result.isError);
        REQUIRE(result.result.address == "demo.example.com:7300");
        REQUIRE(result.result.types == "Quote");
        REQUIRE(result.result.symbols == "AAPL");
        REQUIRE(result.result.duration == DiagnoseTool::DEFAULT_DURATION);
        REQUIRE_FALSE(result.result.compare);
        REQUIRE_FALSE(result.result.report);
        REQUIRE_FALSE(result.result.properties);
    }

    SUBCASE("the options") {
        const auto result = Args::parse({"a:1", "Quote,Trade", "AAPL", "--compare", "b:2", "--duration", "3", "-o",
                                         "report.txt", "-p", "x=y"});

        REQUIRE_FALSE(result.isError);
        REQUIRE(result.result.compare == "b:2");
        REQUIRE(result.result.duration == 3);
        REQUIRE(result.result.report == "report.txt");
        REQUIRE(result.result.properties == "x=y");
        REQUIRE(Args::parse({"a:1", "Quote", "AAPL", "--report", "r.txt"}).result.report == "r.txt");
    }

    SUBCASE("a wrong duration is an error") {
        REQUIRE(Args::parse({"a:1", "Quote", "AAPL", "--duration", "x"}).isError);
        REQUIRE(Args::parse({"a:1", "Quote", "AAPL", "--duration", "0"}).isError);
        REQUIRE(Args::parse({"a:1", "Quote", "AAPL", "--duration", "-2"}).isError);
        REQUIRE(Args::parse({"a:1", "Quote", "AAPL", "--duration", "3601"}).isError);        REQUIRE(Args::parse({"a:1", "Quote", "AAPL", "--duration", "3600"}).result.duration == 3600);
    }

    SUBCASE("the required arguments are required") {
        REQUIRE(Args::parse({"a:1", "Quote"}).isError);
        REQUIRE(Args::parse({}).isError);
    }

    SUBCASE("help") {
        REQUIRE(Args::parse({"--help"}).isHelp);
    }

    SUBCASE("the option names") {
        REQUIRE(CompareArg::getFullName() == "--compare");
        REQUIRE(DurationArg::getFullName() == "--duration");
        REQUIRE(ReportArg::getFullName() == "-o, --report");
    }
}

TEST_CASE("Diagnose: the probe of a host that does not resolve") {
    using namespace std::chrono_literals;

    const auto probe = probeHost("diagnose-test.invalid", 7300, 1s);

    REQUIRE_FALSE(probe.resolved);
    REQUIRE_FALSE(probe.error.empty());
    REQUIRE(probe.ips.empty());
}
