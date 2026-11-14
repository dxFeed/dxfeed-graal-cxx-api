* **\[MDAPI-480]\[C++]** `DXEndpoint` tells why a connection fails (an experimental API, it may change in future
  versions).
    * `DXEndpoint::getLastConnectionError()` returns the newest connection error of the endpoint as
      `ConnectionError`: the time, the address, the category (`DNS`, `CONNECT_FAILED`, `TLS`, `CLOSED_BY_REMOTE`,
      `HEARTBEAT_TIMEOUT` and others), whether an established connection dropped, the Java exception, its message and
      the number of errors.
    * `DXEndpoint::getConnectors()` returns a snapshot of each message connector as `MessageConnectorInfo`: the state,
      the address, the addresses connected now, the numbers of the open and the closed connections, the statistics and
      the last error.
    * `DXEndpoint::addMessageConnectorListener()` adds a listener called after a connector changes its state, with the
      error of the change already recorded; `DXEndpoint::removeMessageConnectorListener()` removes it.
    * The `Connect` tool prints each new connection error to the standard error stream.
    * The new documentation page "Connection troubleshooting" describes these functions and the usual causes of each
      category.
