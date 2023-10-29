// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../internal/Conf.hpp"

namespace dxfcpp {

/**
 * A filter for days used by various search methods.
 * This class provides predefined filters for certain Day attributes,
 * although users can create their own filters to suit their needs.
 * <p>
 * Please note that days can be either trading or non-trading, and this distinction can be
 * either based on rules (e.g. weekends) or dictated by special occasions (e.g. holidays).
 * Different filters treat this distinction differently - some accept only trading days,
 * some only non-trading, and some ignore type of day altogether.
 */
struct DXFCPP_EXPORT DayFilter {

};

}