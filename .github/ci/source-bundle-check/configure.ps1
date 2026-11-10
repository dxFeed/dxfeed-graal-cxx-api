# Copyright (c) 2026 Devexperts LLC.
# SPDX-License-Identifier: MPL-2.0

# Configures an unpacked Full Source Bundle for Windows x64 offline and checks the result: the configuration must
# succeed (or fail with -ExpectFailure), and each -Match regular expression must match its output.
# Usage: configure.ps1 <source dir> <build dir> [-ExpectFailure] [-Match <regex>,...] [-Options <cmake option>,...]
param(
    [Parameter(Mandatory = $true)][string]$Source,
    [Parameter(Mandatory = $true)][string]$Build,
    [switch]$ExpectFailure,
    [string[]]$Match = @(),
    [string[]]$Options = @()
)

$output = & cmake -S $Source -B $Build -A x64 -DDXFCXX_BUILD_DOC=OFF -DFETCHCONTENT_FULLY_DISCONNECTED=ON @Options 2>&1 |
    Out-String
$exitCode = $LASTEXITCODE
Write-Output $output
$description = "cmake $($Options -join ' ')"

if ($ExpectFailure -and $exitCode -eq 0) {
    Write-Output "FAIL: $description succeeded"
    exit 1
}

if (-not $ExpectFailure -and $exitCode -ne 0) {
    Write-Output "FAIL: $description failed with $exitCode"
    exit 1
}

foreach ($regex in $Match) {
    if ($output -notmatch $regex) {
        Write-Output "FAIL: the output of $description has no '$regex'"
        exit 1
    }
}

Write-Output "OK: $description"
exit 0
