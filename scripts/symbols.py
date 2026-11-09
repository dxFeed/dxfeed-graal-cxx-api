#!/usr/bin/env python3
# Copyright (c) 2026 Devexperts LLC.
# SPDX-License-Identifier: MPL-2.0

"""The exported symbols of the shared library against the list of its OS (abi/symbols-<os>.txt).

    python3 scripts/symbols.py extract <library>                      print the list of the library
    python3 scripts/symbols.py compare <library> <list> [--no-std-total] [--update]
    python3 scripts/symbols.py report <lists dir> [--base-dir <dir>] [--abi-report <file>] [--abi-baseline found|none]
                                      [--limit <chars>] [--output <file>] [--github-output <file>]
    python3 scripts/symbols.py fetch [--run <id>] [--dry-run]

(On Windows: py -3 instead of python3.)

The list holds the demangled own symbols of the library (dxfcpp::, the C API, the typeinfo and vtables of own classes),
sorted, one per line. The std:: instantiations depend on the toolchain, so the list keeps only their number
("std-total: N"); --no-std-total skips it (the configurations other than the one the list was made on). --update
rewrites the list with the symbols of the library.

A difference is shown by entity (the function, variable or class a symbol belongs to): an entity that lost symbols and
gained others has a changed signature, the old and the new names side by side; the other lost symbols are removed and
the other new ones added. Changed and removed symbols break the ABI.

report: the Markdown report of the CI run for all platforms: the lists of the symbols jobs (symbols-<os>.txt anywhere
under <lists dir>) against the lists of the base branch (--base-dir; default abi/), the result of the abi job, the
verdict "breaks the ABI: yes, no or unknown" and, when abi/symbols-<os>.txt differ from the CI lists, how to take
them; so the report of a pull request keeps showing its changes after the lists are taken. fetch: takes the lists of
the latest CI run of the current commit (gh), prints the same report and writes them to abi/; the commit is left to the
developer.

The names come from nm (ELF: nm -D --defined-only, Mach-O: nm -gU) and are demangled with c++filt; the names of a DLL
come from its export table and are undecorated with UnDecorateSymbolName of dbghelp (Windows only).
"""

import argparse
import json
import pathlib
import platform
import re
import shutil
import struct
import subprocess
import sys
import tempfile

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

# The inline namespace of the major version (dxfcpp::v8::). The release of a new major version renames it in its
# release commit, so the lists, made before it, are compared without it.
INLINE_NAMESPACE = re.compile(r"\bdxfcpp::v\d+::")

# The repository: the lists are in <root>/abi, the CI workflow is build.yml.
ROOT = pathlib.Path(__file__).resolve().parent.parent
WORKFLOW = "build.yml"

# The first line of the report: the CI finds its comment of the pull request by it.
REPORT_MARKER = "<!-- exported-symbols-report -->"


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


def normalized(names):
    """The names with the inline namespace of the major version shown as dxfcpp::v*::."""
    return {INLINE_NAMESPACE.sub("dxfcpp::v*::", name) for name in names}


class Difference:
    """The difference of a list (expected) and a library (actual), each (own symbols, std:: total), by entity: changed
    [(entity, old names, new names)], removed and added names, all sorted, the inline namespace of the major version
    shown as dxfcpp::v*:: (the release commit of a new major version renames it, the lists are made before it)."""

    def __init__(self, expected, actual):
        (expected_own, self.std_before), (actual_own, self.std_after) = expected, actual
        before, after = normalized(expected_own), normalized(actual_own)
        lost, gained = {}, {}
        for name in before - after:
            lost.setdefault(entity(name), []).append(name)
        for name in after - before:
            gained.setdefault(entity(name), []).append(name)
        self.changed = [(key, sorted(lost[key]), sorted(gained[key])) for key in sorted(lost.keys() & gained.keys())]
        self.removed = sorted(name for key, names in lost.items() if key not in gained for name in names)
        self.added = sorted(name for key, names in gained.items() if key not in lost for name in names)

    @property
    def breaking(self):
        return bool(self.changed or self.removed)

    @property
    def symbols_differ(self):
        return bool(self.changed or self.removed or self.added)

    @property
    def differs(self):
        return self.symbols_differ or self.std_before != self.std_after

    def lines(self):
        """The difference of the own symbols as text, the breaking changes first."""
        lines = []
        if self.changed:
            lines.append("Changed signatures (breaking):")
            for key, old, new in self.changed:
                lines += [f"@@ {key} @@"] + [f"-{name}" for name in old] + [f"+{name}" for name in new]
        if self.removed:
            lines += ["Removed (breaking):"] + [f"-{name}" for name in self.removed]
        if self.added:
            lines += ["Added:"] + [f"+{name}" for name in self.added]
        return lines


