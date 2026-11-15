* **\[MDAPI-469]\[C++]** Tools: the `Diagnose <address> <types> <symbols>` command checks the way to the data step by
  step and prints a verdict.
    * It prints the versions (API, Graal Native SDK, platform, QD), resolves each host of the address to its IP
      addresses and tries a TCP connect to each, prints the states of the endpoint with their times and the connection
      errors, and for `--duration` seconds (10 by default) collects the events: the first ones of each type, their
      flags, the number per type, the time of the last event per symbol and the receive time minus the event time.
    * The verdict is `DATA`, `PARTIAL DATA`, `CONNECTED, NO DATA` (with the hint "likely not entitled or not in the
      endpoint filter; check the endpoint IPF"), `CANNOT CONNECT` (with the last connection error) or
      `CANNOT RESOLVE`; the exit code is 0 for `DATA`, 2 to 5 for the others and 1 for an error.
    * `--compare <address2>` checks a second address after the first one and compares the two.
    * `--report <file>` writes the output to a file. Logins, passwords, tokens and `user:password@` are hidden in the
      output and in the file.
    * The README names `Diagnose` as the first step when there is no data.
