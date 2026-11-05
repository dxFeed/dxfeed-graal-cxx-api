# Copyright (c) 2026 Devexperts LLC.
# SPDX-License-Identifier: MPL-2.0

"""Tests of scripts/symbols.py: python3 -m unittest discover -s scripts/tests"""

import contextlib
import io
import pathlib
import platform
import shutil
import struct
import sys
import tempfile
import unittest
import unittest.mock

SCRIPTS = pathlib.Path(__file__).resolve().parent.parent
sys.path.insert(0, str(SCRIPTS))

import symbols  # noqa: E402
from symbols import SymbolsError  # noqa: E402

NM_ELF = """\
0000000000012340 T _ZN6dxfcpp5Quote10setBidSizeEd
0000000000012350 W _ZNSt6vectorIiSaIiEE17_M_realloc_insertIJRKiEEEvN9__gnu_cxx17__normal_iteratorIPiS1_EEDpOT_
0000000000412360 V _ZTIN6dxfcpp13JavaExceptionE@@SOMEVER
0000000000012370 T dxfc_system_set_property@SOMEVER
0000000000500000 B __bss_start
"""

NM_MACHO = """\
0000000000012340 T __ZN6dxfcpp5Quote10setBidSizeEd
0000000000012370 T _dxfc_system_set_property
"""


def pe_with_exports(names, pe32_plus=True):
    """A minimal PE file: the headers and one section with the export directory and the names."""
    section_rva, section_offset = 0x1000, 0x200
    optional_size = 240 if pe32_plus else 224
    directory = bytearray(40)
    name_table = section_rva + 40
    strings_rva = name_table + 4 * len(names)
    pointers = b""
    strings = b""
    for name in names:
        pointers += struct.pack("<I", strings_rva + len(strings))
        strings += name.encode("ascii") + b"\0"
    struct.pack_into("<I", directory, 24, len(names))
    struct.pack_into("<I", directory, 32, name_table)
    section = bytes(directory) + pointers + strings

    data = bytearray(section_offset + len(section))
    data[0:2] = b"MZ"
    struct.pack_into("<I", data, 0x3C, 0x40)
    data[0x40:0x44] = b"PE\0\0"
    coff = 0x44
    struct.pack_into("<H", data, coff + 2, 1)
    struct.pack_into("<H", data, coff + 16, optional_size)
    optional = coff + 20
    struct.pack_into("<H", data, optional, 0x20B if pe32_plus else 0x10B)
    struct.pack_into("<II", data, optional + (112 if pe32_plus else 96), section_rva, len(section))
    header = optional + optional_size
    data[header:header + 8] = b".edata\0\0"
    struct.pack_into("<IIII", data, header + 8, len(section), section_rva, len(section), section_offset)
    data[section_offset:] = section
    return bytes(data)


