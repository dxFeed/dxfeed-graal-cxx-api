# Copyright (c) 2026 Devexperts LLC.
# SPDX-License-Identifier: MPL-2.0

# Runs the check programs built by check.cmd: <dir>\<way>\check.exe for the ways dll, static and single. Each must
# print "OK:" and exit with 0 in the given time.
# Usage: run.ps1 <dir> [-TimeoutSeconds 60]
param(
    [Parameter(Mandatory = $true)][string]$Dir,
    [int]$TimeoutSeconds = 60
)

$failed = $false

foreach ($way in 'dll', 'static', 'single') {
    $wayDir = Join-Path $Dir $way
    $out = Join-Path $wayDir 'out.txt'
    $err = Join-Path $wayDir 'err.txt'
    $process = Start-Process (Join-Path $wayDir 'check.exe') -WorkingDirectory $wayDir -PassThru -NoNewWindow `
        -RedirectStandardOutput $out -RedirectStandardError $err
    $null = $process.Handle # Keeps the exit code available after the process exits.
    $deadline = (Get-Date).AddSeconds($TimeoutSeconds)

    while (-not $process.HasExited -and (Get-Date) -lt $deadline) {
        Start-Sleep -Milliseconds 200
    }

    if (-not $process.HasExited) {
        Stop-Process -Id $process.Id -Force -ErrorAction SilentlyContinue
        $result = "FAIL: did not exit in $TimeoutSeconds s"
    } elseif ($process.ExitCode -ne 0) {
        $result = "FAIL: exit code $($process.ExitCode)"
    } elseif (-not (Select-String -Path $out -Pattern '^OK:' -Quiet)) {
        $result = 'FAIL: no OK line'
    } else {
        $result = 'OK'
    }

    Write-Output "${way}: $result"
    Get-Content $out -ErrorAction SilentlyContinue | Select-String '^(OK|FAIL):' | ForEach-Object { "  $_" }

    if ($result -ne 'OK') {
        Get-Content $err -ErrorAction SilentlyContinue | Select-Object -Last 20 | ForEach-Object { "  $_" }
        $failed = $true
    }
}

# An explicit exit sets $LASTEXITCODE for the caller of "& run.ps1" (without it, $LASTEXITCODE stays as it was).
if ($failed) {
    exit 1
}

exit 0
