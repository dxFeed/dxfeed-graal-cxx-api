# dxFeed Native API

![Build status](https://github.com/dxFeed/dxfeed-native-api/actions/workflows/build.yml/badge.svg?branch=main)

A project repository that is a dxFeed API written in C++ and provides a C\Obj-C\C++ interface for using the dxFeed Java
API features via dxfeed-graal-native-api as a backend

# TBD

- [x] Endpoint entity
  - [x] C++ API
  - [x] C API
- [ ] DXFeed entity
  - [ ] C++ API
    - [x] getInstance, attachSub*, detachSub*, detachSubAndClear, createSub
    - [ ] getLast*, getIndexed*, getTimeSeries*
  - [ ] C API
- [ ] DXPublisher entity
- [ ] Subscription entity
  - [ ] C++ API
    - [x] constructors, create, attach, detach, close, clear, isClosed
    - [ ] etc
  - [ ] C API
- [ ] Native events mapper
- [x] System properties processing
  - [x] C++ API
  - [x] C API
- [ ] Obj-C artifacts (also for the Swift's FFI)
- [ ] C\C++ samples
- [ ] CI\CD
  - [x] Build
  - [ ] Tests
    - [x] System properties processing
      - [x] C++ API
      - [x] C API 
    - [x] Endpoint entity
      - [x] C++ API
      - [x] C API     
  - [ ] Packaging
- [ ] Docs
  - [x] System properties processing
    - [x] C++ API
    - [x] C API
  - [x] Endpoint entity
    - [x] C++ API
    - [x] C API
  - [ ] DXFeed entity
  - [ ] Subscription entity
    - [ ] C++ API
      - [x] constructors, create, attach, detach, close, clear, isClosed
      - [ ] etc
    - [ ] C API

# License

[SPDX-License-Identifier: MPL-2.0](LICENSE)

[THIRD PARTY LICENSES](THIRD_PARTY_LICENSES)