class EntityTest(unittest.TestCase):
    def test_function_with_return_type_and_templates(self):
        self.assertEqual(symbols.entity("std::pair<int, int> dxfcpp::f<std::pair<int, int> >(int) const"),
                         "dxfcpp::f<std::pair<int, int> >")

    def test_special_names(self):
        self.assertEqual(symbols.entity("typeinfo for dxfcpp::JavaException"), "dxfcpp::JavaException")
        self.assertEqual(symbols.entity("vtable for std::exception"), "std::exception")
        self.assertEqual(symbols.entity("non-virtual thunk to dxfcpp::A::~A()"), "dxfcpp::A::~A")

    def test_operators(self):
        self.assertEqual(symbols.entity("std::ostream& dxfcpp::operator<<(std::ostream&, dxfcpp::Quote const&)"),
                         "dxfcpp::operator<<")
        self.assertEqual(symbols.entity("bool dxfcpp::operator<(dxfcpp::A const&, dxfcpp::A const&)"),
                         "dxfcpp::operator<")
        self.assertEqual(symbols.entity("std::hash<dxfcpp::A>::operator()(dxfcpp::A const&) const"),
                         "std::hash<dxfcpp::A>::operator()")
        self.assertEqual(symbols.entity("dxfcpp::A::operator std::__cxx11::basic_string<char>() const"),
                         "dxfcpp::A::operator std::__cxx11::basic_string<char>")
        self.assertEqual(symbols.entity('dxfcpp::literals::operator"" _s(char const*, unsigned long)'),
                         'dxfcpp::literals::operator"" _s')
        self.assertEqual(symbols.entity("dxfcpp::operator_like(int)"), "dxfcpp::operator_like")
        self.assertEqual(symbols.entity("std::string std::operator+<char, std::char_traits<char> >(char const*, "
                                        "std::string const&)"),
                         "std::operator+<char, std::char_traits<char> >")
        self.assertEqual(symbols.entity("decltype(auto) std::__do_visit<int, std::operator< <dxfcpp::A, dxfcpp::B>"
                                        "(std::variant<dxfcpp::A, dxfcpp::B> const&)::{lambda(auto:1&&)#1}>(int)"),
                         "std::__do_visit<int, std::operator< <dxfcpp::A, dxfcpp::B>(std::variant<dxfcpp::A, "
                         "dxfcpp::B> const&)::{lambda(auto:1&&)#1}>")
        self.assertEqual(symbols.entity("bool dxfcpp::operator<<=(int)"), "dxfcpp::operator<<=")

    def test_msvc_names(self):
        self.assertEqual(symbols.entity("const dxfcpp::v8::RequireMakeShared<struct dxfcpp::v8::CFI>::`vftable'"),
                         "dxfcpp::v8::RequireMakeShared<struct dxfcpp::v8::CFI>::`vftable'")
        self.assertEqual(
            symbols.entity("class std::basic_ostream<char,struct std::char_traits<char> > & dxfcpp::operator<<"
                           "(class std::basic_ostream<char,struct std::char_traits<char> > &,int)"),
            "dxfcpp::operator<<")

    def test_c_function(self):
        self.assertEqual(symbols.entity("dxfc_system_set_property"), "dxfc_system_set_property")


class SplitTest(unittest.TestCase):
    def test_std_counted_own_listed_linker_dropped(self):
        own, std_total = symbols.split_symbols([
            "dxfcpp::Quote::setBidSize(double)",
            "void std::vector<int>::_M_realloc_insert<int const&>(int*, int const&)",
            "typeinfo for std::exception",
            "typeinfo for dxfcpp::JavaException",
            "__gnu_cxx::__normal_iterator<int*>::base() const",
            "dxfc_system_set_property",
            "_init",
            "dxfcpp::Quote::setBidSize(double)",
        ])
        self.assertEqual(own, {"dxfcpp::Quote::setBidSize(double)", "typeinfo for dxfcpp::JavaException",
                               "dxfc_system_set_property"})
        self.assertEqual(std_total, 3)


class NmTest(unittest.TestCase):
    def test_elf_versions_dropped(self):
        self.assertEqual(symbols.parse_nm(NM_ELF, "elf")[2:4],
                         ["_ZTIN6dxfcpp13JavaExceptionE", "dxfc_system_set_property"])

    def test_macho_underscore_dropped(self):
        self.assertEqual(symbols.parse_nm(NM_MACHO, "macho"),
                         ["_ZN6dxfcpp5Quote10setBidSizeEd", "dxfc_system_set_property"])

    @unittest.skipUnless(shutil.which("c++filt"), "no c++filt")
    def test_elf_end_to_end(self):
        own, std_total = symbols.split_symbols(symbols.demangle_itanium(symbols.parse_nm(NM_ELF, "elf")))
        self.assertEqual(own, {"dxfcpp::Quote::setBidSize(double)", "typeinfo for dxfcpp::JavaException",
                               "dxfc_system_set_property"})
        self.assertEqual(std_total, 1)


class PeTest(unittest.TestCase):
    NAMES = ["??0Quote@v8@dxfcpp@@QEAA@XZ", "?getCount@Candle@v8@dxfcpp@@QEBA_JXZ", "dxfc_system_set_property"]

    def test_export_names(self):
        self.assertEqual(symbols.pe_export_names(pe_with_exports(self.NAMES)), self.NAMES)
        self.assertEqual(symbols.pe_export_names(pe_with_exports(self.NAMES, pe32_plus=False)), self.NAMES)

    def test_broken(self):
        with self.assertRaises(SymbolsError):
            symbols.pe_export_names(b"MZ" + bytes(0x3A) + b"\x40\0\0\0" + b"XXXX")

    @unittest.skipUnless(platform.system() == "Windows", "dbghelp")
    def test_undecorate(self):
        # UnDecorateSymbolName ends a const member function with a space, which an editor would trim from the list.
        self.assertEqual(symbols.demangle_msvc(self.NAMES),
                         ["dxfcpp::v8::Quote::Quote(void)", "__int64 dxfcpp::v8::Candle::getCount(void)const",
                          "dxfc_system_set_property"])


