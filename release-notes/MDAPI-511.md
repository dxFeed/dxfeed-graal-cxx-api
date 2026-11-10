* **\[MDAPI-511]\[C++]** **\[BREAKING]** The Graal Native SDK 3.15.0 is the minimum version: the configuration fails
  with an older `DXFEED_GRAAL_NATIVE_SDK_VERSION`, also with `DXFG_LOCAL_ARCHIVE_PATH`.
    * The SDK is built with GraalVM 25.5 and trusts the root CAs of its JDK: a connection to a server whose
      certificate chains up to a root that JDK 25.0.5 removed fails the TLS handshake, unless the application sets
      `javax.net.ssl.trustStore`. See the
      [Graal Native SDK release notes](https://github.com/dxFeed/dxfeed-graal-native-sdk/releases/tag/v3.15.0).
