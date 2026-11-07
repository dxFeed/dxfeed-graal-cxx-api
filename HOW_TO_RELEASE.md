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
`include/dxfeed_graal_cpp_api/detail/Conf.hpp` (`inline namespace vN`), move the unreleased items of
`release-notes/*.md` to `ReleaseNotes.md` and put the heading of the version above them, commit `vX.Y.Z` and tag it. Run them on the branch that is released (`main`, or `release/vN` for
a new major version) with a clean working tree. The same can be done by hand, see below. `v8.2.0` is an example.

### In one command

See what would change (nothing is changed):

```shell
cmake -P scripts/release.cmake v8.2.0 --dry-run
```

Release: the commit, the tag, then the branch and the tag are pushed (the tag last, it starts the workflow). If a push
fails, the script prints how to push the rest or, if nothing is pushed yet, how to undo the commit and the tag:

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

3. Push the branch, then the tag (the tag starts the workflow). The script prints both commands; for `release/vN`
   push that branch instead of `main`:

   ```shell
   git push origin main
   ```

   ```shell
   git push origin v8.2.0
   ```

### The unreleased items

Each pull request adds its item as a file of `release-notes/` (see `release-notes/README.md`). A release or a
pre-release moves them to the top of `ReleaseNotes.md` and removes the files in the commit `vX.Y.Z`: the items marked
`[BREAKING]` first, then the files named by a ticket (`MDAPI-455.md`), the newest first, then the others by name.
`--dry-run` lists them in this order. Items written at the top of `ReleaseNotes.md` itself (by a branch made before
`release-notes/`) stay above them.

### The heading of ReleaseNotes.md

The unreleased items are at the top of `ReleaseNotes.md`, above the first heading, with the items of `release-notes/`
after them. The heading of a version:

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

Run a draft before each release, on the commit the release is made from (the commit `vX.Y.Z` only adds the version
and the notes to it), and release only after a green draft run.
`vX.Y.Z-draftN` is a trial run of the `release` workflow: no commit, only the tag on the current commit (`--push`
pushes it):

```shell
cmake -P scripts/release.cmake v8.2.0-draft1 --push
```

The draft GitHub release shows the unreleased items (with those of `release-notes/`) as a preview of the notes; Jira
is not touched (only published releases are synchronized) and GitHub Pages are not updated. Remove the draft release
and its tag afterwards, also after a failed run (it has no release, but the tag is on origin). Each step says whether
it deleted or found nothing; the release needs the GitHub CLI (`gh`), without it the tags are deleted and the release,
if any, is deleted by hand:

```shell
cmake -P scripts/release.cmake v8.2.0-draft1 --cleanup
```

### What the workflow checks

The release is published only when all of these pass (the `publish_release` job needs them):

- the tests of each library build, before it is packed (`ctest` without the network tests, which the nightly runs of
  the `CI` workflow cover);
- `check_packages`: a CMake project finds each Release package and builds and runs programs with its libraries
  (`.github/ci/package-consumer-check`, on Windows also with the `-static-mt` package); the shared library exports the
  symbols of `abi/symbols-<os>.txt`, has its version in the name (the SONAME on Linux, the install name on macOS) and
  the hardening of a Release build (`.github/ci/library-check`);
- `check_static_mt`: the `-static-mt` packages need no Visual C++ Redistributable;
- `check_linux_packages`: the libraries, the samples and the tools of the Linux packages load on Oracle Linux 8 and
  Ubuntu 22.04, and the samples run from `bin/`;
- the ABI check (in the Oracle Linux 8 RelWithDebInfo build), see below.

GitHub Pages get the documentation after the release is published.

### The ABI check

`scripts/abi_check.sh` compares the ABI of the Linux x86-64 shared library (libabigail: the types and functions of the
public headers) with the asset `abi-x86_64-linux.xml` of the latest published release of the same major (not a draft,
not a pre-release). Each release publishes this asset for the next ones; until the first release of a major there is
nothing to compare with, and the check prints a notice. The `abi` job of the `CI` workflow runs the same check on every
change of the code, so a red check is usually seen in the pull request that causes it.

A removed or changed function, a new virtual function or a changed layout of a public type (its size, data members,
bases) fails the check; additions pass, with the report in the summary of the run. When it fails:

- the change is not intended: undo it, or make it compatible (a new function instead of a changed one);
- it is intended: it is a breaking change and goes to the next major version (`release/vN`);
- the report shows a change that is not the ABI of the library: add a suppression to `abi/abidiff.suppr`, with a
  comment that says why, in a pull request.

### A failed release run

The tag is on origin, the release is not published and GitHub Pages are not updated.

- A failure of the infrastructure (a runner, a download): rerun the failed jobs of the run (`gh run rerun <run id>
  --failed`, or "Re-run failed jobs" on the page of the run); the tag stays.
- A defect: fix it on the branch (a pull request, as usual) and run a draft on the fixed commit. Then move the tag of
  the release to the fixed commit, which starts the workflow again (the commit `vX.Y.Z` stays in the history):

  ```shell
  git fetch origin
  ```

  ```shell
  git push origin --delete v8.2.0
  ```

  ```shell
  git tag -f v8.2.0 origin/main
  ```

  ```shell
  git push origin v8.2.0
  ```

  For `release/vN` tag `origin/release/vN` instead of `origin/main`. The notes are those of `ReleaseNotes.md` at the
  tag; a release note added by the fix goes to the next version.

Never move the tag of a published release: it may be downloaded already. Release the next version instead.

### By hand

1. `ReleaseNotes.md`: add the items of `release-notes/*.md` after the unreleased items, in the order above, and
   remove these files (`git rm`; `release-notes/README.md` stays). Put `## v8.2.0` and an empty line above the
   unreleased items (for a release after a pre-release: replace the heading of the pre-release, see the table).
2. `CMakeLists.txt`: `set(DXFCXX_VERSION "v8.2.0" CACHE STRING ...)`.
3. `docs/Doxyfile`: `PROJECT_NUMBER = v8.2.0`.
4. For a new major version only: `include/dxfeed_graal_cpp_api/detail/Conf.hpp`: `inline namespace v9 {`.
5. Commit these files and the removal with the message `v8.2.0`, tag the commit (`git tag v8.2.0`), push the branch (`main` or
   `release/vN`), then the tag.

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

- Point the README links to the samples at their sources of the release on GitHub, as the release workflow does (the
  tag here is an example; this change is not committed):
```shell
sed -i "s#](samples/#](https://github.com/dxFeed/dxfeed-graal-cxx-api/blob/v8.2.0/samples/#g" README.md
```
- Go to the documentation folder:
```shell
cd docs
```
- Run Doxygen 1.18.0 (the version of the release workflow) and copy the README images next to the main page:
```shell
doxygen ./Doxyfile
mkdir -p html/docs && cp -r images html/docs/
```
- Copy the `html` folder somewhere
- Undo the README change:
```shell
git restore ../README.md
```
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
