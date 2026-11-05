#!/usr/bin/env python3
# Copyright (c) 2026 Devexperts LLC.
# SPDX-License-Identifier: MPL-2.0

"""The exported symbols of the shared library against the list of its OS (abi/symbols-<os>.txt).

    python3 scripts/symbols.py extract <library>                      print the list of the library
    python3 scripts/symbols.py compare <library> <list> [--no-std-total] [--update]

The list holds the demangled own symbols of the library (dxfcpp::, the C API, the typeinfo and vtables of own classes),
sorted, one per line. The std:: instantiations depend on the toolchain, so the list keeps only their number
("std-total: N"); --no-std-total skips it (the configurations other than the one the list was made on). --update
rewrites the list with the symbols of the library.

The names come from nm (ELF: nm -D --defined-only, Mach-O: nm -gU) and are demangled with c++filt; the names of a DLL
come from its export table and are undecorated with UnDecorateSymbolName of dbghelp (Windows only).
"""

import argparse
import difflib
import pathlib
import platform
import re
import struct
import subprocess
import sys

OS_OF_KIND = {"elf": "linux", "macho": "macos", "pe": "windows"}

# The symbols of the linker and the runtime, not of the library.
LINKER_SYMBOLS = {"_init", "_fini", "_edata", "_end", "__bss_start", "_DYNAMIC", "_GLOBAL_OFFSET_TABLE_",
                  "_mh_dylib_header"}

# The namespaces of the standard library and of its ABI runtime: their symbols are counted, not listed.
STD_PREFIXES = ("std::", "__gnu_cxx::", "__gnu_debug::", "__cxxabiv1::")

# The special names of the Itanium demangler before the entity they belong to.
SPECIAL_PREFIX = re.compile(
    r"^(?:(?:typeinfo name for|typeinfo for|vtable for|VTT for|construction vtable for|guard variable for"
    r"|non-virtual thunk to|virtual thunk to|covariant return thunk to|TLS init function for"
    r"|TLS wrapper function for|transaction clone for|reference temporary #\d+ for) )+"
)

# The spellings of the overloaded operators, the longest first: "operator<<" is the shift, "operator< <int>" the less.
OPERATORS = sorted(["<=>", "<<=", ">>=", "->*", "()", "[]", "<<", ">>", "<=", ">=", "==", "!=", "&&", "||", "++", "--",
                    "+=", "-=", "*=", "/=", "%=", "&=", "|=", "^=", "->", "+", "-", "*", "/", "%", "^", "&", "|", "~",
                    "!", "=", "<", ">", ","], key=len, reverse=True)

# UNDNAME_NO_ACCESS_SPECIFIERS: no public:. The Microsoft keywords are removed by MS_KEYWORDS, not by
# UNDNAME_NO_MS_KEYWORDS: some versions of dbghelp (10.0.22621, 10.0.25921) drop the parameters of a function type
# with that flag ("std::function<void >"), so the list would depend on the version of Windows.
UNDNAME_FLAGS = 0x0080

# The Microsoft keywords of the undecorated names: the calling conventions with the space after them ("void __cdecl(",
# "void __cdecl f"), the pointer modifiers with the space before them ("const & __ptr64)").
MS_KEYWORDS = re.compile(r"__(?:cdecl|stdcall|fastcall|thiscall|vectorcall|clrcall)\b ?| __(?:ptr64|ptr32)\b")


class SymbolsError(Exception):
    pass


def binary_kind(data):
    """elf, macho or pe by the magic number of the file."""
    if data[:4] == b"\x7fELF":
        return "elf"
    if data[:4] in (b"\xcf\xfa\xed\xfe", b"\xce\xfa\xed\xfe", b"\xca\xfe\xba\xbe"):
        return "macho"
    if data[:2] == b"MZ":
        return "pe"
    raise SymbolsError("not an ELF, Mach-O or PE file")


def parse_nm(text, kind):
    """The mangled names of nm output: the last field of each line, without the ELF symbol version and the leading
    underscore of Mach-O."""
    names = []
    for line in text.splitlines():
        fields = line.split()
        if not fields:
            continue
        name = fields[-1]
        if kind == "elf":
            name = name.split("@", 1)[0]
        elif kind == "macho" and name.startswith("_"):
            name = name[1:]
        names.append(name)
    return names


