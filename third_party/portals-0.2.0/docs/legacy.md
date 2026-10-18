# Minimal legacy compatibility

Portals 0.2.0 preserves the small streaming API used by the original
dxFeed `MarketDepthModelSample`. It is a migration boundary, separate from
the recommended [managed and declarative UI](declarative-ui.md).

```cpp
#include <portals/legacy.hpp>
#include <iostream>

namespace otp = portals::legacy;

int main() {
  const auto enabled = otp::VirtualTerminal::tryEnable();

  if (!enabled) {
    return 1; // Inspect enabled.error() for the native failure.
  }

  std::cout << otp::hideCursor()
            << otp::moveTo(5, 0) << "Q | NTV | NSDQ | 340.19 | 10"
            << otp::moveTo(5, 39) << "||"
            << otp::moveTo(5, 41) << "Q | NTV | NSDQ | 340.34 | 1000"
            << otp::showCursor() << std::flush;
}
```

`moveTo(line, column)` returns an owning VT `ESC[line;columnH` string. VT
coordinates are normally **one-based**, unlike the retained core's
`CellPoint`. A zero argument is emitted unchanged, as in the old API;
VT terminals treat it as one. There is no addition, clamping, or overflow
caused by coordinate conversion.

`hideCursor()` and `showCursor()` return owning strings. They do not write
or flush output. In the original sample, the returned cursor strings were
ignored, so these calls had no visible effect. To correct that sample,
insert the strings into the output stream and flush after each complete
update. Serialize entire updates and cleanup with the sample's I/O mutex.

## Building the original source

The installed `<portals.hpp>` header exposes only these compatibility names:

```cpp
#include <portals.hpp>

namespace ot = org::ttldtor;
namespace otp = org::ttldtor::portals;

// Original source call shapes remain valid:
// ot::VirtualTerminal::enable();
// otp::moveTo(5, 0) + formattedText;
// otp::hideCursor();
// otp::showCursor();
```

Link the installed `portals::core` target and use C++23. No separate runtime,
renderer, scheduler, router, or reactive system is started. The umbrella
header does **not** restore the entire 0.1.x library, bit utilities, colors,
or unrelated escape helpers. New compatibility consumers should use
`<portals/legacy.hpp>` and `portals::legacy` directly.

## Output modes and lifetime

`VirtualTerminal::enable()` preserves its historical bool result. On Windows,
it enables processed output and VT processing on the existing stdout handle, preserving
other console mode bits. Redirected/non-console output fails. It neither
opens a console nor changes stdin mode, so `std::cin.get()` remains cooked.
`tryEnable()` provides the same operation with a typed `TerminalError`.
An absent stdout returns `INVALID_OPTIONS` with native code zero; failed
Windows API calls return `SYSTEM` with their original error code.
On non-Windows platforms both forms succeed without changing modes; this is
legacy behavior, not a terminal-capability check.

The helper is stateless and does not restore console mode or cursor state.
The caller owns flushing, synchronization and exception-safe cleanup; in
particular, showing the cursor only on the successful exit path is insufficient.
Do not interleave these stream writes with an active `Application`: its
terminal/frame writer owns output exclusively. For an interactive application
with managed cleanup and themes, migrate to the regular UI API instead.

The Windows mode semantics follow
[SetConsoleMode](https://learn.microsoft.com/en-us/windows/console/setconsolemode);
the emitted sequences follow
[Console virtual terminal sequences](https://learn.microsoft.com/en-us/windows/console/console-virtual-terminal-sequences).
