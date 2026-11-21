// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

// A check of a JSON value against the subset of JSON Schema that nettest-summary.schema.json of the kit uses: type,
// const, enum, minimum, maximum, minLength, required, properties, additionalProperties and items.

#pragma once

#include "KitJson.hpp"

#include <algorithm>
#include <functional>
#include <string>
#include <vector>

namespace dxfcpp::tools::nettest::kit {

inline bool sameJson(const Json &first, const Json &second) {
    if (first.kind != second.kind) {
        return false;
    }

    switch (first.kind) {
    case Json::Kind::NUL:
        return true;
    case Json::Kind::BOOLEAN:
        return first.boolean == second.boolean;
    case Json::Kind::INTEGER:
        return first.integer == second.integer;
    case Json::Kind::REAL:
        // The exact equality of JSON numbers, without the warning of a floating-point comparison.
        return std::equal_to<>{}(first.real, second.real);
    case Json::Kind::STRING:
        return first.string == second.string;
    case Json::Kind::ARRAY:
        if (first.array.size() != second.array.size()) {
            return false;
        }

        for (std::size_t index = 0; index < first.array.size(); ++index) {
            if (!sameJson(first.array[index], second.array[index])) {
                return false;
            }
        }

        return true;
    case Json::Kind::OBJECT:
        if (first.object.size() != second.object.size()) {
            return false;
        }

        for (const auto &[key, value] : first.object) {
            if (!second.has(key) || !sameJson(value, second[key])) {
                return false;
            }
        }

        return true;
    }

    return false;
}

inline bool hasType(const Json &value, const std::string &type) {
    switch (value.kind) {
    case Json::Kind::NUL:
        return type == "null";
    case Json::Kind::BOOLEAN:
        return type == "boolean";
    case Json::Kind::INTEGER:
        return type == "integer" || type == "number";
    case Json::Kind::REAL:
        return type == "number";
    case Json::Kind::STRING:
        return type == "string";
    case Json::Kind::ARRAY:
        return type == "array";
    case Json::Kind::OBJECT:
        return type == "object";
    }

    return false;
}

// The number of the code points of a UTF-8 text.
inline std::size_t codePoints(const std::string &text) {
    std::size_t count = 0;

    for (const auto character : text) {
        count += (static_cast<unsigned char>(character) & 0xC0) != 0x80 ? 1 : 0;
    }

    return count;
}

// Appends a message for each place where the value breaks the schema; a keyword that the check does not know is an
// error too, so that a schema of the kit that grows past the subset is not passed silently.
inline void checkSchema(const Json &value, const Json &schema, const std::string &path,
                        std::vector<std::string> &errors) {
    static const std::vector<std::string> KEYWORDS{
        "$schema", "$id",      "title",    "type",       "const",      "enum",
        "minimum", "maximum",  "minLength", "required",  "properties", "additionalProperties",
        "items"};

    for (const auto &[keyword, unused] : schema.object) {
        if (std::find(KEYWORDS.begin(), KEYWORDS.end(), keyword) == KEYWORDS.end()) {
            errors.push_back(path + ": the schema keyword " + keyword + " is not supported by the check");
        }
    }

    if (schema.has("type")) {
        const auto &type = schema["type"];
        bool matches = false;

        if (type.kind == Json::Kind::ARRAY) {
            for (const auto &name : type.array) {
                matches = matches || hasType(value, name.asString());
            }
        } else {
            matches = hasType(value, type.asString());
        }

        if (!matches) {
            errors.push_back(path + ": wrong type");

            return;
        }
    }

    if (schema.has("const") && !sameJson(value, schema["const"])) {
        errors.push_back(path + ": not the const value");
    }

    if (schema.has("enum")) {
        bool found = false;

        for (const auto &option : schema["enum"].array) {
            found = found || sameJson(value, option);
        }

        if (!found) {
            errors.push_back(path + ": not one of the enum values");
        }
    }

    if (value.kind == Json::Kind::INTEGER || value.kind == Json::Kind::REAL) {
        if (schema.has("minimum") && value.asNumber() < schema["minimum"].asNumber()) {
            errors.push_back(path + ": below the minimum");
        }

        if (schema.has("maximum") && value.asNumber() > schema["maximum"].asNumber()) {
            errors.push_back(path + ": above the maximum");
        }
    }

    if (value.kind == Json::Kind::STRING && schema.has("minLength") &&
        static_cast<double>(codePoints(value.string)) < schema["minLength"].asNumber()) {
        errors.push_back(path + ": shorter than the minLength");
    }

    if (value.kind == Json::Kind::OBJECT) {
        if (schema.has("required")) {
            for (const auto &key : schema["required"].array) {
                if (!value.has(key.asString())) {
                    errors.push_back(path + ": no key " + key.asString());
                }
            }
        }

        for (const auto &[key, member] : value.object) {
            if (schema.has("properties") && schema["properties"].has(key)) {
                checkSchema(member, schema["properties"][key], path + "." + key, errors);
            } else if (schema.has("additionalProperties")) {
                const auto &additional = schema["additionalProperties"];

                if (additional.kind == Json::Kind::BOOLEAN) {
                    if (!additional.boolean) {
                        errors.push_back(path + ": unexpected key " + key);
                    }
                } else {
                    checkSchema(member, additional, path + "." + key, errors);
                }
            }
        }
    }

    if (value.kind == Json::Kind::ARRAY && schema.has("items")) {
        for (std::size_t index = 0; index < value.array.size(); ++index) {
            checkSchema(value.array[index], schema["items"], path + "[" + std::to_string(index) + "]", errors);
        }
    }
}

} // namespace dxfcpp::tools::nettest::kit