def pe_export_names(data):
    """The names of the export table of a PE file (DLL)."""
    try:
        pe = struct.unpack_from("<I", data, 0x3C)[0]
        if data[pe:pe + 4] != b"PE\0\0":
            raise SymbolsError("no PE signature")
        coff = pe + 4
        section_count = struct.unpack_from("<H", data, coff + 2)[0]
        optional_size = struct.unpack_from("<H", data, coff + 16)[0]
        optional = coff + 20
        magic = struct.unpack_from("<H", data, optional)[0]
        directories = optional + (96 if magic == 0x10B else 112)
        export_rva = struct.unpack_from("<I", data, directories)[0]
        sections = optional + optional_size

        def offset(rva):
            for index in range(section_count):
                header = sections + 40 * index
                virtual_size, virtual_address, raw_size, raw_pointer = struct.unpack_from("<IIII", data, header + 8)
                if virtual_address <= rva < virtual_address + max(virtual_size, raw_size):
                    return rva - virtual_address + raw_pointer
            raise SymbolsError(f"RVA {rva:#x} is outside of the sections")

        if export_rva == 0:
            return []
        directory = offset(export_rva)
        name_count = struct.unpack_from("<I", data, directory + 24)[0]
        if name_count == 0:
            return []
        names_offset = offset(struct.unpack_from("<I", data, directory + 32)[0])
        names = []
        for index in range(name_count):
            start = offset(struct.unpack_from("<I", data, names_offset + 4 * index)[0])
            names.append(data[start:data.index(b"\0", start)].decode("ascii"))
        return names
    except (struct.error, ValueError) as error:
        raise SymbolsError(f"broken PE export table: {error}") from error


def demangle_itanium(names):
    if not names:
        return []
    try:
        result = subprocess.run(["c++filt", "-n"], input="\n".join(names) + "\n", capture_output=True, text=True,
                                check=True)
    except (OSError, subprocess.CalledProcessError) as error:
        raise SymbolsError(f"c++filt failed: {error}") from error
    demangled = result.stdout.splitlines()
    if len(demangled) != len(names):
        raise SymbolsError(f"c++filt returned {len(demangled)} names for {len(names)}")
    return demangled


def demangle_msvc(names):
    if platform.system() != "Windows":
        raise SymbolsError("the names of a DLL are undecorated on Windows only (dbghelp)")
    import ctypes

    undecorate = ctypes.windll.dbghelp.UnDecorateSymbolName
    buffer = ctypes.create_string_buffer(8192)
    demangled = []
    for name in names:
        if name.startswith("?") and undecorate(name.encode("ascii"), buffer, len(buffer), UNDNAME_FLAGS):
            # A const member function ends with a space ("...(void)const "), which an editor would trim from the list.
            demangled.append(MS_KEYWORDS.sub("", buffer.value.decode("ascii")).rstrip())
        else:
            demangled.append(name)
    return demangled


def entity(name):
    """The qualified name of the function, variable or class that a demangled symbol belongs to: the last name before
    the parameter list, outside of template arguments, without the special prefixes (typeinfo for, vtable for...)."""
    text = SPECIAL_PREFIX.sub("", name)
    depth = 0
    start = 0
    keep_token = False  # a conversion or literal operator: its spaces do not start a new name
    index = 0
    while index < len(text):
        char = text[index]
        after = index + len("operator")
        if (text.startswith("operator", index) and (index == 0 or text[index - 1] in ": ")
                and not (after < len(text) and (text[after].isalnum() or text[after] == "_"))):
            index = after
            if text.startswith("()", index):
                index += 2
                continue
            if text.startswith('""', index) or text.startswith(" ", index):
                keep_token = True
                continue
            index += next((len(spelling) for spelling in OPERATORS if text.startswith(spelling, index)), 0)
            continue
        if char == "<":
            depth += 1
        elif char == ">" and depth > 0:
            depth -= 1
        elif char == "(" and depth == 0:
            if text[start:index] == "decltype":  # a return type decltype(...): skip its parentheses
                nesting = 0
                while index < len(text):
                    nesting += {"(": 1, ")": -1}.get(text[index], 0)
                    if nesting == 0:
                        break
                    index += 1
                index += 1
                continue
            if index > start:
                return text[start:index].strip()
            break
        elif char == " " and depth == 0 and not keep_token:
            start = index + 1
        index += 1
    return text[start:].strip() or text


