<#
    .SYNOPSIS
        This script prepares the two full source bundles for Windows x64:
        *-Full-Source-Bundle.zip with the Graal Native SDK with the dynamic C runtime (Release and Debug), and
        *-Full-Source-Bundle-static-mt.zip with the Graal Native SDK with the static C runtime (Release and Debug, the
        DLL and the static library).
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

    $glfwSha256 = Get-CMakeValue -Variable "GLFW_SHA256" -Pattern "[0-9a-f]{64}"
    $imguiSha256 = Get-CMakeValue -Variable "IMGUI_SHA256" -Pattern "[0-9a-f]{64}"

    Write-Host "dxFeed Graal Native SDK: $graalNativeSdkVer"
    Write-Host "GLFW: $glfwVer"
    Write-Host "Dear ImGui: $imguiVer"

    $downloadPath = "$buildBundleDir/download"

    New-Item -ItemType Directory -Force -Path $downloadPath

    # Downloads an archive into the download directory once (a file left by an earlier run is reused if its SHA-256
    # matches) and returns its path.
    function Get-VerifiedArchive
    {
        param (
            [Parameter(Mandatory = $true)] [string] $Uri,
            [Parameter(Mandatory = $true)] [string] $FileName,
            [Parameter(Mandatory = $true)] [string] $Sha256
        )

        $archivePath = "$downloadPath/$FileName"

        if (Test-Path -LiteralPath $archivePath)
        {
            $existing = (Get-FileHash -Algorithm SHA256 -LiteralPath $archivePath).Hash.ToLowerInvariant()

            if ($existing -eq $Sha256)
            {
                Write-Host "Using the downloaded $FileName"

                return $archivePath
            }
        }

        Write-Host "Downloading $Uri"
        Invoke-WebRequest -Uri $Uri -OutFile $archivePath -ErrorAction Stop
        Assert-Sha256 -Path $archivePath -Expected $Sha256

        return $archivePath
    }

    function Add-GitHubArchiveToBundle
    {
        param (
            [Parameter(Mandatory = $true)] [string] $BundlePath,
            [Parameter(Mandatory = $true)] [string] $Repository,
            [Parameter(Mandatory = $true)] [string] $Tag,
            [Parameter(Mandatory = $true)] [string] $ExpectedDirectory,
            [Parameter(Mandatory = $true)] [string] $Sha256
        )

        $archiveFileName = ($Repository -replace '/', '-') + "-$Tag.zip"
        $uri = "https://github.com/$Repository/archive/refs/tags/$Tag.zip"
        $archivePath = Get-VerifiedArchive -Uri $uri -FileName $archiveFileName -Sha256 $Sha256

        Expand-Archive -Path $archivePath -Force -DestinationPath "$BundlePath/third_party"

        if (!(Test-Path "$BundlePath/third_party/$ExpectedDirectory" -PathType Container))
        {
            throw "The $Repository archive did not create the expected third_party/$ExpectedDirectory directory"
        }
    }

    # Unpacks the SDK archive <variant> of the releases page into third_party/graal-native-sdk-<version>-<variant>, the
    # directory where CMakeLists.txt looks for it. ExpectedFile is a file that must be at the top of the archive.
    function Add-GraalNativeSdkToBundle
    {
        param (
            [Parameter(Mandatory = $true)] [string] $BundlePath,
            [Parameter(Mandatory = $true)] [string] $Variant,
            [Parameter(Mandatory = $true)] [string] $ExpectedFile
        )

        $sha256 = Get-CMakeValue -Variable "DXFEED_GRAAL_NATIVE_SDK_SHA256_$Variant" -Pattern "[0-9a-f]{64}"
        $archiveFileName = "graal-native-sdk-$graalNativeSdkVer-$Variant.zip"
        $uri = "https://github.com/dxFeed/dxfeed-graal-native-sdk/releases/download/v$graalNativeSdkVer/$archiveFileName"
        $archivePath = Get-VerifiedArchive -Uri $uri -FileName $archiveFileName -Sha256 $sha256
        $sdkDirectory = "$BundlePath/third_party/graal-native-sdk-$graalNativeSdkVer-$Variant"

        Expand-Archive -Path $archivePath -Force -DestinationPath $sdkDirectory

        if (!(Test-Path "$sdkDirectory/$ExpectedFile" -PathType Leaf))
        {
            throw "The Graal Native SDK archive $archiveFileName has no $ExpectedFile at the top"
        }
    }

    function New-Bundle
    {
        param (
            [Parameter(Mandatory = $true)] [string] $BundleName,
            [Parameter(Mandatory = $true)] [string[]] $SdkVariants,
            [string[]] $SdkStaticLibVariants = @()
        )

        $bundlePath = "$buildBundleDir/$BundleName"

        if (Test-Path -LiteralPath $bundlePath)
        {
            Remove-Item -LiteralPath $bundlePath -Recurse -Force
        }

        New-Item -ItemType Directory -Force -Path $bundlePath

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

        foreach ($variant in $SdkVariants)
        {
            Add-GraalNativeSdkToBundle -BundlePath $bundlePath -Variant $variant -ExpectedFile "CMakeLists.txt"
        }

        foreach ($variant in $SdkStaticLibVariants)
        {
            Add-GraalNativeSdkToBundle -BundlePath $bundlePath -Variant $variant -ExpectedFile "DxFeedGraalNativeSdk_static.lib"
        }

        Add-GitHubArchiveToBundle -BundlePath $bundlePath -Repository "glfw/glfw" -Tag $glfwVer -ExpectedDirectory "glfw-$glfwVer" -Sha256 $glfwSha256
        Add-GitHubArchiveToBundle -BundlePath $bundlePath -Repository "ocornut/imgui" -Tag "v$imguiVer" -ExpectedDirectory "imgui-$imguiVer" -Sha256 $imguiSha256

        Compress-Archive -Force -Path "$bundlePath" -DestinationPath "$buildBundleDir/$BundleName.zip"
    }

    $bundlePrefix = "dxFeedGraalCxxApi-$DxFeedGraalCxxApiVersion-x86_64-windows-Full-Source-Bundle"

    New-Bundle -BundleName $bundlePrefix -SdkVariants @("amd64-windows", "amd64-windows-debug")
    New-Bundle -BundleName "$bundlePrefix-static-mt" `
        -SdkVariants @("amd64-windows-static-crt", "amd64-windows-debug-static-crt") `
        -SdkStaticLibVariants @("amd64-windows-static-lib", "amd64-windows-debug-static-lib")
}
