// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../internal/Common.hpp"
#include "../internal/JavaObjectHandle.hpp"
#include <dxfeed_graal_cpp_api/entity/SharedEntity.hpp>

DXFCPP_BEGIN_NAMESPACE

/**
 * A wrapper class for Classification of Financial Instruments code as defined in ISO 10962 standard.
 * Main purpose is to provide code validity checks and to construct textual explanation of CFI code
 * as defined in the standard via API. This class does not provide API-accessible constants
 * for specific instrument attributes and values.
 */
struct DXFCPP_EXPORT CFI : RequireMakeShared<CFI> {
    /// The alias to a type of shared pointer to the CFI object
    using Ptr = std::shared_ptr<CFI>;

    /// The alias to a type of unique pointer to the CFI object
    using Unique = std::unique_ptr<CFI>;

    private:
    mutable JavaObjectHandle<CFI> handle_;
    mutable std::mutex mtx_{};
    mutable bool initialized_{};
    std::function<JavaObjectHandle<CFI>()> initializer_;

    void init() const;

    public:
    /**
     * Empty CFI - it has code "XXXXXX".
     */
    static const Ptr EMPTY;

    // lazy c-tor
    CFI(LockExternalConstructionTag, std::function<JavaObjectHandle<CFI>()> initializer);

    CFI(LockExternalConstructionTag, JavaObjectHandle<CFI> &&handle);

    ~CFI() noexcept override;

    /**
     * Returns an instance of CFI for specified CFI code.
     * Accepts short code and expands it to 6 letters by appending "X" at the end.
     *
     * @param code The CFI code.
     * @return The CFI.
     * @throws JavaException("IllegalArgumentException") if the `code` is invalid.
     */
    static Ptr valueOf(const StringLike &code);

    /**
     * Returns an instance of CFI for a specified integer representation of CFI code.
     *
     * @param intCode An integer representation of CFI code.
     * @return The CFI.
     * @throws JavaException("IllegalArgumentException") if the `intCode` is invalid.
     */
    static Ptr valueOf(std::int32_t intCode);

    /**
     * Returns CFI code. The code always has a length of 6 characters.
     *
     * @return CFI code.
     */
    std::string getCode() const;

    /**
     * Returns integer representation of CFI code.
     *
     * @return An integer representation of CFI code.
     */
    std::int32_t getIntCode() const;

    /**
     * Returns a single UTF16 character for the instrument category - the first character of the CFI code.
     *
     * @return The category.
     */
    std::int16_t getCategory() const;

    /**
     * Returns a single ASCII character for the instrument category - the first character of the CFI code.
     *
     * @return The category.
     */
    char getCategoryChar() const;

    /**
     * Returns a single UTF16 character for the instrument group - the second character of the CFI code.
     *
     * @return The group.
     */
    std::int16_t getGroup() const;

    /**
     * Returns a single ASCII character for the instrument group - the second character of the CFI code.
     *
     * @return The group.
     */
    char getGroupChar() const;

    /**
     * Returns `true` if the corresponding instrument is an equity.
     *
     * @return `true` if the corresponding instrument is an equity.
     */
    bool isEquity() const;

    /**
     * Returns `true` if the corresponding instrument is a debt instrument.
     *
     * @return `true` if the corresponding instrument is a debt instrument.
     */
    bool isDebtInstrument() const;

    /**
     * Returns `true` if the corresponding instrument is an entitlement (right).
     *
     * @return `true` if the corresponding instrument is an entitlement (right).
     */
    bool isEntitlement() const;

    /**
     * Returns `true` if the corresponding instrument is an option.
     *
     * @return `true` if the corresponding instrument is an option.
     */
    bool isOption() const;

    /**
     * Returns `true` if the corresponding instrument is a future.
     *
     * @return `true` if the corresponding instrument is a future.
     */
    bool isFuture() const;

    /**
     * Returns `true` if the corresponding instrument is an "other" (miscellaneous) instrument.
     *
     * @return `true` if the corresponding instrument is an "other" (miscellaneous) instrument.
     */
    bool isOther() const;

    struct Value;

    /**
     * Returns container of values that explain the meaning of each character in the CFI code.
     * Array always has a length of 6, and each value explains a corresponding character.
     * If a certain character is unapplicable, unknown or unrecognized - corresponding
     * value will contain reference to this fact.
     *
     * @return The values.
     */
    std::vector<std::shared_ptr<Value>> decipher() const;

    /**
     * Returns a short textual description of this CFI code by listing names of all values
     * for the characters in this CFI code. See also the CFI::decipher () method.
     *
     * @return The description.
     */
    std::string describe() const;

    /**
     * Returns `true` if this object is equal to `other` object
     *
     * @param other Another object
     * @return `true` if this object is equal to `other` object
     */
    bool operator==(const CFI &other) const;

    /**
     * Returns `true` if this object is equal to `other` object
     *
     * @param other Another object
     * @return `true` if this object is equal to `other` object
     */
    bool operator==(const CFI::Ptr &other) const {
        return *this == *other;
    }