def split_symbols(demangled):
    """The own symbols (a set) and the number of the std:: ones."""
    own = set()
    std_names = set()
    for name in demangled:
        if name in LINKER_SYMBOLS:
            continue
        if entity(name).startswith(STD_PREFIXES):
            std_names.add(name)
        else:
            own.add(name)
    return own, len(std_names)


def library_symbols(path):
    """(os name, own symbols, number of std:: symbols) of a shared library."""
    path = pathlib.Path(path)
    if not path.is_file():
        raise SymbolsError(f"{path}: no such file")
    with path.open("rb") as file:
        kind = binary_kind(file.read(4))
    if kind == "pe":
        demangled = demangle_msvc(pe_export_names(path.read_bytes()))
    else:
        command = ["nm", "-D", "--defined-only"] if kind == "elf" else ["nm", "-gU"]
        try:
            text = subprocess.run(command + [str(path)], capture_output=True, text=True, check=True).stdout
        except (OSError, subprocess.CalledProcessError) as error:
            raise SymbolsError(f"nm failed: {error}") from error
        demangled = demangle_itanium(parse_nm(text, kind))
    own, std_total = split_symbols(demangled)
    if not own:
        raise SymbolsError(f"{path}: no exported symbols")
    return OS_OF_KIND[kind], own, std_total


def render(os_name, own, std_total):
    return (
        f"# Exported symbols of the shared library ({os_name}), made by scripts/symbols.py compare --update.\n"
        "# Own symbols one per line; the std:: instantiations only as their number.\n"
        f"std-total: {std_total}\n" + "".join(f"{name}\n" for name in sorted(own))
    )


def parse_list(text):
    """(own symbols, std total) of a list file."""
    own = set()
    std_total = None
    for line in text.splitlines():
        line = line.rstrip()
        if not line or line.startswith("#"):
            continue
        if line.startswith("std-total:"):
            std_total = int(line.split(":", 1)[1])
        else:
            own.add(line)
    if std_total is None:
        raise SymbolsError("the list has no std-total line")
    return own, std_total


def compare(expected, actual, check_std_total=True):
    """The differences as lines: a unified diff of the own symbols and the std:: totals; empty when equal."""
    (expected_own, expected_std), (actual_own, actual_std) = expected, actual
    lines = list(difflib.unified_diff(sorted(expected_own), sorted(actual_own), "list", "library", lineterm="", n=0))
    if check_std_total and expected_std != actual_std:
        lines.append(f"std-total: list {expected_std}, library {actual_std}")
    return lines


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    commands = parser.add_subparsers(dest="command", required=True)
    extract_parser = commands.add_parser("extract", help="print the list of the library")
    extract_parser.add_argument("library")
    compare_parser = commands.add_parser("compare", help="compare the library with the list")
    compare_parser.add_argument("library")
    compare_parser.add_argument("list")
    compare_parser.add_argument("--no-std-total", action="store_true", help="do not compare the std:: total")
    compare_parser.add_argument("--update", action="store_true", help="rewrite the list with the library")
    args = parser.parse_args(argv)

    try:
        os_name, own, std_total = library_symbols(args.library)
        if args.command == "extract":
            sys.stdout.write(render(os_name, own, std_total))
            return 0
        list_path = pathlib.Path(args.list)
        if args.update:
            list_path.parent.mkdir(parents=True, exist_ok=True)
            list_path.write_text(render(os_name, own, std_total), encoding="utf-8", newline="\n")
            print(f"{list_path}: {len(own)} symbols, std-total {std_total}")
            return 0
        if not list_path.is_file():
            raise SymbolsError(f"{list_path}: no such file")
        differences = compare(parse_list(list_path.read_text(encoding="utf-8")), (own, std_total),
                              not args.no_std_total)
        if differences:
            print(f"The exported symbols of {args.library} differ from {list_path}:")
            print("\n".join(differences))
            print(f"If the change is intended: python3 scripts/symbols.py compare --update {args.library} {list_path}")
            return 1
        print(f"{list_path}: {len(own)} symbols match" + ("" if args.no_std_total else f", std-total {std_total}"))
        return 0
    except SymbolsError as error:
        print(f"symbols.py: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    sys.exit(main())
