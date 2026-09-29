// Copyright (c) 2025 ttldtor.
// SPDX-License-Identifier: BSL-1.0

/**
 * @file colormanip.cpp
 * @brief Example demonstrating color manipulation using the `bits` library
 *
 * This example shows how to use the `bits` library for:
 * - Packing color components into a single integer
 * - Unpacking color components from an integer
 * - Manipulating individual color channels
 * - Converting between different color formats
 */

#include <bits/bits.hpp>
#include <cstdint>
#include <iomanip>
#include <iostream>

using namespace org::ttldtor::bits;

/**
 * @brief Represents an RGBA color with individual components
 */
struct Color {
  std::uint8_t red;    ///< Red channel (0-255)
  std::uint8_t green;  ///< Green channel (0-255)
  std::uint8_t blue;   ///< Blue channel (0-255)
  std::uint8_t alpha;  ///< Alpha channel (0-255, 0=transparent, 255=opaque)

  /**
   * @brief Packs color components into a 32-bit ARGB value
   * @return 32-bit integer in ARGB format `(Alpha << 24 | Red << 16 | Green << 8 | Blue)`
   *
   * <pre>
   * Layout: [AAAAAAAA][RRRRRRRR][GGGGGGGG][BBBBBBBB]
   *    bits    24-31    16-23      8-15       0-7
   * </pre>
   */
  [[nodiscard]] std::uint32_t pack() const {
    std::uint32_t result = 0;

    // Shift alpha to bits 24-31 and OR with the result
    result = orOp(result, shl(static_cast<std::uint32_t>(alpha), 24));

    // Shift red to bits 16-23 and OR with the result
    result = orOp(result, shl(static_cast<std::uint32_t>(red), 16));

    // Shift green to bits 8-15 and OR with the result
    result = orOp(result, shl(static_cast<std::uint32_t>(green), 8));

    // Blue occupies bits 0-7, no shift needed
    result = orOp(result, static_cast<std::uint32_t>(blue));

    return result;
  }

  /**
   * @brief Unpacks a 32-bit ARGB value into color components
   * @param packed 32-bit integer in ARGB format
   * @return Color struct with unpacked components
   */
  [[nodiscard]] static Color unpack(std::uint32_t packed) {
    Color color{};

    // Extract alpha from bits 24-31
    color.alpha = static_cast<std::uint8_t>(shr(packed, 24));

    // Extract red from bits 16-23
    color.red = static_cast<std::uint8_t>(shr(packed, 16));

    // Extract green from bits 8-15
    color.green = static_cast<std::uint8_t>(shr(packed, 8));

    // Extract blue from bits 0-7 (no shift needed, just mask)
    color.blue = static_cast<std::uint8_t>(packed);

    return color;
  }

  /**
   * @brief Modifies the alpha channel of a packed color
   * @param packed Original packed color value
   * @param alpha New alpha value (0-255)
   * @return New packed color with an updated alpha channel
   */
  [[nodiscard]] static std::uint32_t setAlpha(std::uint32_t packed, std::uint8_t alpha) {
    // Clear the alpha channel (bits 24-31)
    packed = resetBits(packed, 0xFF000000u);

    // Set a new alpha value
    return setBits(packed, shl(static_cast<std::uint32_t>(alpha), 24));
  }

  /**
   * @brief Modifies the red channel of a packed color
   * @param packed Original packed color value
   * @param red New red value (0-255)
   * @return New packed color with an updated red channel
   */
  [[nodiscard]] static std::uint32_t setRed(std::uint32_t packed, std::uint8_t red) {
    // Clear the red channel (bits 16-23)
    packed = resetBits(packed, 0x00FF0000u);

    // Set a new red value
    return setBits(packed, shl(static_cast<std::uint32_t>(red), 16));
  }

  /**
   * @brief Modifies the green channel of a packed color
   * @param packed Original packed color value
   * @param green New green value (0-255)
   * @return New packed color with an updated green channel
   */
  [[nodiscard]] static std::uint32_t setGreen(std::uint32_t packed, std::uint8_t green) {
    // Clear the green channel (bits 8-15)
    packed = resetBits(packed, 0x0000FF00u);

    // Set a new green value
    return setBits(packed, shl(static_cast<std::uint32_t>(green), 8));
  }

