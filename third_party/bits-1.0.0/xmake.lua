-- Copyright (c) 2025 ttldtor.
-- SPDX-License-Identifier: BSL-1.0
add_rules("mode.debug", "mode.release")

target("bits")
    set_kind("headeronly")
    add_includedirs("include", {public = true})
    set_languages("c++20")

option("tests", {default = true, showmenu = true})

if has_config("tests") then
    add_requires("doctest")
    add_requires("nanobench")

    target("bits_tests")
        set_kind("binary")
        add_files("tests/main.cpp", "tests/bits.cpp")
        set_languages("c++20")
        add_packages("doctest")
        add_deps("bits")
    target("bits_bench")
        set_kind("binary")
        add_files("tests/main.cpp", "tests/bench.cpp")
        set_languages("c++20")
        add_packages("doctest", "nanobench")
        add_deps("bits")
end
