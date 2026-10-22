* **\[MDAPI-433]\[C++]** Added `scripts/release.cmake` (CMake only) and `scripts/release.py` (Python), which prepare a
  release: they set the version in all the places and the heading of `ReleaseNotes.md` (for the pre-releases `-alphaN`,
  `-betaN`, `-preN` and `-rcN` too), commit and tag it, and with `--push` push the branch and the tag; `-draftN` is a
  trial run of the release workflow. The text of a GitHub release is now the section of its version in
  `ReleaseNotes.md`. See `HOW_TO_RELEASE.md`.
