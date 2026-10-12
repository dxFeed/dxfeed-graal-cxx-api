# Copyright (c) 2026 ttldtor.
# SPDX-License-Identifier: BSL-1.0
[CmdletBinding()]
param(
    [string] $SourceDir = (Join-Path $PSScriptRoot '../..'),
    [string] $Revision = 'HEAD',
    [Parameter(Mandatory)] [string] $WorkDir,
    [Parameter(Mandatory)] [string] $OutputArchive,
    [string] $Generator = 'Ninja',
    [string] $CxxCompiler,
    [string] $CCompiler,
    [string] $PythonExecutable,
    [ValidateSet('MultiThreaded', 'MultiThreadedDLL')] [string] $Runtime = 'MultiThreaded',
    [ValidateRange(1, 1024)] [int] $Parallel = 2,
    [string] $CMake = 'cmake'
)
$ErrorActionPreference = 'Stop'
$arguments = @(
    "-DSOURCE_DIR=$([IO.Path]::GetFullPath($SourceDir))",
    "-DSOURCE_REVISION=$Revision",
    "-DWORK_DIR=$([IO.Path]::GetFullPath($WorkDir))",
    "-DOUTPUT_ARCHIVE=$([IO.Path]::GetFullPath($OutputArchive))",
    "-DGENERATOR=$Generator",
    "-DMSVC_RUNTIME_LIBRARY=$Runtime",
    "-DPARALLEL=$Parallel"
)
if ($CxxCompiler) { $arguments += "-DCXX_COMPILER=$CxxCompiler" }
if ($CCompiler) { $arguments += "-DC_COMPILER=$CCompiler" }
if ($PythonExecutable) { $arguments += "-DPYTHON_EXECUTABLE=$PythonExecutable" }
$arguments += @('-P', (Join-Path $PSScriptRoot 'bundle.cmake'))
& $CMake @arguments
if ($LASTEXITCODE -ne 0) { throw "Bundle creation failed with exit code $LASTEXITCODE. Staging was preserved." }
