# Copyright (c) 2026 ttldtor.
# SPDX-License-Identifier: BSL-1.0
# Standalone release preparation for Windows PowerShell 5.1 and PowerShell 7.

<#
.SYNOPSIS
Prepare a release pull request without merging it or publishing a release.
.PARAMETER Tag
Release identity: vMAJOR.MINOR.PATCH or vMAJOR.MINOR.PATCH-rcN.
.PARAMETER DryRun
Validate and preview without writing files or refs. The remote main commit
must already be available locally; preview does not fetch missing objects.
.EXAMPLE
powershell -NoProfile -ExecutionPolicy Bypass -File tools/release/start.ps1 -Tag v0.2.0
#>
[CmdletBinding()]
param(
    [Parameter(Position = 0)][string]$Tag,
    [switch]$DryRun
)

$ErrorActionPreference = 'Stop'
$script:ReleaseFiles = @('CMakeLists.txt', 'ReleaseNotes.md')
$script:TagPattern = '\Av(?<base>(?:0|[1-9][0-9]*)\.(?:0|[1-9][0-9]*)\.(?:0|[1-9][0-9]*))(?<rc>-rc[1-9][0-9]*)?\z'

function Invoke-ReleaseProcess {
    param([string[]]$Command, [string]$Root)
    $executable = (Get-Command $Command[0] -CommandType Application -ErrorAction Stop | Select-Object -First 1).Source
    $quoted = foreach ($argument in $Command[1..($Command.Count - 1)]) {
        '"' + [regex]::Replace([regex]::Replace($argument, '(\\*)"', '$1$1\"'), '(\\+)\z', '$1$1') + '"'
    }
    $info = New-Object System.Diagnostics.ProcessStartInfo
    $info.FileName = $executable
    $info.Arguments = $quoted -join ' '
    $info.WorkingDirectory = $Root
    $info.UseShellExecute = $false
    $info.CreateNoWindow = $true
    $info.RedirectStandardOutput = $true
    $info.RedirectStandardError = $true
    $info.StandardOutputEncoding = New-Object System.Text.UTF8Encoding($false)
    $info.StandardErrorEncoding = New-Object System.Text.UTF8Encoding($false)
    $info.EnvironmentVariables['GIT_OPTIONAL_LOCKS'] = '0'
    $process = New-Object System.Diagnostics.Process
    $process.StartInfo = $info
    try {
        [void]$process.Start()
        $stderr = $process.StandardError.ReadToEndAsync()
        $stdout = $process.StandardOutput.ReadToEnd()
        $process.WaitForExit()
        if ($process.ExitCode -ne 0) { throw "$($Command[0]) failed: $($stderr.Result.Trim())" }
        return $stdout
    } finally { $process.Dispose() }
}

function Invoke-ReleaseGit {
    param([string]$Root, [string[]]$Arguments)
    return (Invoke-ReleaseProcess -Command (@('git') + $Arguments) -Root $Root).Trim()
}

function Get-ReleaseTag {
    param([string]$Value)
    $match = [regex]::Match($Value, $script:TagPattern)
    if (-not $match.Success) { throw 'Tag must be vX.Y.Z or vX.Y.Z-rcN, without leading zeros' }
    return $match
}

function Get-ReleaseRepository {
    param([string]$Url)
    $match = [regex]::Match($Url, '\A(?:https://github\.com/|ssh://git@github\.com/|git@github\.com:)([A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+?)(?:\.git)?/?\z')
    if (-not $match.Success) { throw 'origin must identify a GitHub repository over HTTPS or SSH' }
    return $match.Groups[1].Value
}

function Get-ReleaseVersion {
    param([string]$CMake)
    $uncommented = [regex]::Replace($CMake, '#[^\n]*', { param($match) ' ' * $match.Length })
    $versions = @()
    foreach ($project in [regex]::Matches($uncommented, '\bproject\s*\(([^)]*)\)', 'IgnoreCase')) {
        foreach ($version in [regex]::Matches($project.Groups[1].Value, '\bVERSION\s+([0-9]+\.[0-9]+\.[0-9]+)\b', 'IgnoreCase')) {
            $versions += [pscustomobject]@{ Start = $project.Groups[1].Index + $version.Groups[1].Index; Length = $version.Groups[1].Length; Value = $version.Groups[1].Value }
        }
    }
    if ($versions.Count -ne 1) { throw 'CMakeLists.txt must contain one numeric project VERSION' }
    return $versions[0]
}

