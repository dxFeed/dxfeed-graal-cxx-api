# Copyright (c) 2025 ttldtor.
# SPDX-License-Identifier: BSL-1.0

from conan import ConanFile

class bitsConan(ConanFile):
    name = "bits"
    version = "0.2.0"
    license = "BSL-1.0"
    author = "ttldtor"
    url = "https://github.com/ttldtor/bits"
    description = "Library for bit manipulation"
    topics = ("header-only", "bits", "cpp20")
    no_copy_source = True
    exports_sources = "include/*", "CMakeLists.txt"

    def package(self):
        self.copy("*.hpp", dst="include", src="include")

    def package_info(self):
        self.cpp_info.includedirs = ["include"]
