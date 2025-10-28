// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <dxfeed_graal_cpp_api/api.hpp>

#include "../Args/Args.hpp"

#include <dxfeed_graal_cpp_api/isolated/internal/IsolatedTools.hpp>

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4702)
#include <range/v3/all.hpp>
DXFCXX_DISABLE_MSC_WARNINGS_POP()

namespace dxfcpp::tools {

struct QdsTool {
    static const std::string NAME;
    static const std::string SHORT_DESCRIPTION;
    static const std::string DESCRIPTION;
    static const std::vector<std::string> USAGE;
    static const std::vector<std::string> ADDITIONAL_INFO;
    static const std::vector<ArgType> ARGS;

    [[nodiscard]] static std::string getName() noexcept {
        return NAME;
    }

    [[nodiscard]] static std::string getFullName() noexcept {
        return NAME;
    }

    [[nodiscard]] static std::string prepareHelp(std::size_t namePadding,
                                                 std::size_t nameFieldSize /* padding + name + padding */,
                                                 std::size_t) noexcept {
        return fmt::format("{:{}}{:<{}}{:{}}{}\n", "", namePadding, getFullName(), nameFieldSize - 2 * namePadding, "",
                           namePadding, SHORT_DESCRIPTION);
    }

    struct Args {
        std::string qdsArgs;
        std::optional<std::string> properties;

        static ParseResult<Args> parse(const std::vector<std::string> &args) noexcept {
            std::size_t index = 0;

            if (HelpArg::parse(args, index).result) {
                return ParseResult<Args>::help();
            }

            auto qdsArgs = QdsArgs<>::parse(args);

            if (qdsArgs.isError) {
                return ParseResult<Args>::error(qdsArgs.errorString);
            }

            index++;

            bool propertiesIsParsed{};
            std::optional<std::string> properties{};

            for (; index < args.size();) {
                if (!propertiesIsParsed && PropertiesArg::canParse(args, index)) {
                    auto parseResult = PropertiesArg::parse(args, index);

                    properties = parseResult.result;
                    propertiesIsParsed = true;
                    index = parseResult.nextIndex;
                } else {
                    qdsArgs.result += " " + args[index++];
                }
            }

            return ParseResult<Args>::ok({qdsArgs.result, properties});
        }
    };

    static void run(const Args &args) noexcept {
        using namespace std::literals;

        auto qdsArgs = splitAndTrim(args.qdsArgs, ' ') | ranges::to<std::vector<std::string>>();
        auto parsedProperties = CmdArgsUtils::parseProperties(args.properties);

        System::setProperties(parsedProperties);
        dxfcpp::isolated::internal::IsolatedTools::runTool(qdsArgs);
    }
};

} // namespace dxfcpp::tools