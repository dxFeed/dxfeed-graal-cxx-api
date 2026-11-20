// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "SymbolPool.hpp"

#include "../../../../include/dxfeed_graal_cpp_api/api.hpp"

#include <cstdint>
#include <exception>
#include <string>
#include <unordered_set>
#include <utility>

namespace dxfcpp::tools::nettest {

namespace {

// The seed of QD NetTest (com.devexperts.qd.tools.SymbolGenerator).
constexpr std::int64_t SYMBOL_SEED = 1416948710541751LL;
// The largest -S of the specification. The generator can give about 2.2e9 distinct symbols, so this always ends, but
// 1e8 takes on the order of 1e9 draws and several GB, as in QD NetTest.
constexpr std::int64_t MAX_GENERATED = 100'000'000;

// java.util.Random as its Javadoc specifies it: a 48-bit linear congruential generator.
class JavaRandom {
    static constexpr std::uint64_t MULTIPLIER = 0x5DEECE66DULL;
    static constexpr std::uint64_t ADDEND = 0xBULL;
    static constexpr std::uint64_t MASK = (1ULL << 48) - 1;

    std::uint64_t seed_;

    std::int32_t next(int bits) {
        seed_ = (seed_ * MULTIPLIER + ADDEND) & MASK;

        // The top `bits` of the 48-bit seed; at most 31 here, so the value fits a non-negative int32.
        return static_cast<std::int32_t>(seed_ >> (48 - bits));
    }

    public:
    explicit JavaRandom(std::int64_t seed) : seed_((static_cast<std::uint64_t>(seed) ^ MULTIPLIER) & MASK) {
    }

    // Random.nextInt(bound) for bound > 0.
    std::int32_t nextInt(std::int32_t bound) {
        const std::int32_t bits = next(31);
        const std::int32_t mask = bound - 1;

        if ((bound & mask) == 0) {
            return static_cast<std::int32_t>((static_cast<std::int64_t>(bound) * bits) >> 31);
        }

        // Java rejects a draw while `u - r + (bound - 1)` overflows int; here the sum is computed in 64 bits.
        std::int32_t value = bits % bound;

        for (std::int32_t draw = bits; static_cast<std::int64_t>(draw) - value + mask > INT32_MAX;) {
            draw = next(31);
            value = draw % bound;
        }

        return value;
    }
};

// SymbolGenerator.generateSymbol of QD NetTest with its default length.
std::string nextSymbol(JavaRandom &random) {
    // Two statements: C++ leaves the order of the operands of + unspecified, Java draws nextInt(3) first.
    const auto shortPart = random.nextInt(3);
    const auto length = shortPart + random.nextInt(4) + 1;
    std::string symbol;

    symbol.reserve(static_cast<std::size_t>(length));

    for (std::int32_t index = 0; index < length; ++index) {
        if (random.nextInt(50) == 0) {
            symbol.push_back(static_cast<char>('0' + random.nextInt(10)));
        } else {
            symbol.push_back(static_cast<char>('A' + random.nextInt(26)));
        }
    }

    return symbol;
}

} // namespace

std::vector<std::string> generateSymbols(std::int64_t total, std::int64_t *draws) {
    std::vector<std::string> symbols;
    std::int64_t drawn = 0;

    if (draws != nullptr) {
        *draws = 0;
    }

    if (total < 1 || total > MAX_GENERATED) {
        return symbols;
    }

    symbols.reserve(static_cast<std::size_t>(total));

    std::unordered_set<std::string> seen;
    JavaRandom random(SYMBOL_SEED);

    seen.reserve(static_cast<std::size_t>(total));

    while (static_cast<std::int64_t>(symbols.size()) < total) {
        auto symbol = nextSymbol(random);

        ++drawn;

        if (seen.insert(symbol).second) {
            symbols.push_back(std::move(symbol));
        }
    }

    if (draws != nullptr) {
        *draws = drawn;
    }

    return symbols;
}

SymbolPoolResult readSymbolsFromFile(const std::string &file, std::int64_t total) {
    SymbolPoolResult result;
    std::vector<std::shared_ptr<InstrumentProfile>> profiles;

    try {
        profiles = InstrumentProfileReader::create()->readFromFile(file);
    } catch (const std::exception &e) {
        result.error = "cannot read the IPF file '" + file + "': " + e.what();

        return result;
    }

    auto symbols = std::make_shared<std::vector<std::string>>();

    for (const auto &profile : profiles) {
        if (static_cast<std::int64_t>(symbols->size()) >= total) {
            break;
        }

        if (!profile) {
            continue;
        }

        auto symbol = profile->getSymbol();

        if (!symbol.empty()) {
            symbols->push_back(std::move(symbol));
        }
    }

    if (symbols->empty()) {
        result.error = "the IPF file '" + file + "' has no symbols";

        return result;
    }

    if (static_cast<std::int64_t>(symbols->size()) < total) {
        result.warning = "the IPF file '" + file + "' has " + std::to_string(symbols->size()) +
                         " symbols, fewer than the requested " + std::to_string(total);
    }

    result.symbols = std::move(symbols);
    result.ok = true;

    return result;
}

SymbolPoolResult buildSymbolPool(const Settings &settings) {
    if (!settings.symbolsFile.empty()) {
        return readSymbolsFromFile(settings.symbolsFile, settings.totalSymbols);
    }

    SymbolPoolResult result;
    auto symbols = generateSymbols(settings.totalSymbols);

    if (symbols.empty()) {
        result.error = "cannot generate " + std::to_string(settings.totalSymbols) + " distinct symbols";

        return result;
    }

    result.symbols = std::make_shared<const std::vector<std::string>>(std::move(symbols));
    result.ok = true;

    return result;
}

} // namespace dxfcpp::tools::nettest
