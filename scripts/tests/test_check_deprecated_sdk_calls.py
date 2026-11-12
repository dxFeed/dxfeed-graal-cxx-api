# Copyright (c) 2026 Devexperts LLC.
# SPDX-License-Identifier: MPL-2.0

"""Tests of scripts/check_deprecated_sdk_calls.py: python3 -m unittest discover -s scripts/tests"""

import contextlib
import io
import json
import pathlib
import shutil
import sys
import tempfile
import unittest

SCRIPTS = pathlib.Path(__file__).resolve().parent.parent
sys.path.insert(0, str(SCRIPTS))

import check_deprecated_sdk_calls as check  # noqa: E402

API = {
    "schema_version": 1,
    "functions": [
        {
            "name": "dxfg_DXFeedSubscription_close",
            "deprecated": {
                "message": "use dxfg_v4_DXFeedSubscription_close()",
                "replacements": ["dxfg_v4_DXFeedSubscription_close"],
            },
        },
        {"name": "dxfg_v4_DXFeedSubscription_close"},
        {"name": "dxfg_Metrics_dump"},
    ],
    "types": [{"name": "dxfg_quote_t"}],
}


class CheckDeprecatedSdkCallsTest(unittest.TestCase):
    def setUp(self):
        self.directory = pathlib.Path(tempfile.mkdtemp())
        self.addCleanup(shutil.rmtree, self.directory)
        self.api = self.directory / "dxfg-api.json"
        self.api.write_text(json.dumps(API), encoding="utf-8")
        self.sources = self.directory / "src"
        self.sources.mkdir()

    def run_check(self, *paths):
        out, err = io.StringIO(), io.StringIO()

        with contextlib.redirect_stdout(out), contextlib.redirect_stderr(err):
            code = check.main(["check_deprecated_sdk_calls.py", str(self.api), *map(str, paths)])

        return code, out.getvalue(), err.getvalue()

    def test_a_call_of_a_deprecated_function_fails_with_the_file_and_the_line(self):
        source = self.sources / "Sub.cpp"
        source.write_text("void f() {\n    dxfg_DXFeedSubscription_close(thread, sub);\n}\n", encoding="utf-8")

        code, out, _ = self.run_check(self.sources)

        self.assertEqual(code, 1)
        self.assertIn("Sub.cpp:2: deprecated SDK function dxfg_DXFeedSubscription_close", out)
        self.assertIn("use dxfg_v4_DXFeedSubscription_close", out)

    def test_a_comment_is_checked_too(self):
        (self.sources / "Sub.hpp").write_text("// calls dxfg_DXFeedSubscription_close\n", encoding="utf-8")

        code, _, _ = self.run_check(self.sources)

        self.assertEqual(code, 1)

    def test_types_v4_functions_and_longer_identifiers_pass(self):
        (self.sources / "Ok.cpp").write_text(
            "dxfg_quote_t *quote{};\n"
            "dxfg_v4_DXFeedSubscription_close(thread, sub);\n"
            "dxfg_DXFeedSubscription_closeAll(thread);\n"
            "my_dxfg_DXFeedSubscription_close();\n"
            "dxfg_Metrics_dump(thread, &text);\n",
            encoding="utf-8",
        )

        code, out, _ = self.run_check(self.sources)

        self.assertEqual(code, 0, out)
        self.assertEqual(out, "")

    def test_files_of_other_types_are_skipped(self):
        (self.sources / "notes.md").write_text("dxfg_DXFeedSubscription_close\n", encoding="utf-8")

        code, _, _ = self.run_check(self.sources)

        self.assertEqual(code, 0)

    def test_a_file_can_be_given_directly(self):
        source = self.sources / "Sub.c"
        source.write_text("dxfg_DXFeedSubscription_close(thread, sub);\n", encoding="utf-8")

        code, _, _ = self.run_check(source)

        self.assertEqual(code, 1)

    def test_a_missing_path_is_an_error(self):
        code, _, err = self.run_check(self.directory / "missing")

        self.assertEqual(code, 2)
        self.assertIn("no such file or directory", err)

    def test_an_api_without_deprecated_functions_is_an_error(self):
        self.api.write_text(json.dumps({"functions": [{"name": "dxfg_v4_DXFeedSubscription_close"}]}), encoding="utf-8")

        code, _, err = self.run_check(self.sources)

        self.assertEqual(code, 2)
        self.assertIn("lists no deprecated function", err)

    def test_an_unreadable_api_is_an_error(self):
        self.api.write_text("{", encoding="utf-8")

        code, _, err = self.run_check(self.sources)

        self.assertEqual(code, 2)
        self.assertIn("cannot read", err)


if __name__ == "__main__":
    unittest.main()
