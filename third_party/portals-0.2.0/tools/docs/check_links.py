# Copyright (c) 2026 ttldtor.
# SPDX-License-Identifier: BSL-1.0
"""Validate generated HTML file and anchor links; optional Python 3 gate."""
import json
import sys
from html.parser import HTMLParser
from pathlib import Path
from urllib.parse import urlsplit, unquote

if len(sys.argv) != 2 or not Path(sys.argv[1]).is_dir():
    raise SystemExit("Usage: python tools/docs/check_links.py <generated-html-directory>")
root = Path(sys.argv[1]).resolve()
class Page(HTMLParser):
    def __init__(self):
        super().__init__()
        self.links = []
        self.ids = set()
    def handle_starttag(self, tag, attrs):
        attrs = dict(attrs)
        if "id" in attrs: self.ids.add(attrs["id"])
        if tag == "a" and "name" in attrs: self.ids.add(attrs["name"])
        for key in ("href", "src"):
            if key in attrs: self.links.append(attrs[key])

pages = {}
for file in root.rglob("*.html"):
    page = Page()
    page.feed(file.read_text(encoding="utf-8"))
    pages[file.resolve()] = page
if not pages:
    raise SystemExit('No HTML files found; generate documentation before checking links')

bad = []
checked = 0
for file, page in pages.items():
    for link in page.links:
        url = urlsplit(link)
        if url.scheme or url.netloc or link.startswith("javascript:"): continue
        target = (file.parent / unquote(url.path)).resolve() if url.path else file
        checked += 1
        if not target.exists(): bad.append([str(file.relative_to(root)), link, "missing file"])
        elif url.fragment and target in pages and unquote(url.fragment) not in pages[target].ids:
            bad.append([str(file.relative_to(root)), link, "missing anchor"])
print(json.dumps({"html_files":len(pages), "local_links_checked":checked, "broken":bad}, indent=2))
raise SystemExit(bool(bad))

