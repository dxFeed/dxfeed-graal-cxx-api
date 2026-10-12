# Releasing Portals

The release workflow builds an existing tag from its exact commit. It accepts
`vMAJOR.MINOR.PATCH` and `vMAJOR.MINOR.PATCH-rcN`, where `N` is positive.
The commit must already be included in `origin/main`. Neither the workflow nor
the publisher creates or moves tags.

## Release with one command

From a clean checkout, run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/release/start.ps1 -Tag v0.2.0
```

For a release candidate, use `v0.2.0-rc1` instead. PowerShell, Git, and an
authenticated GitHub CLI (`gh`) with access to the origin repository are required.
The local command does not require or invoke Python. With PowerShell 7 on Windows,
Linux or macOS, replace `powershell` with `pwsh`.
Alternatively, with Python 3.10 or newer:

```powershell
python tools/release/start.py v0.2.0
```

Both standalone entrypoints implement the same preparation contract. Python is
not a dependency of the PowerShell entrypoint. `-ExecutionPolicy Bypass` applies
only to this PowerShell process; the command does not change global policy.
The release workflows must already have been merged into `main`. Add `-DryRun` (PowerShell) or `--dry-run` (Python)
to validate and preview without changing local or remote refs or files. Preview
requires the current remote `main` commit to be available locally; fetch it first
if your checkout is stale. The preview itself does not fetch objects.

The command prepares the numeric CMake version and dates the release-notes
heading in an isolated worktree, commits only those two files, pushes a release
preparation branch, and opens a PR. Write release-note entries manually in the
single `Unreleased` section during development; the command preserves their
text. A stable release leaves an empty, unversioned `## Unreleased` section for
future entries, so developers need not choose the next version in advance.
Existing versioned `Unreleased` headings remain accepted. An RC keeps the working
section available for development and copies its
entries into a dated section for that exact RC. CMake keeps the numeric version,
while tags and archives retain the full RC suffix.

Review and merge the linked PR. Merging a same-repository
`task/release-v*` PR into `main` is the explicit release approval: Actions checks
the merged version and notes, creates the tag at the merge commit, and dispatches
the release workflow. No second terminal command is required. Closing the PR
without merging does not release anything.

