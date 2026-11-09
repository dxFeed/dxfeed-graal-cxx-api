* **\[MDAPI-493]\[C++]** **\[BREAKING]** Functions that take a string take `dxfcpp::StringParam` instead of
  `const dxfcpp::StringLike &`: a pointer and a length that does not own or copy the characters.
    * `StringParam` accepts a string literal, a `char` array, `const char *` and `char *` (a null pointer is an empty
      string), `std::string`, `std::string_view` and contiguous ranges of `char` (`std::vector<char>`,
      `std::array<char, N>`, `std::span<const char>`). Numbers, `bool`, `std::optional`, wide and UTF-8 strings and
      types that convert to `std::string` only by their own operator do not compile.
    * A `char` array is read up to its first NUL, but not past its end. It used to be read as `const char *`, up to a
      NUL that could lie past the array.
    * `StringLike` is removed. `String::EMPTY`, `String::NUL` and `StringHash` moved to
      `dxfeed_graal_cpp_api/util/StringUtils.hpp`, the `OptionalString` concept to
      `dxfeed_graal_cpp_api/detail/Concepts.hpp`; `dxfeed_graal_cpp_api/api.hpp` includes both.
    * The constructors and setters of events, `IndexedEventSource` and `SessionFilter` that store a string are no
      longer `noexcept`: they copy the string. `TradeETH(std::string)` is `explicit TradeETH(StringParam)`.
    * Overrides of virtual functions that take a string (`EventType::setEventSymbol`, `Order::withEventSymbol`,
      `Order::withMarketMaker` and the like) take `StringParam` and drop `noexcept`.
    * The exported symbols of the library change: rebuild applications.
