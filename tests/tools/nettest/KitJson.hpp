// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

// A small JSON reader for the files of the NetTest kit (docs/wrapper-guide/nettest): objects, arrays, strings with the
// escapes of JSON (\uXXXX only below 0x80), integers kept exactly as int64, other numbers as double, true, false and
// null. The line breaks of the file may be CRLF. A malformed file throws std::runtime_error.

#pragma once

#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace dxfcpp::tools::nettest::kit {

struct Json {
    enum class Kind { NUL, BOOLEAN, INTEGER, REAL, STRING, ARRAY, OBJECT };

    Kind kind = Kind::NUL;
    bool boolean = false;
    std::int64_t integer = 0;
    double real = 0;
    std::string string{};
    std::vector<Json> array{};
    std::map<std::string, Json> object{};

    bool isNull() const {
        return kind == Kind::NUL;
    }

    const Json &operator[](const std::string &key) const {
        const auto found = object.find(key);

        if (kind != Kind::OBJECT || found == object.end()) {
            throw std::runtime_error("no key '" + key + "'");
        }

        return found->second;
    }

    bool has(const std::string &key) const {
        return kind == Kind::OBJECT && object.count(key) != 0;
    }

    std::int64_t asInteger() const {
        if (kind != Kind::INTEGER) {
            throw std::runtime_error("not an integer");
        }

        return integer;
    }

    const std::string &asString() const {
        if (kind != Kind::STRING) {
            throw std::runtime_error("not a string");
        }

        return string;
    }
};

class JsonReader {
    const std::string &text_;
    std::size_t position_ = 0;

    [[noreturn]] void fail(const std::string &what) const {
        throw std::runtime_error("JSON: " + what + " at offset " + std::to_string(position_));
    }

    void skipSpaces() {
        while (position_ < text_.size() && (text_[position_] == ' ' || text_[position_] == '\n' ||
                                            text_[position_] == '\r' || text_[position_] == '\t')) {
            ++position_;
        }
    }

    bool consume(const char *word) {
        const std::string expected(word);

        if (text_.compare(position_, expected.size(), expected) == 0) {
            position_ += expected.size();

            return true;
        }

        return false;
    }

    std::string readString() {
        if (position_ >= text_.size() || text_[position_] != '"') {
            fail("a string expected");
        }

        ++position_;

        std::string result;

        while (position_ < text_.size() && text_[position_] != '"') {
            char symbol = text_[position_++];

            if (symbol == '\\') {
                if (position_ >= text_.size()) {
                    fail("an unfinished escape");
                }

                symbol = text_[position_++];

                switch (symbol) {
                case 'n':
                    symbol = '\n';
                    break;
                case 'r':
                    symbol = '\r';
                    break;
                case 't':
                    symbol = '\t';
                    break;
                case 'b':
                    symbol = '\b';
                    break;
                case 'f':
                    symbol = '\f';
                    break;
                case 'u': {
                    const auto code = std::strtol(text_.substr(position_, 4).c_str(), nullptr, 16);

                    if (code <= 0 || code >= 0x80) {
                        fail("an escape beyond ASCII");
                    }

                    position_ += 4;
                    symbol = static_cast<char>(code);
                    break;
                }
                default: // '"', '\\', '/'
                    break;
                }
            }

            result += symbol;
        }

        if (position_ >= text_.size()) {
            fail("an unfinished string");
        }

        ++position_;

        return result;
    }

    Json readValue() {
        skipSpaces();

        if (position_ >= text_.size()) {
            fail("a value expected");
        }

        Json value;
        const char first = text_[position_];

        if (first == '{') {
            value.kind = Json::Kind::OBJECT;
            ++position_;
            skipSpaces();

            if (position_ < text_.size() && text_[position_] == '}') {
                ++position_;

                return value;
            }

            while (true) {
                skipSpaces();

                auto key = readString();

                skipSpaces();

                if (position_ >= text_.size() || text_[position_++] != ':') {
                    fail("':' expected");
                }

                value.object[key] = readValue();
                skipSpaces();

                if (position_ >= text_.size()) {
                    fail("an unfinished object");
                }

                const char next = text_[position_++];

                if (next == '}') {
                    return value;
                }

                if (next != ',') {
                    fail("',' or '}' expected");
                }
            }
        }

        if (first == '[') {
            value.kind = Json::Kind::ARRAY;
            ++position_;
            skipSpaces();

            if (position_ < text_.size() && text_[position_] == ']') {
                ++position_;

                return value;
            }

            while (true) {
                value.array.push_back(readValue());
                skipSpaces();

                if (position_ >= text_.size()) {
                    fail("an unfinished array");
                }

                const char next = text_[position_++];

                if (next == ']') {
                    return value;
                }

                if (next != ',') {
                    fail("',' or ']' expected");
                }
            }
        }

        if (first == '"') {
            value.kind = Json::Kind::STRING;
            value.string = readString();

            return value;
        }

        if (consume("true")) {
            value.kind = Json::Kind::BOOLEAN;
            value.boolean = true;

            return value;
        }

        if (consume("false")) {
            value.kind = Json::Kind::BOOLEAN;

            return value;
        }

        if (consume("null")) {
            return value;
        }

        const auto start = position_;

        while (position_ < text_.size() && std::string("+-0123456789.eE").find(text_[position_]) != std::string::npos) {
            ++position_;
        }

        const auto number = text_.substr(start, position_ - start);

        if (number.empty()) {
            fail("a value expected");
        }

        if (number.find_first_of(".eE") == std::string::npos) {
            value.kind = Json::Kind::INTEGER;
            value.integer = std::strtoll(number.c_str(), nullptr, 10);
        } else {
            value.kind = Json::Kind::REAL;
            value.real = std::strtod(number.c_str(), nullptr);
        }

        return value;
    }

  public:
    explicit JsonReader(const std::string &text) : text_(text) {
    }

    Json read() {
        auto value = readValue();

        skipSpaces();

        if (position_ != text_.size()) {
            fail("text after the value");
        }

        return value;
    }
};

inline std::string readText(const std::string &path) {
    std::ifstream stream(path, std::ios::binary);

    if (!stream) {
        throw std::runtime_error("cannot read " + path);
    }

    std::ostringstream text;

    text << stream.rdbuf();

    return text.str();
}

inline Json readJson(const std::string &path) {
    const auto text = readText(path);

    return JsonReader(text).read();
}

} // namespace dxfcpp::tools::nettest::kit
