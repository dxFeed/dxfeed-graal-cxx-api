* **\[MDAPI-511]\[C++]** **\[BREAKING]** The Graal Native SDK 3.16.0 is the minimum version: the configuration fails
  with an older `DXFEED_GRAAL_NATIVE_SDK_VERSION`, also with `DXFG_LOCAL_ARCHIVE_PATH`.
    * Fixed: TLS connections trusted certificate authorities that are no longer trusted. The SDK is built with
      GraalVM 25.5 and trusts the root CAs of its JDK, so it no longer trusts the eMudhra and Comodo roots that
      JDK 25.0.5 removed and rejects the eMudhra TLS CAs G3B that it blocks (CVE-2026-91403, CVE-2026-114820). A
      connection to a server whose certificate chains up to one of them now fails the TLS handshake, unless the
      application sets `javax.net.ssl.trustStore`. See the
      [Graal Native SDK release notes](https://github.com/dxFeed/dxfeed-graal-native-sdk/releases/tag/v3.15.0).
    * macOS x86-64: the Graal isolate installs its handlers of `SIGPIPE` and `SIGXFSZ`, as on the other platforms
      (SDK 3.14.0+, MDAPI-500, SDK defect MDAPI-501): a write to a closed socket by a connector of an endpoint fails
      with `EPIPE` instead of killing the process.
