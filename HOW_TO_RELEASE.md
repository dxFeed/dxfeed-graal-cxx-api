# How To Release

## Release

A release is the commit `vX.Y.Z` with the version and the heading of `ReleaseNotes.md`, and the tag `vX.Y.Z` on it.
The tag starts the `release` workflow: it builds the packages with the version of the tag and publishes the GitHub
release; its text is the section of the version in `ReleaseNotes.md` (the workflow fails at the start if there is
none).

Two scripts prepare the release, with the same arguments and the same result; take the one that is convenient:

- `scripts/release.cmake`: CMake only (it is needed for the build anyway): `cmake -P scripts/release.cmake ...`
- `scripts/release.py`: Python 3: `python3 scripts/release.py ...` (`py -3` on Windows)

They set the version in `CMakeLists.txt` (`DXFCXX_VERSION`), `docs/Doxyfile` (`PROJECT_NUMBER`) and
`include/dxfeed_graal_cpp_api/internal/Conf.hpp` (`inline namespace vN`), put the heading of the version at the top
of `ReleaseNotes.md`, commit `vX.Y.Z` and tag it. Run them on the branch that is released (`main`, or `release/vN` for
a new major version) with a clean working tree. The same can be done by hand, see below. `v8.2.0` is an example.

### In one command

See what would change (nothing is changed):

```shell
cmake -P scripts/release.cmake v8.2.0 --dry-run
```

Release: the commit, the tag, then the branch and the tag are pushed (the tag last, it starts the workflow):

```shell
cmake -P scripts/release.cmake v8.2.0 --push
```

### Step by step

1. Prepare the release locally, without `--push`: the files are changed, committed and tagged:

   ```shell
   cmake -P scripts/release.cmake v8.2.0
   ```

2. Check the commit:

   ```shell
   git show
   ```

   If something is wrong, nothing is pushed yet: remove the tag and the commit, fix and run the script again:

   ```shell
   git tag -d v8.2.0
   ```

   ```shell
   git reset --keep HEAD~1
   ```

3. Push the branch, then the tag (the tag starts the workflow):

   ```shell
   git push origin main
   ```

   ```shell
   git push origin v8.2.0
   ```

### The heading of ReleaseNotes.md

The unreleased items are at the top of `ReleaseNotes.md`, above the first heading. The heading of a version:

| Version                                          | Top of `ReleaseNotes.md`                             | Result                      |
|--------------------------------------------------|------------------------------------------------------|-----------------------------|
| `vX.Y.Z-rc1` (also `-alphaN`, `-betaN`, `-preN`) | the unreleased items                                 | `## vX.Y.Z-rc1` is inserted |
| a later pre-release, `vX.Y.Z-rc2`                | `## vX.Y.Z-rc1`, maybe with the items added since it | the heading is replaced     |
| `vX.Y.Z`                                         | `## vX.Y.Z-rcN`, maybe with the items added since it | the heading is replaced     |
| `vX.Y.Z`                                         | the unreleased items                                 | `## vX.Y.Z` is inserted     |

Pre-releases are ordered alpha < beta < pre < rc, then by the number. When the heading is replaced, the items added
since the pre-release join its section. Anything else (an earlier or the same pre-release, an older version, no
unreleased items) is an error.

### Draft

`vX.Y.Z-draftN` is a trial run of the `release` workflow: no commit, only the tag on the current commit (`--push`
pushes it). The draft GitHub release shows the unreleased items as a preview of the notes; Jira is not touched (only
published releases are synchronized). Remove the draft release and its tag afterwards (needs the GitHub CLI):

```shell
cmake -P scripts/release.cmake v8.2.0-draft1 --cleanup
```

### By hand

1. `ReleaseNotes.md`: put `## v8.2.0` and an empty line above the unreleased items (for a release after a
   pre-release: replace the heading of the pre-release, see the table).
2. `CMakeLists.txt`: `set(DXFCXX_VERSION "v8.2.0" CACHE STRING ...)`.
3. `docs/Doxyfile`: `PROJECT_NUMBER = v8.2.0`.
4. For a new major version only: `include/dxfeed_graal_cpp_api/internal/Conf.hpp`: `inline namespace v9 {`.
5. Commit these files with the message `v8.2.0`, tag the commit (`git tag v8.2.0`), push the branch, then the tag.

The notes of a version, as the workflow takes them: `cmake -P scripts/release.cmake v8.2.0 --notes`.

### Tests of the scripts

```shell
python3 -m unittest discover -s scripts/tests
```

```shell
cmake -P scripts/tests/test_release.cmake
```

The first one also runs `release.cmake` in a git repository and checks that both scripts give the same result.

### Build the documentation

- Go to the documentation folder:
```shell
cd docs
```
- Run the doxygen
```shell
doxygen ./Doxyfile
```
- Copy the `html` folder somewhere
- Switch to `gh-pages` git branch
```shell
git switch gh-pages 
```
- Copy contents of the "saved" `html` folder to `docs` folder
- Add the changes:
```shell
git add .
```
- Commit the changes:
```shell
git commit -m "PreRelease v0.1.0-alpha"
```
or
```shell
git commit -m "Release v1.1.0"
```
- Push the changes:
```shell
git push
```