function Get-ReleaseSections {
    param([string]$Notes)
    $fenceCharacter = ''
    $fenceLength = 0
    foreach ($line in [regex]::Matches($Notes, '[^\r\n]*(?:\r\n|\n|\r|\z)')) {
        if ($line.Length -eq 0) { continue }
        $text = $line.Value.TrimEnd([char[]]"`r`n")
        $fence = [regex]::Match($text, '^ {0,3}(`{3,}|~{3,})')
        if ($fenceCharacter) {
            if ($fence.Success -and $fence.Groups[1].Value[0].ToString() -eq $fenceCharacter -and $fence.Groups[1].Length -ge $fenceLength -and -not $text.Substring($fence.Length).Trim()) { $fenceCharacter = '' }
            continue
        }
        if ($fence.Success) {
            $fenceCharacter = $fence.Groups[1].Value[0].ToString()
            $fenceLength = $fence.Groups[1].Length
            continue
        }
        $heading = [regex]::Match($text, '\A##[ \t]+(.+?)[ \t]*#*[ \t]*\z')
        if ($heading.Success) {
            [pscustomobject]@{ Start = $line.Index; End = $line.Index + $line.Length; Heading = $heading.Groups[1].Value; Line = $line.Value }
        }
    }
}

function Assert-ReleaseBody {
    param([string]$Body)
    $hasContent = @($Body -split '\r?\n' | Where-Object { $_.Trim() -and -not $_.TrimStart().StartsWith('#') }).Count -gt 0
    if (-not $Body.Trim() -or -not $hasContent) { throw 'Empty release notes section' }
}

function Get-PreparedReleaseFiles {
    param([string]$CMake, [string]$Notes, [string]$ReleaseTag, [string]$ReleaseDate)
    $match = Get-ReleaseTag $ReleaseTag
    [void][datetime]::ParseExact($ReleaseDate, 'yyyy-MM-dd', [cultureinfo]::InvariantCulture)
    $sections = @(Get-ReleaseSections $Notes)
    $unreleased = @($sections | Where-Object { $_.Heading -match '\bUnreleased\b' })
    if ($unreleased.Count -ne 1 -or $sections[0].Start -ne $unreleased[0].Start) { throw 'Release notes must have one Unreleased section, before all released sections' }
    $source = $unreleased[0]
    $sourceTag = ($source.Heading -split '\s+')[0]
    $version = Get-ReleaseVersion $CMake
    $generic = $source.Heading -ieq 'Unreleased'
    if (-not $generic) {
        $sourceMatch = Get-ReleaseTag $sourceTag
        if ($sourceMatch.Groups['rc'].Success) { throw 'Unreleased must describe a numeric base version, not an RC' }
        if ($version.Value -ne $sourceMatch.Groups['base'].Value) { throw 'CMake project VERSION does not match the Unreleased base version' }
        if (@($sections | Where-Object { ($_.Heading -split '\s+')[0] -eq $sourceTag }).Count -ne 1) { throw 'Duplicate release notes sections' }
    }
    $base = $match.Groups['base'].Value
    foreach ($section in $sections | Select-Object -Skip 1) {
        if (($section.Heading -split '\s+')[0] -in @($ReleaseTag, "v$base")) { throw 'The requested release already has a notes section' }
    }
    $bodyEnd = $Notes.Length
    if ($sections.Count -gt 1) { $bodyEnd = $sections[1].Start }
    $body = $Notes.Substring($source.End, $bodyEnd - $source.End)
    Assert-ReleaseBody $body
    $newline = "`n"
    if ($source.Line.EndsWith("`r`n")) { $newline = "`r`n" }
    $datedHeading = "## $ReleaseTag ($ReleaseDate)$newline"
    if ($match.Groups['rc'].Success) {
        $baseHeading = $source.Line
        if (-not $generic) { $baseHeading = $source.Line.Replace($sourceTag, "v$base") }
        $updatedNotes = $Notes.Substring(0, $source.Start) + $baseHeading + $body + $datedHeading + $body + $Notes.Substring($bodyEnd)
    } else {
        $updatedNotes = $Notes.Substring(0, $source.Start) + "## Unreleased$newline$newline" + $datedHeading + $Notes.Substring($source.End)
    }
    return @{ 'CMakeLists.txt' = $CMake.Substring(0, $version.Start) + $base + $CMake.Substring($version.Start + $version.Length); 'ReleaseNotes.md' = $updatedNotes }
}