    /**
     * @return A hash code value for this object.
     */
    std::size_t hashCode() const noexcept;

    /**
     * Returns a string representation of the current object.
     *
     * @return A string representation
     */
    std::string toString() const override;

    friend std::ostream &operator<<(std::ostream &os, const CFI &cfi) {
        return os << cfi.toString();
    }

    friend std::ostream &operator<<(std::ostream &os, const CFI::Ptr &cfi) {
        return os << cfi->toString();
    }

    /**
     * Describes a single attribute with all values as defined in the ISO 10962 standard.
     */
    struct DXFCPP_EXPORT Attribute : RequireMakeShared<Attribute> {
        /// The alias to a type of shared pointer to the Attribute object
        using Ptr = std::shared_ptr<Attribute>;

        /// The alias to a type of unique pointer to the Attribute object
        using Unique = std::unique_ptr<Attribute>;

        private:
        mutable JavaObjectHandle<Attribute> handle_;

        public:
        Attribute(LockExternalConstructionTag, JavaObjectHandle<Attribute> &&handle);

        ~Attribute() noexcept override;

        /**
         * Returns a short name of this attribute.
         *
         * @return The short name.
         */
        std::string getName() const;

        /**
         * Returns a description of this attribute.
         *
         * @return The description.
         */
        std::string getDescription() const;

        /**
         * Returns values of this attribute.
         *
         * @return The values.
         */
        std::vector<std::shared_ptr<CFI::Value>> getValues() const;

        /**
         * Returns `true` if this object is equal to `other` object
         *
         * @param other Another object
         * @return `true` if this object is equal to `other` object
         */
        bool operator==(const Attribute &other) const;

        /**
         * Returns `true` if this object is equal to `other` object
         *
         * @param other Another object
         * @return `true` if this object is equal to `other` object
         */
        bool operator==(const Attribute::Ptr &other) const {
            return *this == *other;
        }

        /**
         * @return A hash code value for this object.
         */
        std::size_t hashCode() const noexcept;

        /**
         * Returns a string representation of the current object.
         *
         * @return A string representation
         */
        std::string toString() const override;

        friend std::ostream &operator<<(std::ostream &os, const Attribute &a) {
            return os << a.toString();
        }

        friend std::ostream &operator<<(std::ostream &os, const Attribute::Ptr &a) {
            return os << a->toString();
        }
    };

    /**
     * Describes single value of single character of CFI code as defined in the ISO 10962 standard.
     */
    struct DXFCPP_EXPORT Value : RequireMakeShared<Value> {
        /// The alias to a type of shared pointer to the Value object
        using Ptr = std::shared_ptr<Value>;

        /// The alias to a type of unique pointer to the Value object
        using Unique = std::unique_ptr<Value>;

        private:
        mutable JavaObjectHandle<Value> handle_;

        public:
        Value(LockExternalConstructionTag, JavaObjectHandle<Value> &&handle);

        ~Value() noexcept override;

        /**
         * Returns an attribute that contains this value.
         *
         * @return The attribute.
         */
        Attribute::Ptr getAttribute() const;

        /**
         * Returns single UTF16 character code of this value.
         *
         * @return UTF16 character code.
         */
        std::int16_t getCode() const;

        /**
         * Returns single ASCII character code of this value.
         *
         * @return ASCII character code.
         */
        char getCodeChar() const;

        /**
         * Returns a short name of this value.
         *
         * @return The short name.
         */
        std::string getName() const;

        /**
         * Returns description of this value.
         *
         * @return The description.
         */
        std::string getDescription() const;

        /**
         * Returns `true` if this object is equal to `other` object
         *
         * @param other Another object
         * @return `true` if this object is equal to `other` object
         */
        bool operator==(const Value &other) const;

        /**
         * Returns `true` if this object is equal to `other` object
         *
         * @param other Another object
         * @return `true` if this object is equal to `other` object
         */
        bool operator==(const Value::Ptr &other) const {
            return *this == *other;
        }

        /**
         * @return A hash code value for this object.
         */
        std::size_t hashCode() const noexcept;

        /**
         * Returns a string representation of the current object.
         *
         * @return A string representation
         */
        std::string toString() const override;

        friend std::ostream &operator<<(std::ostream &os, const Value &v) {
            return os << v.toString();
        }

        friend std::ostream &operator<<(std::ostream &os, const Value::Ptr &v) {
            return os << v->toString();
        }
    };
};

DXFCPP_END_NAMESPACE

template <> struct std::hash<dxfcpp::CFI> {
    std::size_t operator()(const dxfcpp::CFI &cfi) const noexcept {
        return cfi.hashCode();
    }
};

template <> struct std::hash<dxfcpp::CFI::Attribute> {
    std::size_t operator()(const dxfcpp::CFI::Attribute &a) const noexcept {
        return a.hashCode();
    }
};

template <> struct std::hash<dxfcpp::CFI::Value> {
    std::size_t operator()(const dxfcpp::CFI::Value &v) const noexcept {
        return v.hashCode();
    }
};

DXFCXX_DISABLE_MSC_WARNINGS_POP()