// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../../include/dxfeed_graal_cpp_api/api.hpp"

namespace dxfcpp::tools {

/// The scheme of the Tools process: the system properties (`opt:sysprops`), the properties of the endpoint builders and
/// the `-p` options of the tools (`opt:dxprops`), and the dxLink protocol (`resource:dxlink.xml`). Without
/// `opt:dxprops` the scheme ignores the properties of the builders, such as `dxscheme.nanoTime`.
inline constexpr auto TOOLS_SCHEME = "ext:opt:sysprops,opt:dxprops,resource:dxlink.xml";

/// Sets the system properties of the Tools process before the first endpoint: the wildcard subscription, the dxLink
/// protocol and its scheme. The scheme is fixed at the first endpoint of the process.
inline void setToolsSystemProperties() {
    System::setProperty(DXEndpoint::DXFEED_WILDCARD_ENABLE_PROPERTY, "true");
    // Enable experimental feature.
    System::setProperty("dxfeed.experimental.dxlink.enable", "true");
    // Set scheme for dxLink.
    System::setProperty("scheme", TOOLS_SCHEME);
}

} // namespace dxfcpp::tools
