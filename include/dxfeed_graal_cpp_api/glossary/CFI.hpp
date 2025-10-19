// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../internal/Common.hpp"
#include "../internal/Conf.hpp"
#include "../internal/JavaObjectHandle.hpp"

DXFCPP_BEGIN_NAMESPACE

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

    static Ptr valueOf(const StringLikeWrapper &code);

    static Ptr valueOf(std::int32_t intCode);

    std::string getCode() const;

    std::int32_t getIntCode() const;

    std::int16_t getCategory() const;

    char getCategoryChar() const;

    std::int16_t getGroup() const;

    char getGroupChar() const;

    bool isEquity() const;

    bool isDebtInstrument() const;

    bool isEntitlement() const;

    bool isOption() const;

    bool isFuture() const;

    bool isOther() const;

    struct Value;

    std::vector<std::shared_ptr<Value>> decipher() const;

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

        std::string getName() const;

        std::string getDescription() const;

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

        Attribute::Ptr getAttribute() const;

        std::int16_t getCode() const;

        char getCodeChar() const;

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