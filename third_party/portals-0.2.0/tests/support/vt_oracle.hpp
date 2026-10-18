// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0
#pragma once

#include <portals/cell_buffer.hpp>

#include <algorithm>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace test_support {
// Independent small VT machine for the encoder's supported grammar. It does not
// call encodeDiff, the full renderer, a grapheme decoder, or encoding helpers.
// Unicode width rules here cover only the explicit fixture alphabet (CJK, emoji,
// ASCII, combining acute and the three narrow chart brushes); this is deliberately
// not a general VT product.
class VtOracle {
public:
  /** @brief Owning semantic cell; continuation repeats the lead glyph/style. */
  struct Pixel {
    std::string glyph{" "};                         ///< Complete fixture grapheme bytes.
    portals::CellStyle style;                       ///< Independent style value.
    int width{1};                                   ///< One/two for leads, zero for continuation.
    bool tail{};                                    ///< Continuation of the preceding wide lead.
    bool operator==(const Pixel &) const = default; ///< Value equality.
  };

  /** @brief Half-open horizontal range of one complete grapheme. */
  struct Span {
    int row{};                                     ///< Row containing the grapheme.
    int begin{};                                   ///< Inclusive lead column.
    int end{};                                     ///< Exclusive column after the grapheme.
    bool operator==(const Span &) const = default; ///< Value equality.
  };

  /** @brief Owning semantic screen checkpoint, independent of subsequent mutations. */
  struct Snapshot {
    portals::CellSize size;                            ///< Viewport dimensions.
    std::vector<Pixel> cells;                          ///< Row-major cell values.
    std::vector<Span> spans;                           ///< One range per lead, in row-major order.
    portals::CellPoint cursor;                         ///< Cursor in cell coordinates.
    portals::CellStyle cursor_style;                   ///< Style applied by subsequent text.
    bool autowrap{};                                   ///< Whether right-edge output enables wrapping.
    bool wrap_pending{};                               ///< Whether the next spacing glyph wraps.
    bool operator==(const Snapshot &) const = default; ///< Value equality.
  };

  static constexpr std::size_t MAX_CELLS = 16384;     ///< Includes existing large native writer fixtures.
  static constexpr std::size_t MAX_DIMENSION = 4096;  ///< Bound each coordinate independently.
  static constexpr std::size_t MAX_GLYPH_BYTES = 256; ///< Bound combining sequences.
  static constexpr std::size_t MAX_CSI_BYTES = 64;    ///< Bound incomplete parameter storage.

  /** @brief Validate dimensions and product before allocating; return the cell count. */
  static std::size_t validateSize(int width, int height) {
    if (width < 0 || height < 0 || static_cast<std::size_t>(width) > MAX_DIMENSION ||
        static_cast<std::size_t>(height) > MAX_DIMENSION ||
        (height != 0 && static_cast<std::size_t>(width) > MAX_CELLS / static_cast<std::size_t>(height))) {
      throw std::runtime_error("oracle: viewport limit exceeded");
    }

    return static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
  }

  /** @brief Capture cells, spans, style and cursor, without encoder helpers. */
  Snapshot snapshot() const {
    Snapshot result{{width_, height_}, cells_, {}, {x_, y_}, style_, wrap_, wrap_pending_};

    for (int y = 0; y < height_; ++y) {
      for (int x = 0; x < width_; ++x) {
        const auto &pixel = cells_[index(x, y)];

        if (!pixel.tail) {
          result.spans.push_back({y, x, x + pixel.width});
        }
      }
    }

    return result;
  }

  VtOracle(int width, int height) : width_(width), height_(height), cells_(validateSize(width, height)) {
  }

  void resize(int width, int height) {
    std::vector<Pixel> resized(validateSize(width, height));

    for (int y = 0; y < std::min(height_, height); ++y) {
      for (int x = 0; x < std::min(width_, width); ++x) {
        const auto &pixel = cells_[index(x, y)];

        if (!pixel.tail && x + pixel.width > width) {
          continue;
        }

        resized[static_cast<std::size_t>(y * width + x)] = pixel;
      }
    }

    width_ = width;
    height_ = height;
    cells_ = std::move(resized);
    x_ = 0;
    y_ = 0;
    wrap_pending_ = false;
    last_lead_ = -1;
  }