def compare(expected, actual, check_std_total=True):
    """The differences as lines (Difference.lines and the std:: totals); empty when equal."""
    difference = Difference(expected, actual)
    lines = difference.lines()
    if check_std_total and difference.std_before != difference.std_after:
        lines.append(f"std-total: list {difference.std_before}, library {difference.std_after}")
    return lines


class AbiResult:
    """The result of the abi job: the verdict line of scripts/abi_check.sh compare (PASS: or FAIL:) and the report of
    abidiff, or a note why the ABI was not compared."""

    def __init__(self, verdict, report="", unknown=None):
        """unknown: None to take it from the verdict: anything but PASS: or a FAIL: of an incompatible change."""
        self.verdict, self.report = verdict, report
        error = verdict.startswith("FAIL: abidiff reported an error")
        self.breaking = verdict.startswith("FAIL:") and not error
        self.unknown = not (verdict.startswith("PASS:") or self.breaking) if unknown is None else unknown

    @classmethod
    def from_ci(cls, report_path, baseline):
        """From the file of ABI_REPORT (the verdict, then the report) or, without it, from the abi_baseline job: found
        (then the abi job ended before the comparison), none (no release to compare with) or unknown (it failed)."""
        if report_path and pathlib.Path(report_path).is_file():
            text = pathlib.Path(report_path).read_text(encoding="utf-8", errors="replace")
            verdict, _, report = text.partition("\n")
            return cls(verdict.strip(), report.strip("\n"))
        if baseline == "found":
            return cls("Not compared: the abi job ended before the comparison.", unknown=True)
        if baseline == "none":
            return cls("Not compared: no published release of this major version to compare with.", unknown=False)
        return cls("Not compared: the reference release was not looked up (the abi_baseline job failed).",
                   unknown=True)


def verdict_of(differences, abi):
    """yes, no or unknown: does the change break the ABI."""
    if any(d is not None and d.breaking for d in differences.values()) or (abi is not None and abi.breaking):
        return "yes"
    if any(d is None for d in differences.values()) or (abi is not None and abi.unknown):
        return "unknown"
    return "no"


def group_by_platforms(items, order):
    """[(platforms, [values])] of {value: [platforms]}: the values with the same platforms together, the groups of
    more platforms first."""
    groups = {}
    for value, platforms in items.items():
        groups.setdefault(tuple(sorted(platforms, key=order.index)), []).append(value)
    return sorted(((platforms, sorted(values)) for platforms, values in groups.items()),
                  key=lambda group: (-len(group[0]), [order.index(name) for name in group[0]]))


# The accepting section of the CI report.
ACCEPT_FROM_CI = [
    "If the changes are intended, take the lists of this run into `abi/` and commit them:",
    "",
    "```shell",
    "python3 scripts/symbols.py fetch   # Windows: py -3 scripts/symbols.py fetch",
    "```",
]

# The reserve of the length for a note under a cut section.
CUT_NOTE_LENGTH = 160


