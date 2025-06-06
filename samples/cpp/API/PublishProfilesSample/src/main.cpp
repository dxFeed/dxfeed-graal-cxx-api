// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfeed_graal_cpp_api/api.hpp>

#include <string>

using namespace dxfcpp;
using namespace dxfcpp::literals;
using namespace std::literals;

/*
 * Using address like ":7700" it starts a server on a specified port where it provides Profile
 * event for any symbol ending with ":TEST" suffix.
 */
int main(int argc, char *argv[]) {
    try {
        if (argc < 2) {
            return 0;
        }

        // Disable QD logging.
        // Logging::init();

        // Parse args.
        const std::string address = argv[1];

        // Create publisher endpoint and connect it to the specified address
        const auto endpoint = DXEndpoint::create(DXEndpoint::Role::PUBLISHER)->connect(address);
        auto publisher = endpoint->getPublisher();

        // Observe Profile subscriptions and publish profiles for "xxx:TEST" symbols
        publisher->getSubscription(Profile::TYPE)
            ->addChangeListener(
                ObservableSubscriptionChangeListener::create([publisher](auto &&symbols) /* symbolsAdded */ {
                    std::vector<std::shared_ptr<Profile>> events{};
                    events.reserve(symbols.size());

                    for (auto &&symbol : symbols) {
                        if (symbol.isStringSymbol()) {
                            if (auto s = symbol.asStringSymbol(); s.size() > 5 && s.rfind(":TEST") == s.size() - 5) {
                                auto profile = std::make_shared<Profile>(s);

                                profile->setDescription("Test symbol");
                                events.push_back(profile);
                            }
                        }
                    }

                    events.shrink_to_fit();
                    publisher->publishEvents(events);
                }));

        std::cin.get();
    } catch (const RuntimeException &e) {
        std::cerr << e << '\n';

        return 1;
    }

    return 0;
}