  void feed(std::string_view bytes) {
    for (const auto byte : bytes) {
      const auto ch = static_cast<unsigned char>(byte);

      if (ch == 0x18) {
        state_ = State::GROUND;
        csi_.clear();
        unicode_.clear();
        needed_ = 0;

        continue;
      }

      if (state_ == State::ESCAPE) {
        if (ch != '[') {
          throw std::runtime_error("oracle: unsupported escape");
        }

        state_ = State::CSI;

        continue;
      }

      if (state_ == State::CSI) {
        if (ch >= 0x40 && ch <= 0x7e) {
          control(static_cast<char>(ch));
          csi_.clear();
          state_ = State::GROUND;
        } else {
          if (csi_.size() == MAX_CSI_BYTES) {
            throw std::runtime_error("oracle: CSI limit exceeded");
          }

          csi_.push_back(static_cast<char>(ch));
        }

        continue;
      }

      if (ch == 0x1b) {
        needed_ = 0;
        unicode_.clear();
        state_ = State::ESCAPE;

        continue;
      }

      if (needed_ != 0) {
        if ((ch & 0xc0) != 0x80) {
          throw std::runtime_error("oracle: broken UTF-8");
        }

        scalar_ = (scalar_ << 6) | (ch & 0x3f);
        unicode_.push_back(static_cast<char>(ch));
        --needed_;

        if (needed_ == 0) {
          if (scalar_ < minimum_scalar_ || scalar_ > 0x10ffff || (scalar_ >= 0xd800 && scalar_ <= 0xdfff)) {
            throw std::runtime_error("oracle: invalid UTF-8 scalar");
          }

          put(scalar_, unicode_);
        }

        continue;
      }

      if (ch < 0x80) {
        if (ch < 0x20) {
          throw std::runtime_error("oracle: unsupported control");
        }

        put(ch, std::string(1, static_cast<char>(ch)));
      } else {
        unicode_.assign(1, static_cast<char>(ch));

        if ((ch & 0xe0) == 0xc0) {
          needed_ = 1;
          scalar_ = ch & 0x1f;
          minimum_scalar_ = 0x80;
        } else if ((ch & 0xf0) == 0xe0) {
          needed_ = 2;
          scalar_ = ch & 0x0f;
          minimum_scalar_ = 0x800;
        } else if ((ch & 0xf8) == 0xf0) {
          needed_ = 3;
          scalar_ = ch & 0x07;
          minimum_scalar_ = 0x10000;
        } else {
          throw std::runtime_error("oracle: invalid UTF-8 lead");
        }
      }
    }
  }

  bool equals(const portals::CellBuffer &frame) const {
    if (frame.size() != portals::CellSize{width_, height_}) {
      return false;
    }

    for (int y = 0; y < height_; ++y) {
      for (int x = 0; x < width_; ++x) {
        const auto actual = frame.at({x, y});
        const auto &pixel = cells_[index(x, y)];

        if (!actual || pixel.glyph != actual->glyph || pixel.style != actual->style ||
            pixel.width != actual->cell.columns ||
            pixel.tail != (actual->cell.kind == portals::CellKind::CONTINUATION)) {
          return false;
        }
      }
    }

    return true;
  }

  bool neutral() const {
    return x_ == 0 && y_ == 0 && wrap_ && !wrap_pending_ && style_ == portals::CellStyle{} && state_ == State::GROUND &&
           needed_ == 0;
  }

  int scrolls() const {
    return scrolls_;
  }

  const Pixel &at(int x, int y) const {
    return cells_[index(x, y)];
  }

private:
  enum class State {
    GROUND,
    ESCAPE,
    CSI
  };

  std::size_t index(int x, int y) const {
    if (x < 0 || y < 0 || x >= width_ || y >= height_) {
      throw std::runtime_error("oracle: cell coordinate outside viewport");
    }

    return static_cast<std::size_t>(y * width_ + x);
  }

  void clearGlyph(int x, int y) {
    auto i = index(x, y);

    if (cells_[i].tail) {
      --i;
    }

    const bool wide = cells_[i].width == 2;

    cells_[i] = {};

    if (wide) {
      cells_[i + 1] = {};
    }
  }

  void put(std::uint32_t scalar, const std::string &bytes) {
    if (scalar == 0x301) {
      if (last_lead_ < 0) {
        throw std::runtime_error("oracle: unattached accent");
      }

      auto &lead = cells_[static_cast<std::size_t>(last_lead_)];

      if (bytes.size() > MAX_GLYPH_BYTES - lead.glyph.size()) {
        throw std::runtime_error("oracle: glyph byte limit exceeded");
      }

      lead.glyph += bytes;

      if (lead.width == 2) {
        cells_[static_cast<std::size_t>(last_lead_) + 1].glyph = lead.glyph;
      }

      return;
    }

    if (!((scalar >= 0x20 && scalar <= 0x7e) || scalar == 0x2501 || scalar == 0x2502 || scalar == 0x2588 ||
          (scalar >= 0x4e00 && scalar <= 0x9fff) || (scalar >= 0x1f300 && scalar <= 0x1faff))) {
      throw std::runtime_error("oracle: scalar outside fixture alphabet");
    }

    const int columns = ((scalar >= 0x4e00 && scalar <= 0x9fff) || (scalar >= 0x1f300 && scalar <= 0x1faff)) ? 2 : 1;

    if (width_ == 0 || height_ == 0) {
      throw std::runtime_error("oracle: write to empty viewport");
    }

    if (wrap_pending_ && wrap_) {
      x_ = 0;
      ++y_;
      wrap_pending_ = false;

      if (y_ == height_) {
        ++scrolls_;
        y_ = height_ - 1;
        std::rotate(cells_.begin(), cells_.begin() + width_, cells_.end());
        std::fill(cells_.end() - width_, cells_.end(), Pixel{});
      }
    }

    if (x_ + columns > width_) {
      throw std::runtime_error("oracle: split wide glyph");
    }

    clearGlyph(x_, y_);

    if (columns == 2) {
      clearGlyph(x_ + 1, y_);
    }

    const auto i = index(x_, y_);

    cells_[i] = {bytes, style_, columns, false};

    if (columns == 2) {
      cells_[i + 1] = {bytes, style_, 0, true};
    }

    last_lead_ = static_cast<int>(i);
    x_ += columns;

    if (x_ >= width_) {
      x_ = width_ - 1;
      wrap_pending_ = wrap_;
    }
  }

