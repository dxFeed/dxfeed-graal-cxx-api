# Copyright (c) 2026 ttldtor.
# SPDX-License-Identifier: BSL-1.0
[CmdletBinding()]
param(
    [ValidateSet('dashboard', 'gallery', 'custom_widget', 'replay')]
    [string] $Example = 'dashboard',
    [Parameter(ValueFromRemainingArguments)] [string[]] $ExampleArguments
)
$programs = @{
    dashboard = 'portals_dashboard.exe'
    gallery = 'portals_gallery.exe'
    custom_widget = 'portals_custom_widget.exe'
    replay = 'portals_replay_example.exe'
}
$program = Join-Path $PSScriptRoot ('bin/' + $programs[$Example])
if (-not (Test-Path -LiteralPath $program -PathType Leaf)) { throw "Example executable is missing: $program" }
& $program @ExampleArguments
exit $LASTEXITCODE