def summary_of(differences, abi, label):
    """The first line of the report: the verdict and its reasons."""
    present = [difference for difference in differences.values() if difference is not None]
    verdict = verdict_of(differences, abi)
    reasons = []
    if verdict == "yes":
        breaking = [name for name, difference in differences.items() if difference is not None and difference.breaking]
        if breaking:
            reasons.append(f"exported symbols are removed or changed on {label(breaking)}")
        if abi is not None and abi.breaking:
            reasons.append("the ABI check found an incompatible change")
        return f"**Breaks the ABI: yes:** {'; '.join(reasons)}. Such a change needs a new major version."
    if verdict == "unknown":
        missing = [name for name, difference in differences.items() if difference is None]
        if missing:
            reasons.append(f"no list of {label(missing)}: the symbols job ended before the comparison")
        if abi is not None and abi.unknown:
            reasons.append(f"ABI check: {abi.verdict or 'no verdict'}")
        return f"**Breaks the ABI: unknown:** {'; '.join(reasons)}"
    summary = ("**Breaks the ABI: no:** exported symbols are only added" if any(d.added for d in present)
               else "**Breaks the ABI: no:** the exported symbols are unchanged")
    if any(d.std_before != d.std_after for d in present):
        summary += "; the number of std:: instantiations changed"
    return summary + "."


def report(differences, abi=None, limit=None, accept=None, pending=None, marker=True):
    """The Markdown report.

    differences: {platform: Difference, or None when the CI has no list of the platform}; abi: AbiResult or None (no
    section); limit: the maximum length (a comment of a pull request takes 65536 characters), reached by cutting the
    lines of the sections; accept: the lines of the section how to take the lists, shown for the platforms of pending
    (their lists in abi/ differ from the CI; None: the platforms that differ in differences); marker: the first line
    REPORT_MARKER, by which the job finds its comment."""
    order = list(differences)
    present = [name for name in order if differences[name] is not None]
    if pending is None:
        pending = [name for name in present if differences[name].differs]

    def label(platforms):
        return "all platforms" if len(order) > 1 and list(platforms) == order else ", ".join(platforms)

    changed, removed, added = {}, {}, {}
    for name in present:
        difference = differences[name]
        for key, old, new in difference.changed:
            changed.setdefault((key, tuple(old), tuple(new)), []).append(name)
        for symbol in difference.removed:
            removed.setdefault(symbol, []).append(name)
        for symbol in difference.added:
            added.setdefault(symbol, []).append(name)

    head = ([REPORT_MARKER] if marker else []) + [
        "### Exported symbols and ABI", "", summary_of(differences, abi, label), "",
        "| Platform | Changed | Removed | Added | std:: instantiations |",
        "| --- | ---: | ---: | ---: | ---: |"]
    for name in order:
        difference = differences[name]
        if difference is None:
            head.append(f"| {name} | | | | no list |")
            continue
        std = (str(difference.std_after) if difference.std_before == difference.std_after
               else f"{difference.std_before} -> {difference.std_after}")
        head.append(f"| {name} | {len(difference.changed)} | {len(difference.removed)} | {len(difference.added)} "
                    f"| {std} |")

    # The sections: (title, fence language, lines, details): the lines can be cut to the limit.
    sections = []
    if changed:
        lines = []
        for (key, old, new), platforms in sorted(changed.items()):
            lines += [f"@@ {key} ({label(platforms)}) @@"] + [f"-{name}" for name in old] + [f"+{name}" for name in new]
        sections.append(("Changed signatures (breaking)", "diff", lines, None))
    for title, symbols_of, sign in (("Removed (breaking)", removed, "-"), ("Added", added, "+")):
        if symbols_of:
            lines = []
            for platforms, names in group_by_platforms(symbols_of, order):
                lines += [f"@@ {label(platforms)} @@"] + [f"{sign}{name}" for name in names]
            sections.append((title, "diff", lines, None))
    if abi is not None:
        sections.append(("ABI check (abidiff, Linux)", "text", abi.report.splitlines(), abi.verdict))
    tail = []
    if accept and pending:
        tail = [f"The lists of {label(pending)} in `abi/` differ from the libraries of this run.", ""] + list(accept)
    elif accept and any(differences[name].differs for name in present):
        tail = ["The lists in `abi/` match the libraries of this run."]

    fixed = len("\n".join(head + tail)) + sum(len(title) + len(details or "") + 80 for title, _, _, details in sections)
    kept_of = {index: lines for index, (_, _, lines, _) in enumerate(sections)}
    if limit is not None:
        # An equal share of the rest for each section; what a short section leaves goes to the longer ones.
        budget = max(0, limit - fixed - CUT_NOTE_LENGTH * len(sections))
        by_size = sorted(range(len(sections)), key=lambda index: sum(len(line) + 1 for line in sections[index][2]))
        for position, index in enumerate(by_size):
            share = budget // (len(sections) - position)
            kept = []
            for line in sections[index][2]:
                if len(line) + 1 > share:
                    break
                kept.append(line)
                share -= len(line) + 1
                budget -= len(line) + 1
            if len(kept) < len(sections[index][2]) and kept and kept[-1].startswith("@@"):
                budget += len(kept.pop()) + 1  # no header of a group without its lines
            kept_of[index] = kept

    out = list(head)
    for index, (title, language, lines, details) in enumerate(sections):
        kept = kept_of[index]
        out += ["", f"#### {title}", ""]
        if details is not None:
            out.append(details)
            if kept:
                out += ["", "<details><summary>The report of abidiff</summary>", ""]
        if kept:
            out += [f"```{language}"] + kept + ["```"]
        if details is not None and kept:
            out += ["", "</details>"]
        if len(kept) < len(lines):
            out += ["", f"*{len(lines) - len(kept)} more lines are cut: the full report is the artifact "
                        "`exported-symbols-report` of the CI run.*"]
    if tail:
        out += ["", "#### Accepting the lists", ""] + tail
    return "\n".join(out) + "\n"