function Get-ReleaseFilesAt {
    param([string]$Root, [string]$Commit)
    $files = @{}
    foreach ($name in $script:ReleaseFiles) { $files[$name] = Invoke-ReleaseProcess -Command @('git', 'show', "${Commit}:$name") -Root $Root }
    return $files
}

function Assert-PreparedRelease {
    param([hashtable]$Files, [string]$ReleaseTag)
    $tagMatch = Get-ReleaseTag $ReleaseTag
    if ((Get-ReleaseVersion $Files['CMakeLists.txt']).Value -ne $tagMatch.Groups['base'].Value) { throw 'Prepared CMake project VERSION does not match the requested tag' }
    $sections = @(Get-ReleaseSections $Files['ReleaseNotes.md'])
    $selected = @($sections | Where-Object { ($_.Heading -split '\s+')[0] -eq $ReleaseTag })
    if ($selected.Count -ne 1) { throw 'Prepared notes must have one exact section for the requested tag' }
    $dated = [regex]::Match($selected[0].Heading, '\A' + [regex]::Escape($ReleaseTag) + ' \((\d{4}-\d{2}-\d{2})\)\z')
    if (-not $dated.Success) { throw 'Prepared notes must have an exact dated section for the requested tag' }
    [void][datetime]::ParseExact($dated.Groups[1].Value, 'yyyy-MM-dd', [cultureinfo]::InvariantCulture)
    $end = $Files['ReleaseNotes.md'].Length
    $next = @($sections | Where-Object { $_.Start -gt $selected[0].Start })
    if ($next.Count) { $end = $next[0].Start }
    Assert-ReleaseBody $Files['ReleaseNotes.md'].Substring($selected[0].End, $end - $selected[0].End)
    return $dated.Groups[1].Value
}

function Assert-ReleaseBranch {
    param([string]$Root, [string]$Commit, [string]$MainCommit, [string]$ReleaseTag)
    $parents = (Invoke-ReleaseGit $Root @('rev-list', '--parents', '-n', '1', $Commit)) -split '\s+'
    if ($parents.Count -ne 2) { throw 'Preparation branch must contain a single release preparation commit' }
    [void](Invoke-ReleaseGit $Root @('merge-base', '--is-ancestor', $parents[1], $MainCommit))
    $changed = (Invoke-ReleaseGit $Root @('diff', '--name-only', $parents[1], $Commit)) -split '\r?\n'
    if (-not $changed[0] -or @($changed | Where-Object { $_ -notin $script:ReleaseFiles }).Count) { throw 'Preparation commit may change only CMakeLists.txt and ReleaseNotes.md' }
    $actual = Get-ReleaseFilesAt $Root $Commit
    $releaseDate = Assert-PreparedRelease $actual $ReleaseTag
    $original = Get-ReleaseFilesAt $Root $parents[1]
    $expected = Get-PreparedReleaseFiles $original['CMakeLists.txt'] $original['ReleaseNotes.md'] $ReleaseTag $releaseDate
    foreach ($name in $script:ReleaseFiles) { if ($actual[$name] -cne $expected[$name]) { throw 'Existing preparation branch differs from the requested version and unchanged notes body' } }
}

