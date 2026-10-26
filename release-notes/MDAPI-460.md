* **\[MDAPI-460]\[C++]** **\[BREAKING]** Added `ErrorManager` (`ErrorManager.hpp`) for the errors without a caller: an
  exception in a callback of the dxFeed Graal SDK no longer terminates the process. See the "Error handling" page.
    * The callbacks of the SDK (the events of a subscription, the state of an endpoint or of an instrument profile
      connection, a change of a subscription, the instrument profiles of a collector, the events of a model, the log
      messages) catch the exceptions and report them to `ErrorManager`.
    * `ErrorManager` passes each error (`ErrorInfo`: component, source location, message, the class and the stack trace
      of a Java exception) to the handler and keeps the last ones in a journal: `getErrors()`, `getLastError()`,
      `clearErrors()`, `setCapacity()` (128 by default). The default handler writes one line to `stderr`;
      `setHandler()` replaces it, a null handler turns the output off, `resetHandler()` restores it. The handler is
      called on the thread of the error, possibly on several threads at once.
    * An exception of a listener of `IndexedTxModel`, ignored before, now goes to `ErrorManager` (and to `stderr` by
      default).
    * An event of a type that the C++ API does not implement (`CONFIGURATION`, for example) no longer stops the
      delivery: the event is skipped, the other events of the batch are delivered, and one error per type and batch is
      reported with the number of the skipped events. A listener of a subscription is not called for a batch whose
      events are all skipped.
    * `JavaException` got `getJavaClassName()`, `getJavaMessage()` and `getJavaStackTrace()`.
    * **\[BREAKING]** Removed `ErrorHandlingManager` and `Error` (`internal/managers/ErrorHandlingManager.hpp`): they
      were not used. Use `ErrorManager`.