def read_list(path):
    return parse_list(pathlib.Path(path).read_text(encoding="utf-8"))


def find_lists(directory, recursive=True):
    """{platform: path} of the files symbols-<platform>.txt in the directory (and under it when recursive)."""
    paths = pathlib.Path(directory).rglob("symbols-*.txt") if recursive else pathlib.Path(directory).glob(
        "symbols-*.txt")
    lists = {path.stem[len("symbols-"):]: path for path in paths}
    return {name: lists[name] for name in sorted(lists)}


def differences_of(lists, abi_dir):
    """{platform: Difference or None} of the lists of the CI ({platform: path}) against the lists of abi_dir."""
    return {name: Difference(read_list(listed), read_list(lists[name])) if name in lists else None
            for name, listed in find_lists(abi_dir, recursive=False).items()}


def run_command(command):
    """The output of a command run in the repository."""
    try:
        return subprocess.run(command, cwd=ROOT, capture_output=True, text=True, check=True).stdout
    except FileNotFoundError as error:
        raise SymbolsError(f"{command[0]} not found") from error
    except subprocess.CalledProcessError as error:
        raise SymbolsError(f"{' '.join(command)} failed: {error.stderr.strip() or error}") from error


def latest_run(commit):
    """(id, note) of the latest CI run of the commit that was not cancelled, a completed one first; the note says when
    the run is not completed yet."""
    runs = json.loads(run_command(["gh", "run", "list", "--workflow", WORKFLOW, "--commit", commit, "--limit", "20",
                                   "--json", "databaseId,status,conclusion,createdAt"]))
    runs = sorted((run for run in runs if run["conclusion"] != "cancelled"), key=lambda run: run["createdAt"],
                  reverse=True)
    if not runs:
        raise SymbolsError(f"no CI run of the commit {commit[:12]}: push it, or pass --run <id>")
    completed = [run for run in runs if run["status"] == "completed"]
    if completed:
        return completed[0]["databaseId"], None
    return runs[0]["databaseId"], (f"the run {runs[0]['databaseId']} is not completed: only its finished symbols jobs "
                                   "count")