  /**
   * @brief Modifies the blue channel of a packed color
   * @param packed Original packed color value
   * @param blue New blue value (0-255)
   * @return New packed color with an updated blue channel
   */
  [[nodiscard]] static std::uint32_t setBlue(std::uint32_t packed, std::uint8_t blue) {
    // Clear the blue channel (bits 0-7)
    packed = resetBits(packed, 0x000000FFu);

    // Set a new blue value
    return setBits(packed, static_cast<std::uint32_t>(blue));
  }

  /**
   * @brief Converts an ARGB format to RGBA format
   * @param argb Color in ARGB format (Alpha << 24 | Red << 16 | Green << 8 | Blue)
   * @return Color in RGBA format (Red << 24 | Green << 16 | Blue << 8 | Alpha)
   */
  [[nodiscard]] static std::uint32_t argbToRgba(std::uint32_t argb) {
    // Extract individual channels
    const auto alpha = static_cast<std::uint8_t>(shr(argb, 24));
    const auto red = static_cast<std::uint8_t>(shr(argb, 16));
    const auto green = static_cast<std::uint8_t>(shr(argb, 8));
    const auto blue = static_cast<std::uint8_t>(argb);

    // Repack in RGBA format
    std::uint32_t rgba = 0;
    rgba = orOp(rgba, shl(static_cast<std::uint32_t>(red), 24));
    rgba = orOp(rgba, shl(static_cast<std::uint32_t>(green), 16));
    rgba = orOp(rgba, shl(static_cast<std::uint32_t>(blue), 8));
    rgba = orOp(rgba, static_cast<std::uint32_t>(alpha));

    return rgba;
  }

  /**
   * @brief Blends two colors using alpha blending
   * @param foreground Foreground color (with alpha)
   * @param background Background color
   * @return Blended color
   *
   * Formula: `result = foreground * alpha + background * (1 - alpha)`
   */
  [[nodiscard]] static Color blend(const Color& foreground, const Color& background) {
    // Convert alpha from 0-255 to 0.0-1.0 range
    const float alpha = static_cast<float>(foreground.alpha) / 255.0f;
    const float invAlpha = 1.0f - alpha;

    Color result{};
    result.red = static_cast<std::uint8_t>(static_cast<float>(foreground.red) * alpha +
                                           static_cast<float>(background.red) * invAlpha);
    result.green = static_cast<std::uint8_t>(static_cast<float>(foreground.green) * alpha +
                                             static_cast<float>(background.green) * invAlpha);
    result.blue = static_cast<std::uint8_t>(static_cast<float>(foreground.blue) * alpha +
                                            static_cast<float>(background.blue) * invAlpha);
    result.alpha = 255;  // Result is fully opaque

    return result;
  }

  /**
   * @brief Checks if the color has an alpha channel (not fully opaque)
   * @param packed Packed color value
   * @return true if alpha < 255
   */
  [[nodiscard]] static bool hasTransparency(std::uint32_t packed) {
    // Extract the alpha channel and check if it's less than 255
    const auto alpha = static_cast<std::uint8_t>(shr(packed, 24));

    return alpha < 255;
  }

  /**
   * @brief Inverts the color (excluding an alpha channel)
   * @param packed Original color
   * @return Inverted color
   */
  [[nodiscard]] static std::uint32_t invert(std::uint32_t packed) {
    // XOR with 0x00FFFFFF to invert RGB channels, leaving alpha unchanged
    return xorOp(packed, 0x00FFFFFFu);
  }

  /**
   * @brief Prints color information
   */
  void print() const {
    std::cout << "RGBA(" << static_cast<int>(red) << ", " << static_cast<int>(green) << ", " << static_cast<int>(blue)
              << ", " << static_cast<int>(alpha) << ")";
  }
};