function Start-Release {
    param([string]$Root, [string]$ReleaseTag, [switch]$Preview)
    [void](Get-ReleaseTag $ReleaseTag)
    $Root = Invoke-ReleaseGit $Root @('rev-parse', '--show-toplevel')
    if (Invoke-ReleaseGit $Root @('status', '--porcelain', '--untracked-files=all')) { throw 'Working tree must be clean before release preparation' }
    $repository = Get-ReleaseRepository (Invoke-ReleaseGit $Root @('remote', 'get-url', 'origin'))
    $pushUrls = @((Invoke-ReleaseGit $Root @('remote', 'get-url', '--push', '--all', 'origin')) -split '\r?\n')
    if ($pushUrls.Count -ne 1 -or (Get-ReleaseRepository $pushUrls[0]) -ine $repository) { throw 'origin must have exactly one push URL for the same GitHub repository' }
    $branch = "task/release-$ReleaseTag"
    $refs = @{}
    $remoteLines = Invoke-ReleaseGit $Root @('ls-remote', 'origin', 'refs/heads/main', "refs/heads/$branch", "refs/tags/$ReleaseTag", "refs/tags/$ReleaseTag^{}")
    foreach ($line in $remoteLines -split '\r?\n') { if ($line) { $parts = $line -split '\s+'; $refs[$parts[1]] = $parts[0] } }
    $mainCommit = $refs['refs/heads/main']
    if (-not $mainCommit) { throw 'origin has no main branch' }
    $tagCommit = $refs["refs/tags/$ReleaseTag^{}"]
    if (-not $tagCommit) { $tagCommit = $refs["refs/tags/$ReleaseTag"] }
    $remoteBranch = $refs["refs/heads/$branch"]
    $localBranch = Invoke-ReleaseGit $Root @('for-each-ref', '--format=%(objectname)', "refs/heads/$branch")
    if ($remoteBranch -and $localBranch -and $remoteBranch -ne $localBranch) { throw 'Local and remote preparation branches differ; refusing to overwrite either' }
    if ($Preview) {
        if ($remoteBranch -or $localBranch -or $tagCommit) { throw 'Release refs already exist; use the normal command to validate and resume' }
        $source = Get-ReleaseFilesAt $Root $mainCommit
        $prepared = Get-PreparedReleaseFiles $source['CMakeLists.txt'] $source['ReleaseNotes.md'] $ReleaseTag (Get-Date -Format 'yyyy-MM-dd')
        $changed = @($script:ReleaseFiles | Where-Object { $source[$_] -cne $prepared[$_] })
        return (@{ tag = $ReleaseTag; branch = $branch; base = 'main'; main_commit = $mainCommit; changed_files = $changed; dry_run = $true } | ConvertTo-Json)
    }
    [void](Invoke-ReleaseProcess -Command @('gh', 'auth', 'status', '--hostname', 'github.com') -Root $Root)
    [void](Invoke-ReleaseGit $Root @('fetch', '--no-tags', 'origin', 'refs/heads/main'))
    if ((Invoke-ReleaseGit $Root @('rev-parse', 'FETCH_HEAD')) -ne $mainCommit) { throw 'origin/main changed during preparation; retry the command' }
    $prJson = Invoke-ReleaseProcess -Command @('gh', 'pr', 'list', '--repo', $repository, '--head', $branch, '--base', 'main', '--state', 'all', '--json', 'url,state,headRefOid,mergeCommit,isCrossRepository') -Root $Root
    $parsedPrs = ConvertFrom-Json -InputObject $prJson
    $prs = @($parsedPrs)
    if ($prs.Count -gt 1) { throw 'Multiple release preparation PRs exist; inspect them before retrying' }
    $pr = $null
    if ($prs.Count) { $pr = $prs[0] }
    if ($pr -and $pr.isCrossRepository -ne $false) { throw 'Preparation PR must originate from the same repository' }
    if ($pr -and $pr.state -eq 'CLOSED') { throw 'Preparation PR was closed without merging; inspect it before retrying' }
    if ($pr -and $pr.state -eq 'MERGED') {
        $merged = $pr.mergeCommit.oid
        if ($tagCommit -and $tagCommit -ne $merged) { throw 'Existing release tag does not point to the PR merge commit' }
        [void](Invoke-ReleaseGit $Root @('fetch', '--no-tags', 'origin', $merged))
        [void](Invoke-ReleaseGit $Root @('merge-base', '--is-ancestor', $merged, $mainCommit))
        [void](Assert-PreparedRelease (Get-ReleaseFilesAt $Root $merged) $ReleaseTag)
        return $pr.url
    }
    if ($tagCommit) { throw 'Release tag already exists without a matching merged preparation PR' }
    if ($remoteBranch) {
        [void](Invoke-ReleaseGit $Root @('fetch', '--no-tags', 'origin', "refs/heads/$branch"))
        if ((Invoke-ReleaseGit $Root @('rev-parse', 'FETCH_HEAD')) -ne $remoteBranch) { throw 'Remote preparation branch changed; retry the command' }
    }
    $commit = $remoteBranch
    if (-not $commit) { $commit = $localBranch }
    if ($commit) { Assert-ReleaseBranch $Root $commit $mainCommit $ReleaseTag }
    if ($pr) {
        if ($pr.headRefOid -ne $commit) { throw 'Preparation PR head does not match the validated branch' }
        return $pr.url
    }
    $prepared = $null
    if (-not $commit) {
        $source = Get-ReleaseFilesAt $Root $mainCommit
        $prepared = Get-PreparedReleaseFiles $source['CMakeLists.txt'] $source['ReleaseNotes.md'] $ReleaseTag (Get-Date -Format 'yyyy-MM-dd')
    }
    $temporary = Join-Path ([IO.Path]::GetTempPath()) ('portals-release-' + [guid]::NewGuid().ToString('N'))
    [void][IO.Directory]::CreateDirectory($temporary)
    $worktree = Join-Path $temporary 'worktree'
    $bodyPath = Join-Path $temporary 'pr-body.md'
    $utf8 = New-Object System.Text.UTF8Encoding($false)
    try {
        if ($localBranch) { [void](Invoke-ReleaseGit $Root @('worktree', 'add', $worktree, $branch)) }
        else {
            $baseCommit = $mainCommit
            if ($commit) { $baseCommit = $commit }
            [void](Invoke-ReleaseGit $Root @('worktree', 'add', '-b', $branch, $worktree, $baseCommit))
        }
        if (-not $commit) {
            foreach ($name in $script:ReleaseFiles) { [IO.File]::WriteAllText((Join-Path $worktree $name), $prepared[$name], $utf8) }
            [void](Invoke-ReleaseGit $worktree (@('add', '--') + $script:ReleaseFiles))
            [void](Invoke-ReleaseGit $worktree (@('commit', '--only', '-m', "Prepare release $ReleaseTag", '--') + $script:ReleaseFiles))
            $commit = Invoke-ReleaseGit $worktree @('rev-parse', 'HEAD')
            Assert-ReleaseBranch $Root $commit $mainCommit $ReleaseTag
        }
        [void](Invoke-ReleaseGit $worktree @('push', 'origin', "HEAD:refs/heads/$branch"))
        $body = "Prepare ${ReleaseTag}: update the numeric CMake project version and date the release notes. The release notes body is preserved.`n`nReview and merge this PR into main to create the release tag and start the verified release publication workflow. Merging this PR authorizes tag creation and release publication; this command does not merge it.`n"
        [IO.File]::WriteAllText($bodyPath, $body, $utf8)
        return (Invoke-ReleaseProcess -Command @('gh', 'pr', 'create', '--repo', $repository, '--head', $branch, '--base', 'main', '--title', "Prepare release $ReleaseTag", '--body-file', $bodyPath) -Root $Root).Trim()
    } finally {
        if (Test-Path -LiteralPath $worktree) {
            try {
                $dirty = Invoke-ReleaseGit $worktree @('status', '--porcelain', '--untracked-files=all')
                $ignored = Invoke-ReleaseGit $worktree @('ls-files', '--others', '--ignored', '--exclude-standard')
                if ($dirty -or $ignored) { [Console]::Error.WriteLine("Recovery worktree preserved: $worktree") }
                else { [void](Invoke-ReleaseGit $Root @('worktree', 'remove', $worktree)) }
            } catch { [Console]::Error.WriteLine("Recovery worktree preserved: $worktree") }
        }
        if (Test-Path -LiteralPath $bodyPath) { [IO.File]::Delete($bodyPath) }
        if (@([IO.Directory]::EnumerateFileSystemEntries($temporary)).Count -eq 0) { [IO.Directory]::Delete($temporary) }
    }
}

if ($MyInvocation.InvocationName -ne '.') {
    try { Start-Release -Root (Get-Location).Path -ReleaseTag $Tag -Preview:$DryRun }
    catch { [Console]::Error.WriteLine("Release start error: $($_.Exception.Message)"); exit 1 }
}
