# Security Policy

## Supported Versions

Security fixes are made in the next release of the library. Only the latest release is supported: before you report,
check that the problem is still there in it.

## Reporting a Vulnerability

Please do not report security vulnerabilities in public issues, discussions or pull requests.

Report them privately through GitHub:
[Report a vulnerability](https://github.com/dxFeed/dxfeed-graal-cxx-api/security/advisories/new)
(the **Security** tab of the repository → **Report a vulnerability**). Only you and the maintainers see the report.

Please include:

- the version of the library (and of the Graal Native SDK, if you changed it), the platform and the compiler;
- what an attacker can do and under which conditions;
- the steps or a minimal program that reproduce the problem;
- a fix or a workaround, if you have one.

## What Happens Next

1. We confirm that we received the report and ask questions in the report itself, if needed.
2. If the problem is confirmed, we prepare a fix in private and agree with you on the date of the disclosure.
3. We release the fix and publish a security advisory, with a CVE where it applies. We credit you in the advisory,
   unless you ask us not to.

If the problem is not a vulnerability, we tell you why and close the report; a plain bug can then go to a public issue.

## Scope

In scope: the C++ API, the C API, the Tools and the samples of this repository, and the way this repository builds
and publishes its releases.

The library runs on the [Graal Native SDK](https://github.com/dxFeed/dxfeed-graal-native-sdk), which contains GraalVM
and the Java libraries of dxFeed. Report problems in the SDK here as well: we forward them to the team of the SDK.

Not a vulnerability:

- a crash or a leak caused by a call that the documentation forbids (for example, a null pointer where one is not
  allowed, or the use of an object after it is closed);
- the demo addresses, demo credentials and test data in the samples and the documentation;
- a finding of an automated scanner without a shown way to exploit it.