The dispatcher uses `contents: write` to create the tag and `actions: write`
to start the release workflow. It dispatches explicitly because a tag created
with `GITHUB_TOKEN` does not itself trigger a push workflow; `workflow_dispatch`
is an exception to that restriction. See [GitHub token event rules](https://docs.github.com/en/actions/concepts/security/github_token).

The release workflow verifies Windows/Linux builds and package consumers before
publication. RC releases are prereleases and never become `latest`. A qualifying
stable release also updates documentation through Actions Pages.

## Existing tags and manual recovery

Tags are immutable release identities: never move an existing tag to another
commit. To rebuild an existing tag without publication:

```powershell
gh workflow run release.yml --ref main -f tag=v0.2.0-rc1 -F publish=false
gh run list --workflow release.yml
gh run download <run-id> --dir "C:/artifacts/portals-release-check"
```

For an explicitly approved retry of publication, use `-F publish=true`.
Manual runs default to build-only. A release-start workflow failure after merging
can be rerun from the Actions page; an existing tag must still identify the same
merge commit. The publisher verifies existing assets and refuses to replace
changed bytes. A pushed `v*` tag also remains a supported manual release trigger.

## Release assets and compatibility

The first release set contains:

- Windows x64 MSVC Release ZIP with four examples, installed static SDK and HTML.
  The SDK uses `MultiThreaded` (`/MT`); applications must use a compatible
  compiler, architecture, configuration and CRT. It is not interchangeable with
  an `/MD` SDK. The compiler version and DLL imports are recorded.
- Linux x64 GCC 14 Release tar.gz with the same payload, built on Ubuntu 24.04.
  The manifest records compiler, libc and executable dependencies. Compatible
  glibc and libstdc++ are required; this is not a universal Linux binary.
- Source ZIP and tar.gz produced by `git archive`, including vendored
  dependencies and licenses. A source consumer is built from the extracted
  archive with dependency downloading disabled. `.git`, private `.work` state
  and untracked build output are excluded.
- HTML documentation ZIP with Mermaid SVG and local resources, plus per-platform
  build manifests and final `release-manifest.json` and `SHA256SUMS`.

Archive names include the complete tag. Manifests record the exact commit,
configuration, compiler/runtime tuple and asset hashes. Hashes detect corruption;
they are not signatures. The publisher validates the tag again before writing
the release.

The published SDK is Release only. Debug is a required verification configuration,
not a second published SDK. There are no macOS/ARM packages, symbol archives or
external dxFeed SDK assets in this release set.

## Verification and documentation tools

Windows Server 2022/MSVC and Ubuntu 24.04/GCC 14 builders each run the full
headless CTest suite in Debug and Release, including installed-package consumers.
Only the interactive `portals.line-edit.native` test is excluded. Native terminal
acceptance remains a separate gate. After all validation and naming jobs pass,
separate packaging jobs call the existing `tools/package/bundle.cmake` directly.
It builds the tagged source snapshot, checks a freshly extracted bundle,
relocates the SDK, builds its consumer, and validates HTML links.
`tools/release/package.cmake` adds source/documentation archives and checked
platform manifests. Debug and Release test receipts identify the exact tag,
commit, platform and successful configuration used by those manifests.

The workflows pin Doxygen 1.18.0 official binaries by SHA-256 and Mermaid CLI
12.0.0. Node.js 24 supplies the established diagram toolchain. Python 3.12 is used
for the small metadata, publication and checking utilities; build orchestration
uses explicit CMake commands in YAML. Actions use full commit SHAs. Tool versions
are visible in workflow `env`, while Doxygen platform digests are in the package
matrix. These tool downloads require a network connection; vendored source
dependencies do not make toolchain or documentation generator installation
offline. Ordinary library builds do not require Doxygen or Mermaid.

## Selecting compilers and configurations

The `validation` matrix in `release.yml` explicitly lists runner OS, C and C++
compilers, a unique toolchain ID, Debug/Release configuration, MSVC CRT and Linux
toolchain packages. Artifact and receipt names include that toolchain ID, so a
new compiler entry does not collide with another compiler for the same OS.
Configure, build and CTest are separate workflow steps. To verify an additional
compiler, add its entries to this matrix and provide its runner setup/packages.
The naming checker keeps its own explicit Clang toolchain.

The `package` matrix is a separate publication baseline: Windows x64 MSVC `/MT`
and Ubuntu 24.04 x64 GCC 14 Release. Adding a validation compiler does not
automatically add another binary distribution. The existing runnable bundle
packager explicitly requires MSVC for the Windows C and C++ SDK; changing that
binary baseline requires deliberate packager support and compatibility checks.

For local builds, choose a project preset and override compilers with
`-DCMAKE_C_COMPILER=...` and `-DCMAKE_CXX_COMPILER=...`, or pass the same options
to a direct `cmake -S ... -B ...` configure command. Always choose a **new build
directory when changing toolchains**: CMake stores compiler selection and checks
in its build cache. Changing a matrix entry is straightforward; reusing an old
compiler cache is not.

To repeat the Windows gates and bundle locally, start an x64 MSVC developer PowerShell with
Git, CMake 3.24+, Ninja, Python 3.12+, Doxygen 1.18.0, Node.js 24 and Mermaid CLI
12.0.0 on PATH. Select an existing tag and a fresh work directory outside the
checkout:

```powershell
$releaseTag = 'v0.2.0-rc1'
$releaseCommit = git rev-parse --verify "refs/tags/$releaseTag`^{commit}"
git switch --detach $releaseCommit
python tools/release/metadata.py prepare --tag $releaseTag --repository ttldtor/portals --source . --output-dir "C:/artifacts/portals-metadata-run-001"
cmake --preset windows-msvc-debug -B "C:/temp/portals-validation-debug-run-001" -DCMAKE_C_COMPILER=cl -DCMAKE_CXX_COMPILER=cl -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreadedDebug
cmake --build "C:/temp/portals-validation-debug-run-001" --parallel 2
ctest --test-dir "C:/temp/portals-validation-debug-run-001" -C Debug --output-on-failure --no-tests=error -E '^portals\.line-edit\.native$'
cmake --preset windows-msvc-release -B "C:/temp/portals-validation-release-run-001" -DCMAKE_C_COMPILER=cl -DCMAKE_CXX_COMPILER=cl -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded
cmake --build "C:/temp/portals-validation-release-run-001" --parallel 2
ctest --test-dir "C:/temp/portals-validation-release-run-001" -C Release --output-on-failure --no-tests=error -E '^portals\.line-edit\.native$'
cmake "-DSOURCE_DIR=$((Get-Location).Path)" "-DSOURCE_REVISION=$releaseCommit" "-DWORK_DIR=C:/temp/portals-bundle-run-001" "-DOUTPUT_ARCHIVE=C:/artifacts/portals-$releaseTag-windows-x64-release.zip" "-DC_COMPILER=$((Get-Command cl).Source)" "-DCXX_COMPILER=$((Get-Command cl).Source)" "-DMSVC_RUNTIME_LIBRARY=MultiThreaded" "-DPYTHON_EXECUTABLE=$((Get-Command python).Source)" -P tools/package/bundle.cmake
```

Never reuse staging or output paths. Existing paths are refused and evidence is
retained. These commands create no GitHub release. For the complete source,
documentation and manifest set, use the manual build-only Actions run above.

## Publication, retries and latest

Build jobs have `contents: read`. Only the publication job has `contents: write`.
Publication is serialized globally so tags cannot race to change stable latest;
build concurrency is grouped by tag without cancelling a running build.

The publisher creates a draft, uploads the complete checked asset set, verifies
it, and publishes it. RC releases have `prerelease=true` and never become latest.
A stable version becomes latest only when its numeric version is newer than the
current stable latest. Releasing an older patch series preserves the newer
latest release.

A retry can resume an incomplete draft when its commit and existing asset bytes
match. An already published matching release is a verified no-op. A mismatch in
commit, notes, metadata, asset inventory or checksums stops the run; published
assets are never silently overwritten. Rebuilt binaries can differ between
runner images or toolchain updates, so a mismatch needs investigation rather
than automatic replacement. Retain the original verified Actions artifacts.

## GitHub Pages through Actions

The documentation site is deployed by `docs.yml` through an Actions artifact
and the `github-pages` environment; no `gh-pages` branch is used. Set the
repository's **Settings → Pages → Build and deployment → Source** to
**GitHub Actions**, and allow the release tag refs in the environment's deployment
policy if it restricts refs. This setup is an administrator action, not something
the workflow changes itself.

After a stable release becomes or remains current latest, `release.yml` calls
the reusable documentation workflow with the already verified Linux HTML artifact
and exact release commit. RC and older stable patch releases do not deploy.
The workflow checks current latest before preparing Pages and again immediately
before deployment. The deployment job alone receives `pages: write` and
`id-token: write` and targets `github-pages`.

To restore documentation, manually run `docs.yml` with the explicit stable tag
that is currently latest. It checks the tag and main ancestry, rebuilds the same
commit with the pinned tools, validates local links, and deploys via Actions:

```powershell
gh workflow run docs.yml --ref main -f tag=v0.2.0
```

The release uses a reusable workflow because events created with `GITHUB_TOKEN`
do not start another release-triggered workflow. A successful GitHub release
can therefore remain published even when a later Pages deployment fails; retry
documentation separately after resolving the Pages environment or site settings.

References:
[manual workflows](https://docs.github.com/en/actions/how-tos/manage-workflow-runs/manually-run-a-workflow),
[reusable workflows](https://docs.github.com/en/actions/how-tos/reuse-automations/reuse-workflows),
[GITHUB_TOKEN event restrictions](https://docs.github.com/en/actions/how-tos/write-workflows/choose-when-workflows-run/trigger-a-workflow),
[Pages custom workflows](https://docs.github.com/en/pages/getting-started-with-github-pages/using-custom-workflows-with-github-pages),
[release API](https://docs.github.com/en/rest/releases/releases),
[Doxygen 1.18.0 release](https://github.com/doxygen/doxygen/releases/tag/Release_1_18_0),
[Mermaid CLI](https://github.com/mermaid-js/mermaid-cli),
[MSVC runtime selection](https://cmake.org/cmake/help/latest/prop_tgt/MSVC_RUNTIME_LIBRARY.html).
