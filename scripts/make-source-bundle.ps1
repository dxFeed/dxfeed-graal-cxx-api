<#
    .SYNOPSIS
        This script prepares a full source bundle.
    .PARAMETER DxFeedGraalCxxApiVersion
        The dxFeed Graal CXX API version (https://github.com/dxFeed/dxfeed-graal-cxx-api).
#>
param (
    [Parameter(Mandatory = $true)]
    [string] $DxFeedGraalCxxApiVersion
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

if (!$DxFeedGraalCxxApiVersion)
{
    Write-Error "\`$DxFeedGraalCxxApiVersion is not set"
}
else
{
    $thisDir = Split-Path $MyInvocation.MyCommand.Path -Parent
    $repoRoot = (Resolve-Path "$thisDir/..").Path
    $buildBundleDir = "$thisDir/../build-bundle"
    $deps = Get-Content "$repoRoot/deps.json" -Raw | ConvertFrom-Json
    $graalNativeSdkVer = $deps."graal-native-sdk"
    $glfwVer = $deps.glfw
    $imguiVer = $deps.imgui

    foreach ($dependency in @("graal-native-sdk", "glfw", "imgui"))
    {
        if ([string]::IsNullOrWhiteSpace([string] $deps.$dependency))
        {
            throw "deps.json does not contain a version for '$dependency'"
        }
    }

    Write-Host "dxFeed Graal Native SDK: $graalNativeSdkVer"
    Write-Host "GLFW: $glfwVer"
    Write-Host "Dear ImGui: $imguiVer"

    $bundleName = "dxFeedGraalCxxApi-$DxFeedGraalCxxApiVersion-x86_64-windows-Full-Source-Bundle"
    $downloadPath = "$buildBundleDir/download"
    $bundlePath = "$buildBundleDir/$bundleName"

    New-Item -ItemType Directory -Force -Path $downloadPath

    if (Test-Path -LiteralPath $bundlePath)
    {
        Remove-Item -LiteralPath $bundlePath -Recurse -Force
    }

    New-Item -ItemType Directory -Force -Path $bundlePath

    function Add-GitHubArchiveToBundle
    {
        param (
            [Parameter(Mandatory = $true)] [string] $Repository,
            [Parameter(Mandatory = $true)] [string] $Tag,
            [Parameter(Mandatory = $true)] [string] $ExpectedDirectory
        )

        $archiveFileName = ($Repository -replace '/', '-') + "-$Tag.zip"
        $archivePath = "$downloadPath/$archiveFileName"
        $uri = "https://github.com/$Repository/archive/refs/tags/$Tag.zip"

        Write-Host "Downloading $uri"
        Invoke-WebRequest -Uri $uri -OutFile $archivePath -ErrorAction Stop
        Expand-Archive -Path $archivePath -Force -DestinationPath "$bundlePath/third_party"

        if (!(Test-Path "$bundlePath/third_party/$ExpectedDirectory" -PathType Container))
        {
            throw "The $Repository archive did not create the expected third_party/$ExpectedDirectory directory"
        }
    }

    Copy-Item -Path "$thisDir/../.github" -Recurse -Force -Destination "$bundlePath"
    Copy-Item -Path "$thisDir/../cmake" -Recurse -Force -Destination "$bundlePath"
    Copy-Item -Path "$thisDir/../docs" -Recurse -Force -Destination "$bundlePath"
    Copy-Item -Path "$thisDir/../include" -Recurse -Force -Destination "$bundlePath"
    Copy-Item -Path "$thisDir/../resources" -Recurse -Force -Destination "$bundlePath"
    Copy-Item -Path "$thisDir/../samples" -Recurse -Force -Destination "$bundlePath"
    Copy-Item -Path "$thisDir/../scripts" -Recurse -Force -Destination "$bundlePath"
    Copy-Item -Path "$thisDir/../src" -Recurse -Force -Destination "$bundlePath"
    Copy-Item -Path "$thisDir/../tests" -Recurse -Force -Destination "$bundlePath"
    Copy-Item -Path "$thisDir/../third_party" -Recurse -Force -Destination "$bundlePath"
    Copy-Item -Path "$thisDir/../tools" -Recurse -Force -Destination "$bundlePath"
    Copy-Item -Path "$thisDir/../*.md" -Force -Destination "$bundlePath"
    Copy-Item -Path "$thisDir/../*.json" -Force -Destination "$bundlePath"
    Copy-Item -Path "$thisDir/../*.txt" -Force -Destination "$bundlePath"
    Copy-Item -Path "$thisDir/../LICENSE" -Force -Destination "$bundlePath"
    Copy-Item -Path "$thisDir/../.clang-format" -Force -Destination "$bundlePath"
    Invoke-WebRequest -Uri "https://github.com/dxFeed/dxfeed-graal-native-sdk/releases/download/v${graalNativeSdkVer}/graal-native-sdk-${graalNativeSdkVer}-amd64-windows.zip" -OutFile "$downloadPath/graal-native-sdk-${graalNativeSdkVer}-amd64-windows.zip" -ErrorAction Stop
    Expand-Archive -Path "$downloadPath/graal-native-sdk-${graalNativeSdkVer}-amd64-windows.zip" -Force -DestinationPath "$bundlePath/third_party/graal-native-sdk-${graalNativeSdkVer}-amd64-windows"

    Add-GitHubArchiveToBundle -Repository "glfw/glfw" -Tag $glfwVer -ExpectedDirectory "glfw-$glfwVer"
    Add-GitHubArchiveToBundle -Repository "ocornut/imgui" -Tag "v$imguiVer" -ExpectedDirectory "imgui-$imguiVer"

    Compress-Archive -Force -Path "$bundlePath" -DestinationPath "$buildBundleDir/$bundleName.zip"
}
