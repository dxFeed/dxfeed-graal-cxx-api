* **\[MDAPI-32]\[C++]** Strings passed to the dxFeed Graal SDK are checked for valid UTF-8.
    * A stray byte, a truncated or overlong sequence, an encoded surrogate or a value above U+10FFFF throw
      `InvalidArgumentException` naming the function, the parameter, the byte offset and the bytes of the invalid
      sequence, for example `DXEndpoint::connect(address): invalid UTF-8 at byte 12: 0xC3 0x28`. For a user name, a
      password, a token and the values of the `dxfeed.user` and `dxfeed.password` properties the message names the
      offset only.
    * `System::setProperty` and `System::getProperty` throw instead of cutting a key or a value to its longest valid
      prefix; `dxfc_system_set_property` and `dxfc_system_get_property` return `DXFC_EC_ERROR`.
    * The C functions `dxfc_dxendpoint_user`, `dxfc_dxendpoint_password`, `dxfc_dxendpoint_connect`,
      `dxfc_dxendpoint_builder_with_name`, `dxfc_dxendpoint_builder_with_property`,
      `dxfc_dxendpoint_builder_with_properties` and `dxfc_dxendpoint_builder_supports_property` return
      `DXFC_EC_ERROR` instead of letting an exception out.
    * `DXEndpoint::Builder::withProperty` checks the key and the value before it stores them: on invalid UTF-8 the
      builder does not change.
    * A property from `dxfeed.system.properties` or a `DXFEED_*` environment variable that is not valid UTF-8 is not
      set.
    * The string fields and the symbols of events are checked when the events are published
      (`DXPublisher::publishEvents`, writing to a tape); the message names the event type and the field, for example
      `TimeAndSale::exchangeSaleConditions: invalid UTF-8 at byte 1: 0xFF`. The setters of events do not check.
    * Symbols are checked when they go to a subscription (`DXFeedSubscription::addSymbols` and the other functions with
      symbols): `StringSymbol(symbol)`, `CandleSymbol(symbol)`, `IndexedEventSource(name)`.
    * `fillGraalData` of the event types, `SymbolWrapper::toGraal`, `SymbolWrapper::toGraalUnique`,
      `EventSourceWrapper::toGraal`, `EventSourceWrapper::toGraalUnique` and `IndexedEventSource::toGraalUnique` are no
      longer `noexcept`.