int main() {
  std::cout << "=== Color Manipulation Example ===" << std::endl;
  std::cout << std::hex << std::uppercase << std::setfill('0');

  // Example 1: Pack and unpack color
  std::cout << "\n--- Example 1: Pack and Unpack ---" << std::endl;
  Color color1{255, 128, 64, 200};
  std::cout << "Original color: ";
  color1.print();
  std::cout << std::endl;

  std::uint32_t packed = color1.pack();
  std::cout << "Packed (ARGB): 0x" << std::setw(8) << packed << std::endl;

  Color unpacked = Color::unpack(packed);
  std::cout << "Unpacked color: ";
  unpacked.print();
  std::cout << std::endl;

  // Example 2: Modify individual channels
  std::cout << "\n--- Example 2: Modify Channels ---" << std::endl;
  std::uint32_t color2 = Color{100, 150, 200, 255}.pack();
  std::cout << "Original: 0x" << std::setw(8) << color2 << " -> ";
  Color::unpack(color2).print();
  std::cout << std::endl;

  color2 = Color::setRed(color2, 255);
  std::cout << "After setRed(255): 0x" << std::setw(8) << color2 << " -> ";
  Color::unpack(color2).print();
  std::cout << std::endl;

  color2 = Color::setAlpha(color2, 128);
  std::cout << "After setAlpha(128): 0x" << std::setw(8) << color2 << " -> ";
  Color::unpack(color2).print();
  std::cout << std::endl;

  // Example 3: Format conversion
  std::cout << "\n--- Example 3: ARGB to RGBA ---" << std::endl;
  std::uint32_t argb = 0xC8FF8040u;  // A=200, R=255, G=128, B=64
  std::cout << "ARGB: 0x" << std::setw(8) << argb << std::endl;

  std::uint32_t rgba = Color::argbToRgba(argb);
  std::cout << "RGBA: 0x" << std::setw(8) << rgba << std::endl;

  // Example 4: Color blending
  std::cout << "\n--- Example 4: Alpha Blending ---" << std::endl;
  Color foreground{255, 0, 0, 128};  // Semi-transparent red
  Color background{0, 0, 255, 255};  // Opaque blue

  std::cout << "Foreground: ";
  foreground.print();
  std::cout << std::endl;

  std::cout << "Background: ";
  background.print();
  std::cout << std::endl;

  Color blended = Color::blend(foreground, background);
  std::cout << "Blended: ";
  blended.print();
  std::cout << std::endl;

  // Example 5: Transparency check
  std::cout << "\n--- Example 5: Transparency Check ---" << std::endl;
  const std::uint32_t opaqueColor = Color{255, 255, 255, 255}.pack();
  const std::uint32_t transparentColor = Color{255, 255, 255, 200}.pack();

  std::cout << "Opaque color (0x" << std::setw(8) << opaqueColor
            << ") has transparency: " << (Color::hasTransparency(opaqueColor) ? "YES" : "NO") << std::endl;
  std::cout << "Transparent color (0x" << std::setw(8) << transparentColor
            << ") has transparency: " << (Color::hasTransparency(transparentColor) ? "YES" : "NO") << std::endl;

  // Example 6: Color inversion
  std::cout << "\n--- Example 6: Color Inversion ---" << std::endl;

  constexpr Color original{255, 128, 64, 255};

  std::cout << "Original: 0x" << std::setw(8) << original.pack() << " -> ";
  original.print();
  std::cout << std::endl;

  const std::uint32_t inverted = Color::invert(original.pack());
  std::cout << "Inverted: 0x" << std::setw(8) << inverted << " -> ";
  Color::unpack(inverted).print();
  std::cout << std::endl;

  // Example 7: Grayscale conversion
  std::cout << "\n--- Example 7: Grayscale Conversion ---" << std::endl;

  constexpr Color colorful{200, 100, 50, 255};

  std::cout << "Colorful: ";
  colorful.print();
  std::cout << std::endl;

  // Convert to grayscale using weighted average
  constexpr auto gray =
    static_cast<std::uint8_t>(0.299f * colorful.red + 0.587f * colorful.green + 0.114f * colorful.blue);

  constexpr Color grayscale{gray, gray, gray, colorful.alpha};

  std::cout << "Grayscale: ";
  grayscale.print();
  std::cout << std::endl;

  return 0;
}