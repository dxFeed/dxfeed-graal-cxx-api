// Copyright (c) 2025 ttldtor.
// SPDX-License-Identifier: BSL-1.0

/**
 * @file
 * @brief Library for bit manipulation.
 * @author ttldtor
 */

#pragma once

#include <concepts>
#include <limits>
#include <type_traits>

/**
 * @defgroup bits Library for bit manipulation.
 */
/**@{*/

namespace org::ttldtor::bits {

namespace detail {
template <typename T, typename...>
struct MaxImpl {
  using Type = T;
};

template <typename T, typename U, typename... Ts>
struct MaxImpl<T, U, Ts...> {
  using Type = MaxImpl<std::conditional_t<sizeof(T) >= sizeof(U), T, U>, Ts...>::Type;
};
}  // namespace detail

/**
 * @ingroup bits
 * Returns max type by size (first is better)
 */
template <typename... Ts>
using Max = detail::MaxImpl<Ts...>::Type;

/**
 * @ingroup bits
 * Performs a right arithmetic bit shift operation (`>>` in Java, C, etc.). The sign bit is extended to preserve the
 * signedness of the number.
 *
 * The result of the shift will be of the same type as the `value` being shifted.
 * If the shift is a negative number of bits, then a @ref ::sal() "left arithmetic shift" will be performed.
 * If the shift size is greater than or equal to the number of bits in the shifted `value`, then, if the `value` is
 * negative (a signed integer type), `-1` will be returned, and if positive, then `0` will be returned.
 *
 * @tparam ValueType The type of `value`
 * @tparam ShiftType The type of `shift`
 * @param value The value to be shifted.
 * @param shift The shift in bits
 * @return The shifted `value`
 */
template <std::integral ValueType, std::integral ShiftType>
constexpr ValueType sar(ValueType value, ShiftType shift) noexcept;

/**
 * @ingroup bits
 * Performs a left arithmetic bit shift operation (`sal`, `<<` in Java, C, etc.). The `shift` is unsigned.
 *
 * The result of the shift will be of the same type as the `value` being shifted.
 * If the shift size is greater than or equal to the number of bits in the shifted `value`, then `0` will be
 * returned.
 *
 * @tparam ValueType The type of `value`
 * @tparam UnsignedShiftType The type of `shift`
 * @param value The value to be shifted
 * @param shift The shift in bits
 * @return The shifted `value`
 */
template <std::integral ValueType, std::unsigned_integral UnsignedShiftType>
constexpr ValueType leftArithmeticShift(ValueType value, UnsignedShiftType shift) noexcept {
  using UnsignedValueType = std::make_unsigned_t<std::remove_cv_t<ValueType>>;
  constexpr auto MAX_VALUE_BITS = static_cast<UnsignedShiftType>(std::numeric_limits<UnsignedValueType>::digits);

  if (shift < MAX_VALUE_BITS) {
    return static_cast<ValueType>(static_cast<UnsignedValueType>(value) << shift);
  }

  return ValueType{0};
}

/**
 * @ingroup bits
 * Performs a left arithmetic bit shift operation (`sal`, `<<` in Java, C, etc.). The `shift` is signed.
 *
 * The result of the shift will be of the same type as the `value` being shifted.
 * If the shift is a negative number of bits, then a @ref ::sar() "right arithmetic shift" will be performed.
 * If the shift size is greater than or equal to the number of bits in the shifted `value`, then `0` will be
 * returned.
 *
 * @tparam ValueType The type of `value`
 * @tparam SignedShiftType The type of `shift`
 * @param value The value to be shifted
 * @param shift The shift in bits
 * @return The shifted `value`
 */
template <std::integral ValueType, std::signed_integral SignedShiftType>
constexpr ValueType leftArithmeticShift(ValueType value, SignedShiftType shift) noexcept {
  using UnsignedShiftType = std::make_unsigned_t<std::remove_cv_t<SignedShiftType>>;

  if (shift < 0) {
    const UnsignedShiftType magnitude = UnsignedShiftType{0} - static_cast<UnsignedShiftType>(shift);

    return sar(value, magnitude);
  }

  using UnsignedValueType = std::make_unsigned_t<std::remove_cv_t<ValueType>>;
  constexpr auto MAX_VALUE_BITS = static_cast<UnsignedShiftType>(std::numeric_limits<UnsignedValueType>::digits);
  const auto unsignedShift = static_cast<UnsignedShiftType>(shift);

  if (unsignedShift < MAX_VALUE_BITS) {
    return static_cast<ValueType>(static_cast<UnsignedValueType>(value) << unsignedShift);
  }

  return ValueType{0};
}

/**
 * @ingroup bits
 * Performs a left arithmetic bit shift operation (`<<` in Java, C, etc.).
 *
 * The result of the shift will be of the same type as the `value` being shifted.
 * If the shift is a negative number of bits, then a @ref ::sar() "right arithmetic shift" will be performed.
 * If the shift size is greater than or equal to the number of bits in the shifted `value`, then `0` will be
 * returned.
 *
 * @tparam ValueType The type of `value`
 * @tparam ShiftType The type of `shift`
 * @param value The value to be shifted.
 * @param shift The shift in bits
 * @return The shifted `value`
 */
template <std::integral ValueType, std::integral ShiftType>
constexpr ValueType sal(ValueType value, ShiftType shift) noexcept {
  return leftArithmeticShift(value, shift);
}

/**
 * @ingroup bits
 * Performs a right arithmetic bit shift operation (`sar`, `>>` in Java, C, etc.). The `shift` is unsigned.
 * The sign bit is extended to preserve the signedness of the number.
 *
 * The result of the shift will be of the same type as the `value` being shifted.
 * If the shift size is greater than or equal to the number of bits in the shifted `value`, then, if the `value` is
 * negative (a signed integer type), `-1` will be returned, and if positive, then `0` will be returned.
 *
 * @tparam ValueType The type of `value`
 * @tparam UnsignedShiftType The type of `shift`
 * @param value The value to be shifted
 * @param shift The shift in bits
 * @return The shifted `value`
 */
template <std::integral ValueType, std::unsigned_integral UnsignedShiftType>
constexpr ValueType rightArithmeticShift(ValueType value, UnsignedShiftType shift) noexcept {
  using UnsignedValueType = std::make_unsigned_t<std::remove_cv_t<ValueType>>;
  constexpr auto MAX_VALUE_BITS = static_cast<UnsignedShiftType>(std::numeric_limits<UnsignedValueType>::digits);

  if (shift < MAX_VALUE_BITS) {
    return static_cast<ValueType>(value >> shift);
  }

  return value < 0 ? static_cast<ValueType>(-1) : ValueType{0};
}

/**
 * @ingroup bits
 * Performs a right arithmetic bit shift operation (`sar`, `>>` in Java, C, etc.). The `shift` is signed.
 * The sign bit is extended to preserve the signedness of the number.
 *
 * The result of the shift will be of the same type as the `value` being shifted.
 * If the shift is a negative number of bits, then a @ref ::sal() "left arithmetic shift" will be performed.
 * If the shift size is greater than or equal to the number of bits in the shifted `value`, then, if the `value` is
 * negative (a signed integer type), `-1` will be returned, and if positive, then `0` will be returned.
 *
 * @tparam ValueType The type of `value`
 * @tparam SignedShiftType The type of `shift`
 * @param value The value to be shifted
 * @param shift The shift in bits
 * @return The shifted `value`
 */
template <std::integral ValueType, std::signed_integral SignedShiftType>
constexpr ValueType rightArithmeticShift(ValueType value, SignedShiftType shift) noexcept {
  using UnsignedShiftType = std::make_unsigned_t<std::remove_cv_t<SignedShiftType>>;

  if (shift < 0) {
    const UnsignedShiftType magnitude = UnsignedShiftType{0} - static_cast<UnsignedShiftType>(shift);

    return sal(value, magnitude);
  }

  using UnsignedValueType = std::make_unsigned_t<std::remove_cv_t<ValueType>>;
  constexpr auto MAX_VALUE_BITS = static_cast<UnsignedShiftType>(std::numeric_limits<UnsignedValueType>::digits);
  const auto unsignedShift = static_cast<UnsignedShiftType>(shift);

  if (unsignedShift < MAX_VALUE_BITS) {
    return static_cast<ValueType>(value >> unsignedShift);
  }

  return value < 0 ? static_cast<ValueType>(-1) : ValueType{0};
}

/**
 * @ingroup bits
 * Performs a right arithmetic bit shift operation (`>>` in Java, C, etc.). The sign bit is extended to preserve the
 * signedness of the number.
 *
 * The result of the shift will be of the same type as the `value` being shifted.
 * If the shift is a negative number of bits, then a @ref ::sal() "left arithmetic shift" will be performed.
 * If the shift size is greater than or equal to the number of bits in the shifted `value`, then, if the `value` is
 * negative (a signed integer type), `-1` will be returned, and if positive, then `0` will be returned.
 *
 * @tparam ValueType The type of `value`
 * @tparam ShiftType The type of `shift`
 * @param value The value to be shifted.
 * @param shift The shift in bits
 * @return The shifted `value`
 */
template <std::integral ValueType, std::integral ShiftType>
constexpr ValueType sar(ValueType value, ShiftType shift) noexcept {
  return rightArithmeticShift(value, shift);
}

/**
 * @ingroup bits
 * Performs a right logical bit shift operation (`>>>` in Java). Fills the left bits by zero.
 *
 * The result of the shift will be of the same type as the `value` being shifted.
 * If the shift is a negative number of bits, then a @ref ::shl() "left logical shift" will be performed.
 * If the shift size is greater than or equal to the number of bits in the shifted `value`, then `0` will be
 * returned.
 *
 * @tparam ValueType The type of `value`
 * @tparam ShiftType The type of `shift`
 * @param value The value to be shifted.
 * @param shift The shift in bits
 * @return The shifted `value`
 */
template <std::integral ValueType, std::integral ShiftType>
constexpr ValueType shr(ValueType value, ShiftType shift) noexcept;

/**
 * @ingroup bits
 * Performs a left logical bit shift operation (`shl`, `<<<`). The `shift` is unsigned.
 *
 * The result of the shift will be of the same type as the `value` being shifted.
 * If the shift is a negative number of bits, then a @ref ::shr() "right logical shift" will be performed.
 * If the shift size is greater than or equal to the number of bits in the shifted `value`, then `0` will be
 * returned.
 *
 * @tparam ValueType The type of `value`
 * @tparam UnsignedShiftType The type of `shift`
 * @param value The value to be shifted
 * @param shift The shift in bits
 * @return The shifted `value`
 */
template <std::integral ValueType, std::unsigned_integral UnsignedShiftType>
constexpr ValueType leftLogicalShift(ValueType value, UnsignedShiftType shift) noexcept {
  return leftArithmeticShift(value, shift);
}

/**
 * @ingroup bits
 * Performs a left logical bit shift operation (`shl`, `<<<`). The `shift` is signed.
 *
 * The result of the shift will be of the same type as the `value` being shifted.
 * If the shift is a negative number of bits, then a @ref ::shr() "right logical shift" will be performed.
 * If the shift size is greater than or equal to the number of bits in the shifted `value`, then `0` will be
 * returned.
 *
 * @tparam ValueType The type of `value`
 * @tparam SignedShiftType The type of `shift`
 * @param value The value to be shifted
 * @param shift The shift in bits
 * @return The shifted `value`
 */
template <std::integral ValueType, std::integral SignedShiftType>
constexpr ValueType leftLogicalShift(ValueType value, SignedShiftType shift) noexcept {
  using UnsignedShiftType = std::make_unsigned_t<std::remove_cv_t<SignedShiftType>>;

  if (shift < 0) {
    const UnsignedShiftType magnitude = UnsignedShiftType{0} - static_cast<UnsignedShiftType>(shift);

    return shr(value, magnitude);
  }

  using UnsignedValueType = std::make_unsigned_t<std::remove_cv_t<ValueType>>;
  constexpr auto MAX_VALUE_BITS = static_cast<UnsignedShiftType>(std::numeric_limits<UnsignedValueType>::digits);
  const auto unsignedShift = static_cast<UnsignedShiftType>(shift);

  if (unsignedShift < MAX_VALUE_BITS) {
    return static_cast<ValueType>(static_cast<UnsignedValueType>(value) << unsignedShift);
  }

  return ValueType{0};
}

/**
 * @ingroup bits
 * Performs a left logical bit shift operation (`shl`, `<<<`).
 *
 * The result of the shift will be of the same type as the `value` being shifted.
 * If the shift is a negative number of bits, then a @ref ::shr() "right logical shift" will be performed.
 * If the shift size is greater than or equal to the number of bits in the shifted `value`, then `0` will be
 * returned.
 *
 * @tparam ValueType The type of `value`
 * @tparam ShiftType The type of `shift`
 * @param value The value to be shifted.
 * @param shift The shift in bits
 * @return The shifted `value`
 */
template <std::integral ValueType, std::integral ShiftType>
constexpr ValueType shl(ValueType value, ShiftType shift) noexcept {
  return leftLogicalShift(value, shift);
}

/**
 * @ingroup bits
 * Performs a right logical bit shift operation (`shr`, `>>>` in Java). The `shift` is unsigned. Fills the left bits by
 * zero.
 *
 * The result of the shift will be of the same type as the `value` being shifted.
 * If the shift is a negative number of bits, then a @ref ::shl() "left logical shift" will be performed.
 * If the shift size is greater than or equal to the number of bits in the shifted `value`, then `0` will be
 * returned.
 *
 * @tparam ValueType The type of `value`
 * @tparam UnsignedShiftType The type of `shift`
 * @param value The value to be shifted
 * @param shift The shift in bits
 * @return The shifted `value`
 */
template <std::integral ValueType, std::unsigned_integral UnsignedShiftType>
constexpr ValueType rightLogicalShift(ValueType value, UnsignedShiftType shift) noexcept {
  using UnsignedValueType = std::make_unsigned_t<std::remove_cv_t<ValueType>>;
  constexpr auto MAX_VALUE_BITS = static_cast<UnsignedShiftType>(std::numeric_limits<UnsignedValueType>::digits);

  if (shift < MAX_VALUE_BITS) {
    return static_cast<ValueType>(static_cast<UnsignedValueType>(value) >> shift);
  }

  return ValueType{0};
}

/**
 * @ingroup bits
 * Performs a right logical bit shift operation (`shr`, `>>>` in Java). The `shift` is signed. Fills the left bits by
 * zero.
 *
 * The result of the shift will be of the same type as the `value` being shifted.
 * If the shift is a negative number of bits, then a @ref ::shl() "left logical shift" will be performed.
 * If the shift size is greater than or equal to the number of bits in the shifted `value`, then `0` will be
 * returned.
 *
 * @tparam ValueType The type of `value`
 * @tparam SignedShiftType The type of `shift`
 * @param value The value to be shifted
 * @param shift The shift in bits
 * @return The shifted `value`
 */
template <std::integral ValueType, std::signed_integral SignedShiftType>
constexpr ValueType rightLogicalShift(ValueType value, SignedShiftType shift) noexcept {
  using UnsignedShiftType = std::make_unsigned_t<std::remove_cv_t<SignedShiftType>>;

  if (shift < 0) {
    const UnsignedShiftType magnitude = UnsignedShiftType{0} - static_cast<UnsignedShiftType>(shift);

    return shl(value, magnitude);
  }

  using UnsignedValueType = std::make_unsigned_t<std::remove_cv_t<ValueType>>;
  constexpr auto MAX_VALUE_BITS = static_cast<UnsignedShiftType>(std::numeric_limits<UnsignedValueType>::digits);
  const auto unsignedShift = static_cast<UnsignedShiftType>(shift);

  if (unsignedShift < MAX_VALUE_BITS) {
    return static_cast<ValueType>(static_cast<UnsignedValueType>(value) >> shift);
  }

  return ValueType{0};
}

/**
 * @ingroup bits
 * Performs a right logical bit shift operation (`shr`, `>>>` in Java). Fills the left bits by zero.
 *
 * The result of the shift will be of the same type as the `value` being shifted.
 * If the shift is a negative number of bits, then a @ref ::shl() "left logical shift" will be performed.
 * If the shift size is greater than or equal to the number of bits in the shifted `value`, then `0` will be
 * returned.
 *
 * @tparam ValueType The type of `value`
 * @tparam ShiftType The type of `shift`
 * @param value The value to be shifted.
 * @param shift The shift in bits
 * @return The shifted `value`
 */
template <std::integral ValueType, std::integral ShiftType>
constexpr ValueType shr(ValueType value, ShiftType shift) noexcept {
  return rightLogicalShift(value, shift);
}

/**
 * @ingroup bits
 * Performs a bitwise AND operation between two values of possibly different types
 * and ensures the result is cast back to the type of the first argument.
 *
 * @tparam FirstType The type of the first argument.
 * @tparam SecondType The type of the second argument.
 * @param first The first operand of type `FirstType`.
 * @param second The second operand of type `SecondType`.
 * @return The result of the bitwise AND operation, cast to type `FirstType`.
 */
template <std::integral FirstType, std::integral SecondType>
constexpr FirstType andOp(FirstType first, SecondType second) noexcept {
  using MaxUnsignedType = std::make_unsigned_t<Max<FirstType, SecondType>>;

  return static_cast<FirstType>(static_cast<MaxUnsignedType>(first) & static_cast<MaxUnsignedType>(second));
}

/**
 * @ingroup bits
 * Performs a bitwise OR operation between two values of possibly different types
 * and ensures the result is cast back to the type of the first argument.
 *
 * @tparam FirstType The type of the first argument.
 * @tparam SecondType The type of the second argument.
 * @param first The first operand of type `FirstType`.
 * @param second The second operand of type `SecondType`.
 * @return The result of the bitwise OR operation, cast to type `FirstType`.
 */
template <std::integral FirstType, std::integral SecondType>
constexpr FirstType orOp(FirstType first, SecondType second) noexcept {
  using MaxUnsignedType = std::make_unsigned_t<Max<FirstType, SecondType>>;

  return static_cast<FirstType>(static_cast<MaxUnsignedType>(first) | static_cast<MaxUnsignedType>(second));
}

/**
 * @ingroup bits
 * Performs a bitwise XOR operation between two values of possibly different types
 * and ensures the result is cast back to the type of the first argument.
 *
 * @tparam FirstType The type of the first argument.
 * @tparam SecondType The type of the second argument.
 * @param first The first operand of type `FirstType`.
 * @param second The second operand of type `SecondType`.
 * @return The result of the bitwise XOR operation, cast to type `FirstType`.
 */
template <std::integral FirstType, std::integral SecondType>
constexpr FirstType xorOp(FirstType first, SecondType second) noexcept {
  using MaxUnsignedType = std::make_unsigned_t<Max<FirstType, SecondType>>;

  return static_cast<FirstType>(static_cast<MaxUnsignedType>(first) ^ static_cast<MaxUnsignedType>(second));
}

/**
 * @ingroup bits
 * Determines if the specified bits are set in the source value.
 *
 * @tparam T the type of arguments.
 * @param sourceBits The value to check for the presence of specific bits.
 * @param bitMaskToCheck The mask of bits to check against the source value.
 * @return `true` if the specified bits are set in the source value.
 */
template <std::unsigned_integral T>
constexpr bool bitsAreSet(T sourceBits, T bitMaskToCheck) {
  return andOp(sourceBits, bitMaskToCheck) != 0;
}

/**
 * @ingroup bits
 * Determines if the specified bits are set in the source value.
 *
 * @tparam SourceBitsType The type of the source bits (e.g., an integer type).
 * @tparam BitMaskType The type of the bit mask (e.g., an integer type).
 * @param sourceBits The value to check for the presence of specific bits.
 * @param bitMaskToCheck The mask of bits to check against the source value.
 * @return `true` if the specified bits are set in the source value.
 */
template <std::integral SourceBitsType, std::integral BitMaskType>
constexpr bool bitsAreSet(SourceBitsType sourceBits, BitMaskType bitMaskToCheck) {
  using MaxType = Max<SourceBitsType, BitMaskType>;

  if constexpr (std::is_signed_v<SourceBitsType> || std::is_signed_v<BitMaskType>) {
    using MaxUnsignedType = std::make_unsigned_t<MaxType>;

    return bitsAreSet(static_cast<MaxUnsignedType>(sourceBits), static_cast<MaxUnsignedType>(bitMaskToCheck));
  } else {
    return bitsAreSet(static_cast<MaxType>(sourceBits), static_cast<MaxType>(bitMaskToCheck));
  }
}

/**
 * @ingroup bits
 * Sets specific bits in the source value using a bitmask.
 *
 * @tparam T The type of arguments.
 * @param sourceBits The original bits to modify.
 * @param bitMaskToSet The bitmask representing the bits to be set.
 * @return The resulting value after setting the specified bits.
 */
template <std::unsigned_integral T>
constexpr T setBits(T sourceBits, T bitMaskToSet) {
  return orOp(sourceBits, bitMaskToSet);
}

/**
 * @ingroup bits
 * Sets specific bits in the source value using a bitmask.
 *
 * @tparam SourceBitsType The type of the source bits (e.g., an integer type).
 * @tparam BitMaskType The type of the bit mask (e.g., an integer type).
 * @param sourceBits The original bits to modify.
 * @param bitMaskToSet The bitmask representing the bits to be set.
 * @return The resulting value after setting the specified bits.
 */
template <std::integral SourceBitsType, std::integral BitMaskType>
constexpr SourceBitsType setBits(SourceBitsType sourceBits, BitMaskType bitMaskToSet) {
  using MaxType = Max<SourceBitsType, BitMaskType>;

  if constexpr (std::is_signed_v<SourceBitsType> || std::is_signed_v<BitMaskType>) {
    using MaxUnsignedType = std::make_unsigned_t<MaxType>;

    return static_cast<SourceBitsType>(
      setBits(static_cast<MaxUnsignedType>(sourceBits), static_cast<MaxUnsignedType>(bitMaskToSet)));
  } else {
    return static_cast<SourceBitsType>(setBits(static_cast<MaxType>(sourceBits), static_cast<MaxType>(bitMaskToSet)));
  }
}

/**
 * @ingroup bits
 * Resets (clears) specific bits in a bitmask.
 *
 * @tparam T The type of the source bits and the bitmask. Should be an integral type.
 * @param sourceBits The original bitmask containing the bits to modify.
 * @param bitMaskToReset The bitmask specifying the bits to reset.
 * @return The resulting value after resetting the specified bits.
 */
template <std::unsigned_integral T>
constexpr T resetBits(T sourceBits, T bitMaskToReset) {
  return andOp(sourceBits, ~bitMaskToReset);
}

/**
 * @ingroup bits
 * Resets (clears) specific bits in a bitmask.
 *
 * @tparam SourceBitsType The type of the source bits (e.g., an integer type).
 * @tparam BitMaskType The type of the bit mask (e.g., an integer type).
 * @param sourceBits The original bitmask containing the bits to modify.
 * @param bitMaskToReset The bitmask specifying the bits to reset.
 * @return The resulting value after resetting the specified bits.
 */
template <std::integral SourceBitsType, std::integral BitMaskType>
// ReSharper disable once CppDFAConstantParameter
constexpr SourceBitsType resetBits(SourceBitsType sourceBits, BitMaskType bitMaskToReset) {
  using MaxType = Max<SourceBitsType, BitMaskType>;

  if constexpr (std::is_signed_v<SourceBitsType> || std::is_signed_v<BitMaskType>) {
    using MaxUnsignedType = std::make_unsigned_t<MaxType>;

    return static_cast<SourceBitsType>(
      resetBits(static_cast<MaxUnsignedType>(sourceBits), static_cast<MaxUnsignedType>(bitMaskToReset)));
  } else {
    return static_cast<SourceBitsType>(
      resetBits(static_cast<MaxType>(sourceBits), static_cast<MaxType>(bitMaskToReset)));
  }
}

}  // namespace org::ttldtor::bits

/**@}*/