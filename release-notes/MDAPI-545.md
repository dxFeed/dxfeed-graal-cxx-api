* **\[MDAPI-545]\[C++]** The files of a release can be verified: each archive has an SBOM next to it, and the release
  has the SHA-256 checksums of its files and a build provenance attestation (see "Verifying a download" in the README).
    * The SBOM of an archive is `<name>.cdx.json` (CycloneDX 1.6): the C++ libraries of the archive, the Graal Native
      SDK with the GraalVM and the JDK that built it and the Java libraries of the SDK, with their versions and
      licenses.
    * `SHA256SUMS` has the checksums of the archives and the SBOMs: `sha256sum -c SHA256SUMS --ignore-missing`.
    * `gh attestation verify <file> --repo dxFeed/dxfeed-graal-cxx-api` checks that the file was built by the release
      workflow of this repository.