def fetch(run=None, dry_run=False, abi_dir=ROOT / "abi"):
    """Takes the lists of a CI run (the latest of HEAD by default) into abi_dir; returns the report."""
    note = None
    if run is None:
        run, note = latest_run(run_command(["git", "rev-parse", "HEAD"]).strip())
    with tempfile.TemporaryDirectory() as directory:
        try:
            run_command(["gh", "run", "download", str(run), "--pattern", "symbols-*", "--dir", directory])
        except SymbolsError as error:
            if "no valid artifacts" not in str(error):
                raise
        lists = find_lists(directory)
        if not lists:
            raise SymbolsError(f"the run {run} has no lists (artifacts symbols-*): its symbols jobs did not run or "
                               "ended before the comparison")
        differences = differences_of(lists, abi_dir)
        lines = [f"The lists of the CI run {run} against {abi_dir}" + (f" ({note})" if note else "") + ":", ""]
        lines.append(report(differences, marker=False))
        changed = [name for name, difference in differences.items() if difference is not None and difference.differs]
        if dry_run or not changed:
            lines.append("Nothing written." if dry_run else "The lists match: nothing to write.")
        else:
            for name in changed:
                text = lists[name].read_text(encoding="utf-8").replace("\r\n", "\n")
                (pathlib.Path(abi_dir) / f"symbols-{name}.txt").write_text(text, encoding="utf-8", newline="\n")
            written = ", ".join(f"symbols-{name}.txt" for name in changed)
            lines.append(f"Written: {written}. Review and commit them.")
    return "\n".join(lines)


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
    report_parser = commands.add_parser("report", help="the Markdown report of the lists of a CI run")
    report_parser.add_argument("lists", help="the directory with symbols-<os>.txt of the symbols jobs")
    report_parser.add_argument("--abi-dir", default=str(ROOT / "abi"),
                               help="the lists of the change, which the CI lists must match")
    report_parser.add_argument("--base-dir", help="the lists of the base branch, against which the changes are shown "
                                                  "(default: --abi-dir)")
    report_parser.add_argument("--abi-report", help="the file of the abi job (ABI_REPORT of abi_check.sh)")
    report_parser.add_argument("--abi-baseline", choices=["found", "none", "unknown"],
                               help="the result of the abi_baseline job; without it and --abi-report: no ABI section")
    report_parser.add_argument("--limit", type=int, help="the maximum length of the report")
    report_parser.add_argument("--output", help="the file of the report (default: the standard output)")
    report_parser.add_argument("--github-output", help="append verdict=yes|no|unknown and comment=true|false")
    fetch_parser = commands.add_parser("fetch", help="take the lists of the latest CI run of HEAD into abi/")
    fetch_parser.add_argument("--run", type=int, help="the id of the CI run")
    fetch_parser.add_argument("--dry-run", action="store_true", help="only print the report")
    args = parser.parse_args(argv)

    try:
        if args.command == "report":
            lists = find_lists(args.lists)
            base_dir = args.base_dir or args.abi_dir
            differences = differences_of(lists, base_dir)
            if not differences:
                raise SymbolsError(f"{base_dir}: no lists symbols-<os>.txt")
            pending = [name for name, difference in differences_of(lists, args.abi_dir).items()
                       if difference is not None and difference.differs]
            abi = (AbiResult.from_ci(args.abi_report, args.abi_baseline) if args.abi_report or args.abi_baseline
                   else None)
            text = report(differences, abi, args.limit, ACCEPT_FROM_CI, pending)
            if args.output:
                pathlib.Path(args.output).write_text(text, encoding="utf-8", newline="\n")
            else:
                sys.stdout.write(text)
            if args.github_output:
                verdict = verdict_of(differences, abi)
                comment = verdict != "no" or bool(pending) or any(d is not None and d.differs
                                                                   for d in differences.values())
                with open(args.github_output, "a", encoding="utf-8") as output:
                    output.write(f"verdict={verdict}\ncomment={str(comment).lower()}\n")
            return 0
        if args.command == "fetch":
            if shutil.which("gh") is None:
                raise SymbolsError("gh (GitHub CLI) not found: install it and run gh auth login")
            print(fetch(args.run, args.dry_run))
            return 0

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
            print("or, after the CI run of the commit: python3 scripts/symbols.py fetch (Windows: py -3 instead of "
                  "python3)")
            return 1
        print(f"{list_path}: {len(own)} symbols match" + ("" if args.no_std_total else f", std-total {std_total}"))
        return 0
    except SymbolsError as error:
        print(f"symbols.py: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    sys.exit(main())
