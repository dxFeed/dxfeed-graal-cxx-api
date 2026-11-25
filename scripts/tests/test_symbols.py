# Copyright (c) 2026 Devexperts LLC.
# SPDX-License-Identifier: MPL-2.0

"""Tests of scripts/symbols.py: python3 -m unittest discover -s scripts/tests"""

import contextlib
import io
import json
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
    NAMES = ["??0Quote@v8@dxfcpp@@QEAA@XZ", "?getCount@Candle@v8@dxfcpp@@QEBA_JXZ",
             "?setHandler@ErrorManager@v8@dxfcpp@@SAXV?$function@$$A6AXAEBUErrorInfo@v8@dxfcpp@@@Z@std@@@Z",
             "dxfc_system_set_property"]

    def test_export_names(self):
        self.assertEqual(symbols.pe_export_names(pe_with_exports(self.NAMES)), self.NAMES)
        self.assertEqual(symbols.pe_export_names(pe_with_exports(self.NAMES, pe32_plus=False)), self.NAMES)

    def test_broken(self):
        with self.assertRaises(SymbolsError):
            symbols.pe_export_names(b"MZ" + bytes(0x3A) + b"\x40\0\0\0" + b"XXXX")

    def test_ms_keywords(self):
        undecorated = ("class dxfcpp::S const & __ptr64 __cdecl dxfcpp::f(class std::function<void __cdecl(struct "
                       "dxfcpp::E const & __ptr64)>,char const * __ptr64)const __ptr64&")
        self.assertEqual(symbols.MS_KEYWORDS.sub("", undecorated),
                         "class dxfcpp::S const & dxfcpp::f(class std::function<void (struct dxfcpp::E const &)>,"
                         "char const *)const&")

    @unittest.skipUnless(platform.system() == "Windows", "dbghelp")
    def test_undecorate(self):
        # UnDecorateSymbolName ends a const member function with a space, which an editor would trim from the list;
        # the parameters of a function type are kept by every version of dbghelp.
        self.assertEqual(symbols.demangle_msvc(self.NAMES),
                         ["dxfcpp::v8::Quote::Quote(void)", "__int64 dxfcpp::v8::Candle::getCount(void)const",
                          "static void dxfcpp::v8::ErrorManager::setHandler(class std::function<void (struct "
                          "dxfcpp::v8::ErrorInfo const &)>)",
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

    def test_compare_ignores_the_inline_namespace_of_the_major_version(self):
        # The release commit of a new major version renames the inline namespace; the list was made before it.
        expected = ({"dxfcpp::v8::A::f(dxfcpp::v8::B const&)", "vtable for dxfcpp::v8::A", "dxfc_a"}, 7)
        actual = ({"dxfcpp::v9::A::f(dxfcpp::v9::B const&)", "vtable for dxfcpp::v9::A", "dxfc_a"}, 7)
        self.assertEqual(symbols.compare(expected, actual), [])
        differences = symbols.compare(expected, ({"dxfcpp::v9::A::f(dxfcpp::v9::B const&)", "dxfc_a"}, 7))
        self.assertEqual(differences[-1], "-vtable for dxfcpp::v*::A")
        # Only the namespace directly in dxfcpp is the version: a name with a v-number elsewhere still counts.
        self.assertIn("+dxfcpp::v*::v2::f()", symbols.compare(({"dxfc_a"}, 7), ({"dxfc_a", "dxfcpp::v9::v2::f()"}, 7)))


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


class DifferenceTest(unittest.TestCase):
    def test_changed_removed_added_by_entity(self):
        before = ({"dxfcpp::v8::A::f(int)", "dxfcpp::v8::A::g()", "dxfcpp::v8::A::h()", "vtable for dxfcpp::v8::A"}, 7)
        after = ({"dxfcpp::v9::A::f(long)", "dxfcpp::v9::A::g()", "dxfcpp::v9::A::k()", "vtable for dxfcpp::v9::A"}, 8)
        difference = symbols.Difference(before, after)
        self.assertEqual(difference.changed,
                         [("dxfcpp::v*::A::f", ["dxfcpp::v*::A::f(int)"], ["dxfcpp::v*::A::f(long)"])])
        self.assertEqual(difference.removed, ["dxfcpp::v*::A::h()"])
        self.assertEqual(difference.added, ["dxfcpp::v*::A::k()"])
        self.assertTrue(difference.breaking)
        self.assertEqual((difference.std_before, difference.std_after), (7, 8))

    def test_overloads_of_a_changed_entity_together(self):
        difference = symbols.Difference(({"dxfcpp::f(int)", "dxfcpp::f(char)"}, 0),
                                        ({"dxfcpp::f(int)", "dxfcpp::f(long)", "dxfcpp::f(short)"}, 0))
        self.assertEqual(difference.changed,
                         [("dxfcpp::f", ["dxfcpp::f(char)"], ["dxfcpp::f(long)", "dxfcpp::f(short)"])])
        self.assertEqual((difference.removed, difference.added), ([], []))

    def test_additions_do_not_break(self):
        difference = symbols.Difference(({"dxfc_a"}, 1), ({"dxfc_a", "dxfc_b"}, 1))
        self.assertFalse(difference.breaking)
        self.assertTrue(difference.differs)
        only_std = symbols.Difference(({"dxfc_a"}, 1), ({"dxfc_a"}, 2))
        self.assertFalse(only_std.symbols_differ)
        self.assertTrue(only_std.differs)

    def test_compare_breaking_changes_first(self):
        lines = symbols.compare(({"dxfcpp::f(int)", "dxfcpp::g()"}, 1), ({"dxfcpp::f(long)", "dxfcpp::h()"}, 2))
        self.assertEqual(lines, ["Changed signatures (breaking):", "@@ dxfcpp::f @@", "-dxfcpp::f(int)",
                                 "+dxfcpp::f(long)", "Removed (breaking):", "-dxfcpp::g()", "Added:", "+dxfcpp::h()",
                                 "std-total: list 1, library 2"])


def difference(before, after, std=(5, 5)):
    return symbols.Difference((set(before), std[0]), (set(after), std[1]))


class ReportTest(unittest.TestCase):
    def test_breaking_change_grouped_by_platforms(self):
        differences = {
            "linux": difference({"dxfc_a", "dxfcpp::f(int)", "dxfcpp::g()"}, {"dxfc_a", "dxfcpp::f(long)", "dxfc_new"}),
            "macos": difference({"dxfc_a", "dxfcpp::f(int)", "dxfcpp::g()"}, {"dxfc_a", "dxfcpp::f(long)", "dxfc_new"}),
            "windows": difference({"dxfc_a", "void dxfcpp::f(int)"}, {"dxfc_a", "void dxfcpp::f(long)", "dxfc_new"}),
        }
        text = symbols.report(differences, accept=symbols.ACCEPT_FROM_CI)
        self.assertTrue(text.startswith(symbols.REPORT_MARKER + "\n"))
        self.assertIn("**Breaks the ABI: yes:** exported symbols are removed or changed on all platforms.", text)
        self.assertIn("| linux | 1 | 1 | 1 | 5 |", text)
        self.assertIn("@@ dxfcpp::f (linux, macos) @@\n-dxfcpp::f(int)\n+dxfcpp::f(long)\n", text)
        self.assertIn("@@ dxfcpp::f (windows) @@\n-void dxfcpp::f(int)\n+void dxfcpp::f(long)\n", text)
        self.assertIn("#### Removed (breaking)\n\n```diff\n@@ linux, macos @@\n-dxfcpp::g()\n```", text)
        self.assertIn("#### Added\n\n```diff\n@@ all platforms @@\n+dxfc_new\n```", text)
        self.assertIn("python3 scripts/symbols.py fetch", text)
        self.assertNotIn("#### ABI check", text)

    def test_unchanged_and_added_only(self):
        same = {"linux": difference({"dxfc_a"}, {"dxfc_a"}), "windows": difference({"dxfc_a"}, {"dxfc_a"})}
        text = symbols.report(same, accept=symbols.ACCEPT_FROM_CI)
        self.assertIn("**Breaks the ABI: no:** the exported symbols are unchanged.", text)
        self.assertNotIn("####", text)
        added = {"linux": difference({"dxfc_a"}, {"dxfc_a", "dxfc_b"}, (5, 6))}
        text = symbols.report(added, accept=symbols.ACCEPT_FROM_CI)
        self.assertIn("**Breaks the ABI: no:** exported symbols are only added; the number of std:: instantiations "
                      "changed.", text)
        self.assertIn("| linux | 0 | 0 | 1 | 5 -> 6 |", text)
        self.assertIn("#### Accepting the lists", text)
        only_std = {"linux": difference({"dxfc_a"}, {"dxfc_a"}, (5, 6))}
        self.assertIn("**Breaks the ABI: no:** the exported symbols are unchanged; the number of std:: instantiations "
                      "changed.", symbols.report(only_std))

    def test_taken_lists_keep_the_changes_of_the_base(self):
        # The lists of abi/ match the CI (pending empty), the changes against the base branch are still shown.
        differences = {"linux": difference({"dxfc_a", "dxfc_b"}, {"dxfc_a"}),
                       "windows": difference({"dxfc_a"}, {"dxfc_a"})}
        text = symbols.report(differences, accept=symbols.ACCEPT_FROM_CI, pending=[])
        self.assertIn("**Breaks the ABI: yes:**", text)
        self.assertIn("```diff\n@@ linux @@\n-dxfc_b\n```", text)
        self.assertIn("#### Accepting the lists\n\nThe lists in `abi/` match the libraries of this run.", text)
        self.assertNotIn("scripts/symbols.py fetch", text)
        text = symbols.report(differences, accept=symbols.ACCEPT_FROM_CI, pending=["windows"])
        self.assertIn("The lists of windows in `abi/` differ from the libraries of this run.", text)
        self.assertIn("scripts/symbols.py fetch", text)
        self.assertFalse(symbols.report(differences, marker=False).startswith(symbols.REPORT_MARKER))

    def test_missing_list_is_unknown(self):
        differences = {"linux": difference({"dxfc_a"}, {"dxfc_a"}), "windows": None}
        self.assertEqual(symbols.verdict_of(differences, None), "unknown")
        self.assertIn("**Breaks the ABI: unknown:** no list of windows: the symbols job ended before the comparison",
                      symbols.report(differences))
        # A breaking change elsewhere decides it.
        differences["linux"] = difference({"dxfc_a"}, set())
        self.assertEqual(symbols.verdict_of(differences, None), "yes")

    def test_abi_results(self):
        same = {"linux": difference({"dxfc_a"}, {"dxfc_a"})}
        with tempfile.TemporaryDirectory() as directory:
            path = pathlib.Path(directory, "abi-report.txt")

            def from_file(text):
                path.write_text(text, encoding="utf-8")
                return symbols.AbiResult.from_ci(str(path), "found")

            incompatible = from_file("FAIL: incompatible ABI change (exit code 12): ...\n"
                                     "Functions changes summary: 1 Removed\n")
            self.assertEqual(symbols.verdict_of(same, incompatible), "yes")
            text = symbols.report(same, incompatible)
            self.assertIn("the ABI check found an incompatible change", text)
            self.assertIn("FAIL: incompatible ABI change (exit code 12): ...\n\n<details>", text)
            self.assertIn("```text\nFunctions changes summary: 1 Removed\n```", text)
            self.assertEqual(symbols.verdict_of(same, from_file("FAIL: the layout of a public type changed\n")), "yes")
            self.assertEqual(symbols.verdict_of(same, from_file("FAIL: abidiff reported an error (exit code 1)\n")),
                             "unknown")
            self.assertEqual(symbols.verdict_of(same, from_file("PASS: the ABI is unchanged (exit code 0)\n")), "no")
            # A file without a known verdict proves nothing.
            self.assertEqual(symbols.verdict_of(same, from_file("")), "unknown")
            self.assertIn("ABI check: no verdict", symbols.report(same, from_file("")))
            self.assertEqual(symbols.verdict_of(same, from_file("Segmentation fault\n")), "unknown")
            path.unlink()
            self.assertEqual(symbols.verdict_of(same, symbols.AbiResult.from_ci(str(path), "found")), "unknown")
            self.assertEqual(symbols.verdict_of(same, symbols.AbiResult.from_ci(None, "unknown")), "unknown")
            no_release = symbols.AbiResult.from_ci(None, "none")
            self.assertEqual(symbols.verdict_of(same, no_release), "no")
            self.assertIn("Not compared: no published release", symbols.report(same, no_release))

    def test_limit_cuts_the_longest_section_by_groups(self):
        before = {f"dxfcpp::F{index}::f(int)" for index in range(300)} | {"dxfcpp::removed()"}
        after = {f"dxfcpp::F{index}::f(long)" for index in range(300)} | {"dxfcpp::added()"}
        differences = {"linux": difference(before, after)}
        full = symbols.report(differences, accept=symbols.ACCEPT_FROM_CI)
        cut = symbols.report(differences, accept=symbols.ACCEPT_FROM_CI, limit=4000)
        self.assertGreater(len(full), 4000)
        self.assertLessEqual(len(cut), 4000)
        self.assertNotIn("more lines are cut", full)
        self.assertIn("more lines are cut: the full report is the artifact `exported-symbols-report`", cut)
        self.assertIn("```diff\n@@ linux @@\n-dxfcpp::removed()\n```", cut)
        self.assertIn("```diff\n@@ linux @@\n+dxfcpp::added()\n```", cut)
        self.assertIn("#### Accepting the lists", cut)
        changed = cut.split("#### Changed signatures (breaking)", 1)[1].split("```", 2)[1].splitlines()
        self.assertFalse(changed[-1].startswith("@@"))  # no header of a group without its lines


class ReportMainTest(unittest.TestCase):
    def test_report_command(self):
        with tempfile.TemporaryDirectory() as directory:
            root = pathlib.Path(directory)
            (root / "abi").mkdir()
            (root / "ci" / "symbols-linux").mkdir(parents=True)
            (root / "abi" / "symbols-linux.txt").write_text(symbols.render("linux", {"dxfc_a", "dxfc_b"}, 3))
            (root / "abi" / "symbols-windows.txt").write_text(symbols.render("windows", {"dxfc_a"}, 0))
            # The list of Windows is written by the job with CRLF.
            (root / "ci" / "symbols-linux" / "symbols-linux.txt").write_bytes(
                symbols.render("linux", {"dxfc_a"}, 3).replace("\n", "\r\n").encode())
            output = root / "github-output"
            report = root / "report.md"
            code = symbols.main(["report", str(root / "ci"), "--abi-dir", str(root / "abi"), "--abi-baseline", "none",
                                 "--limit", "65000", "--output", str(report), "--github-output", str(output)])
            self.assertEqual(code, 0)
            self.assertEqual(output.read_text(encoding="utf-8"), "verdict=yes\ncomment=true\n")
            text = report.read_text(encoding="utf-8")
            self.assertIn("| windows | | | | no list |", text)
            self.assertIn("```diff\n@@ linux @@\n-dxfc_b\n```", text)
            self.assertIn("Not compared: no published release", text)

    def test_base_dir_and_comment_flag(self):
        with tempfile.TemporaryDirectory() as directory:
            root = pathlib.Path(directory)
            for name in ("base", "abi", "ci"):
                (root / name).mkdir()
            (root / "base" / "symbols-linux.txt").write_text(symbols.render("linux", {"dxfc_a", "dxfc_b"}, 3))
            (root / "abi" / "symbols-linux.txt").write_text(symbols.render("linux", {"dxfc_a"}, 3))
            (root / "ci" / "symbols-linux.txt").write_text(symbols.render("linux", {"dxfc_a"}, 3))
            output, report = root / "github-output", root / "report.md"

            def run(*options):
                output.write_text("")
                code = symbols.main(["report", str(root / "ci"), "--abi-dir", str(root / "abi"), *options,
                                     "--output", str(report), "--github-output", str(output)])
                self.assertEqual(code, 0)
                return output.read_text(encoding="utf-8"), report.read_text(encoding="utf-8")

            # The taken lists match the CI: against the base branch the change still breaks the ABI.
            flags, text = run("--base-dir", str(root / "base"))
            self.assertEqual(flags, "verdict=yes\ncomment=true\n")
            self.assertIn("The lists in `abi/` match the libraries of this run.", text)
            # Without the base: nothing changed, no new comment.
            flags, text = run()
            self.assertEqual(flags, "verdict=no\ncomment=false\n")
            self.assertNotIn("####", text)

    def test_new_platform_without_a_list_in_the_base(self):
        with tempfile.TemporaryDirectory() as directory:
            root = pathlib.Path(directory)
            for name in ("base", "abi", "ci"):
                (root / name).mkdir()
            for name in ("base", "abi", "ci"):
                (root / name / "symbols-linux.txt").write_text(symbols.render("linux", {"dxfc_a"}, 3))
            for name in ("abi", "ci"):
                (root / name / "symbols-linux-aarch64.txt").write_text(symbols.render("linux", {"dxfc_a"}, 5))
            report = root / "report.md"
            code = symbols.main(["report", str(root / "ci"), "--abi-dir", str(root / "abi"), "--base-dir",
                                 str(root / "base"), "--output", str(report)])
            self.assertEqual(code, 0)
            text = report.read_text(encoding="utf-8")
            self.assertIn("| linux | 0 | 0 | 0 | 3 |\n| linux-aarch64 | 0 | 0 | 0 | 5 |", text)


class WorkflowTest(unittest.TestCase):
    def test_the_job_finds_its_comment_by_the_marker(self):
        workflow = (SCRIPTS.parent / ".github" / "workflows" / "build.yml").read_text(encoding="utf-8")
        self.assertIn(f"marker='{symbols.REPORT_MARKER}'", workflow)


class FetchTest(unittest.TestCase):
    def setUp(self):
        self.directory = pathlib.Path(tempfile.mkdtemp())
        self.addCleanup(shutil.rmtree, self.directory)
        self.abi = self.directory / "abi"
        self.abi.mkdir()
        (self.abi / "symbols-linux.txt").write_text(symbols.render("linux", {"dxfc_a", "dxfc_b"}, 3), newline="\n")
        (self.abi / "symbols-windows.txt").write_text(symbols.render("windows", {"dxfc_a"}, 0), newline="\n")
        self.runs = [
            {"databaseId": 30, "status": "completed", "conclusion": "cancelled", "createdAt": "2026-11-09T12:00:00Z"},
            {"databaseId": 20, "status": "completed", "conclusion": "failure", "createdAt": "2026-11-09T11:00:00Z"},
            {"databaseId": 10, "status": "completed", "conclusion": "success", "createdAt": "2026-11-09T10:00:00Z"},
        ]
        self.ci_lists = {"linux": symbols.render("linux", {"dxfc_a", "dxfc_c"}, 3).replace("\n", "\r\n"),
                         "windows": symbols.render("windows", {"dxfc_a"}, 0).replace("\n", "\r\n")}
        self.downloaded = []

    def fake_command(self, command):
        if command[:2] == ["git", "rev-parse"]:
            return "0123456789abcdef\n"
        if command[:3] == ["gh", "run", "list"]:
            self.assertIn("0123456789abcdef", command)
            return json.dumps(self.runs)
        if command[:3] == ["gh", "run", "download"]:
            self.downloaded.append(command[3])
            if not self.ci_lists:
                raise SymbolsError("gh run download failed: no valid artifacts found to download")
            target = pathlib.Path(command[command.index("--dir") + 1])
            for name, text in self.ci_lists.items():
                (target / f"symbols-{name}").mkdir(parents=True)
                (target / f"symbols-{name}" / f"symbols-{name}.txt").write_bytes(text.encode())
            return ""
        self.fail(f"unexpected command {command}")

    def fetch(self, **options):
        with unittest.mock.patch.object(symbols, "run_command", self.fake_command):
            return symbols.fetch(abi_dir=self.abi, **options)

    def test_latest_run_not_cancelled_written_with_lf(self):
        text = self.fetch()
        self.assertEqual(self.downloaded, ["20"])
        self.assertIn("The lists of the CI run 20", text)
        self.assertIn("**Breaks the ABI: yes:**", text)
        self.assertNotIn(symbols.REPORT_MARKER, text)
        self.assertIn("Written: symbols-linux.txt.", text)
        self.assertEqual((self.abi / "symbols-linux.txt").read_bytes(),
                         symbols.render("linux", {"dxfc_a", "dxfc_c"}, 3).encode())

    def test_dry_run_and_matching_lists_write_nothing(self):
        before = (self.abi / "symbols-linux.txt").read_bytes()
        self.assertIn("Nothing written.", self.fetch(dry_run=True))
        self.assertEqual((self.abi / "symbols-linux.txt").read_bytes(), before)
        self.ci_lists["linux"] = before.decode()
        self.assertIn("The lists match: nothing to write.", self.fetch(run=7))
        self.assertEqual(self.downloaded, ["20", "7"])

    def test_list_of_a_new_platform_written(self):
        self.ci_lists["linux"] = (self.abi / "symbols-linux.txt").read_text()
        self.ci_lists["linux-aarch64"] = symbols.render("linux", {"dxfc_a", "dxfc_b"}, 5).replace("\n", "\r\n")
        text = self.fetch()
        self.assertIn("New platforms: linux-aarch64.", text)
        self.assertIn("Written: symbols-linux-aarch64.txt.", text)
        self.assertEqual((self.abi / "symbols-linux-aarch64.txt").read_bytes(),
                         symbols.render("linux", {"dxfc_a", "dxfc_b"}, 5).encode())

    def test_run_in_progress_and_no_run(self):
        self.runs = [{"databaseId": 40, "status": "in_progress", "conclusion": "", "createdAt": "2026-11-09T13:00:00Z"}]
        self.assertIn("the run 40 is not completed", self.fetch())
        self.runs = []
        with self.assertRaisesRegex(SymbolsError, "no CI run of the commit 0123456789ab"):
            self.fetch()

    def test_a_run_without_lists(self):
        self.ci_lists = {}
        with self.assertRaisesRegex(SymbolsError, "the run 20 has no lists"):
            self.fetch()


if __name__ == "__main__":
    unittest.main()