class ListTest(unittest.TestCase):
    def test_render_parse_round_trip(self):
        own = {"dxfcpp::b()", "dxfcpp::a()"}
        text = symbols.render("linux", own, 7)
        self.assertIn("std-total: 7\ndxfcpp::a()\ndxfcpp::b()\n", text)
        self.assertEqual(symbols.parse_list(text), (own, 7))

    def test_parse_ignores_trailing_whitespace(self):
        self.assertEqual(symbols.parse_list("std-total: 7\r\ndxfcpp::a()const \r\n"), ({"dxfcpp::a()const"}, 7))

    def test_no_std_total_line(self):
        with self.assertRaises(SymbolsError):
            symbols.parse_list("# comment\ndxfcpp::a()\n")

    def test_compare(self):
        expected = ({"dxfcpp::a()", "dxfcpp::b()"}, 7)
        self.assertEqual(symbols.compare(expected, ({"dxfcpp::a()", "dxfcpp::b()"}, 7)), [])
        differences = symbols.compare(expected, ({"dxfcpp::a()", "dxfcpp::c()"}, 8))
        self.assertIn("-dxfcpp::b()", differences)
        self.assertIn("+dxfcpp::c()", differences)
        self.assertEqual(differences[-1], "std-total: list 7, library 8")
        self.assertEqual(symbols.compare(expected, ({"dxfcpp::a()", "dxfcpp::b()"}, 8), check_std_total=False), [])


class MainTest(unittest.TestCase):
    def run_main(self, *args):
        out, err = io.StringIO(), io.StringIO()
        with contextlib.redirect_stdout(out), contextlib.redirect_stderr(err):
            code = symbols.main(list(args))
        return code, out.getvalue(), err.getvalue()

    def test_missing_library(self):
        code, _, err = self.run_main("extract", "no-such-library.so")
        self.assertEqual(code, 2)
        self.assertIn("no such file", err)

    def test_not_a_library(self):
        with tempfile.TemporaryDirectory() as directory:
            path = pathlib.Path(directory, "text.so")
            path.write_text("text")
            code, _, err = self.run_main("extract", str(path))
        self.assertEqual(code, 2)
        self.assertIn("not an ELF, Mach-O or PE file", err)

    def test_no_exports(self):
        with tempfile.TemporaryDirectory() as directory:
            path = pathlib.Path(directory, "empty.dll")
            path.write_bytes(pe_with_exports([]))
            with unittest.mock.patch.object(symbols, "demangle_msvc", lambda names: names):
                code, _, err = self.run_main("extract", str(path))
        self.assertEqual(code, 2)
        self.assertIn("no exported symbols", err)

    def test_compare_update_and_mismatch(self):
        with tempfile.TemporaryDirectory() as directory:
            library = pathlib.Path(directory, "lib.dll")
            listed = pathlib.Path(directory, "abi", "symbols-windows.txt")
            library.write_bytes(pe_with_exports(["dxfc_a", "dxfc_b"]))
            with unittest.mock.patch.object(symbols, "demangle_msvc", lambda names: names):
                self.assertEqual(self.run_main("compare", str(library), str(listed))[0], 2)
                self.assertEqual(self.run_main("compare", "--update", str(library), str(listed))[0], 0)
                self.assertEqual(self.run_main("compare", str(library), str(listed))[0], 0)
                library.write_bytes(pe_with_exports(["dxfc_a", "dxfc_c"]))
                code, out, _ = self.run_main("compare", str(library), str(listed))
        self.assertEqual(code, 1)
        self.assertIn("-dxfc_b", out)
        self.assertIn("+dxfc_c", out)


if __name__ == "__main__":
    unittest.main()
