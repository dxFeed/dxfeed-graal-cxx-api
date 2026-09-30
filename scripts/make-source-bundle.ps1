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

    # The archives are verified with the SHA-256 hashes that CMakeLists.txt uses for the same downloads.
    $cmakeLists = Get-Content "$repoRoot/CMakeLists.txt" -Raw

    function Get-CMakeValue
    {
        param (
            [Parameter(Mandatory = $true)] [string] $Variable,
            [Parameter(Mandatory = $true)] [string] $Pattern
        )

        $match = [regex]::Match($cmakeLists, "set\($([regex]::Escape($Variable)) `"($Pattern)`"\)")

        if (!$match.Success)
        {
            throw "CMakeLists.txt does not set $Variable"
        }

        return $match.Groups[1].Value
    }

    function Assert-Sha256
    {
        param (
            [Parameter(Mandatory = $true)] [string] $Path,
            [Parameter(Mandatory = $true)] [string] $Expected
        )

        $actual = (Get-FileHash -Algorithm SHA256 -LiteralPath $Path).Hash.ToLowerInvariant()

        if ($actual -ne $Expected)
        {
            throw "SHA-256 mismatch for ${Path}: expected $Expected, actual $actual"
        }
    }

    $sdkSha256Version = Get-CMakeValue -Variable "DXFEED_GRAAL_NATIVE_SDK_SHA256_VERSION" -Pattern "[^`"]+"

    if ($graalNativeSdkVer -ne $sdkSha256Version)
    {
        throw "CMakeLists.txt has the SHA-256 of the Graal Native SDK $sdkSha256Version, not $graalNativeSdkVer"
    }

    $sdkSha256 = Get-CMakeValue -Variable "DXFEED_GRAAL_NATIVE_SDK_SHA256_amd64-windows" -Pattern "[0-9a-f]{64}"
    $glfwSha256 = Get-CMakeValue -Variable "GLFW_SHA256" -Pattern "[0-9a-f]{64}"
    $imguiSha256 = Get-CMakeValue -Variable "IMGUI_SHA256" -Pattern "[0-9a-f]{64}"

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
            [Parameter(Mandatory = $true)] [string] $ExpectedDirectory,
            [Parameter(Mandatory = $true)] [string] $Sha256
        )

        $archiveFileName = ($Repository -replace '/', '-') + "-$Tag.zip"
        $archivePath = "$downloadPath/$archiveFileName"
        $uri = "https://github.com/$Repository/archive/refs/tags/$Tag.zip"

        Write-Host "Downloading $uri"
        Invoke-WebRequest -Uri $uri -OutFile $archivePath -ErrorAction Stop
        Assert-Sha256 -Path $archivePath -Expected $Sha256
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
    Assert-Sha256 -Path "$downloadPath/graal-native-sdk-${graalNativeSdkVer}-amd64-windows.zip" -Expected $sdkSha256
    Expand-Archive -Path "$downloadPath/graal-native-sdk-${graalNativeSdkVer}-amd64-windows.zip" -Force -DestinationPath "$bundlePath/third_party/graal-native-sdk-${graalNativeSdkVer}-amd64-windows"

    Add-GitHubArchiveToBundle -Repository "glfw/glfw" -Tag $glfwVer -ExpectedDirectory "glfw-$glfwVer" -Sha256 $glfwSha256
    Add-GitHubArchiveToBundle -Repository "ocornut/imgui" -Tag "v$imguiVer" -ExpectedDirectory "imgui-$imguiVer" -Sha256 $imguiSha256

    Compress-Archive -Force -Path "$bundlePath" -DestinationPath "$buildBundleDir/$bundleName.zip"
}