  void control(char final) {
    const bool private_mode = !csi_.empty() && csi_[0] == '?';
    std::vector<int> values;
    int number = 0;

    for (std::size_t i = private_mode ? 1 : 0; i < csi_.size(); ++i) {
      if (csi_[i] == ';') {
        values.push_back(number);
        number = 0;
      } else if (csi_[i] >= '0' && csi_[i] <= '9') {
        const auto digit = csi_[i] - '0';

        if (number > (std::numeric_limits<int>::max() - digit) / 10) {
          throw std::runtime_error("oracle: CSI numeric overflow");
        }

        number = number * 10 + digit;
      } else {
        throw std::runtime_error("oracle: invalid CSI parameter");
      }
    }

    values.push_back(number);

    if (final == 'H' && !private_mode && values.size() <= 2) {
      y_ = std::max(1, values[0]) - 1;
      x_ = values.size() > 1 ? std::max(1, values[1]) - 1 : 0;

      if ((height_ > 0 && y_ >= height_) || (width_ > 0 && x_ >= width_) || (height_ == 0 && y_ != 0) ||
          (width_ == 0 && x_ != 0)) {
        throw std::runtime_error("oracle: cursor outside viewport");
      }

      wrap_pending_ = false;
      last_lead_ = -1;
    } else if (final == 'J' && !private_mode && values.size() == 1 && values[0] == 2) {
      std::fill(cells_.begin(), cells_.end(), Pixel{" ", style_, 1, false});
    } else if (final == 'm' && !private_mode) {
      for (std::size_t i = 0; i < values.size(); ++i) {
        const auto code = values[i];

        if (code == 0) {
          style_ = {};
        } else if (code == 1) {
          style_.bold = true;
        } else if (code == 3) {
          style_.italic = true;
        } else if (code == 4) {
          style_.underline = true;
        } else if ((code >= 30 && code <= 37) || (code >= 40 && code <= 47) || (code >= 90 && code <= 97) ||
                   (code >= 100 && code <= 107)) {
          // Decode standard ANSI channels independently of the production nearest-color encoder.
          const bool background = (code >= 40 && code <= 47) || code >= 100;
          const bool bright = code >= 90;
          const auto index = code - (bright ? (background ? 100 : 90) : (background ? 40 : 30));
          const auto low = static_cast<std::uint8_t>(bright ? 85 : 0);
          const auto high = static_cast<std::uint8_t>(bright ? 255 : 170);
          portals::RgbColor color{index & 1 ? high : low, index & 2 ? high : low, index & 4 ? high : low};

          if (!bright && index == 3) {
            color.green = 85;
          }

          if (background) {
            style_.background = color;
          } else {
            style_.foreground = color;
          }
        } else if ((code == 38 || code == 48) && i + 4 < values.size() && values[i + 1] == 2) {
          if (values[i + 2] > 255 || values[i + 3] > 255 || values[i + 4] > 255) {
            throw std::runtime_error("oracle: RGB component outside range");
          }

          const portals::RgbColor rgb{static_cast<std::uint8_t>(values[i + 2]),
                                      static_cast<std::uint8_t>(values[i + 3]),
                                      static_cast<std::uint8_t>(values[i + 4])};

          if (code == 38) {
            style_.foreground = rgb;
          } else {
            style_.background = rgb;
          }

          i += 4;
        } else {
          throw std::runtime_error("oracle: unsupported SGR");
        }
      }
    } else if ((final == 'l' || final == 'h') && private_mode && values.size() == 1 && values[0] == 7) {
      wrap_ = final == 'h';
      wrap_pending_ = false;
    } else if (final == 'l' && values.size() == 1 &&
               ((private_mode && values[0] == 6) || (!private_mode && values[0] == 4))) {
      // Full viewport and replace mode are the oracle's fixed baseline.
    } else if (final == 'r' && csi_.empty()) {
      x_ = 0;
      y_ = 0;
      wrap_pending_ = false;
    } else {
      throw std::runtime_error("oracle: unsupported CSI");
    }
  }

  int width_;
  int height_;
  std::vector<Pixel> cells_;
  State state_{};
  std::string csi_;
  std::string unicode_;
  std::uint32_t scalar_{};
  std::uint32_t minimum_scalar_{};
  int needed_{};
  int x_{};
  int y_{};
  int last_lead_{-1};
  int scrolls_{};
  bool wrap_{true};
  bool wrap_pending_{};
  portals::CellStyle style_;
};
} // namespace test_support
