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

if (!$DxFeedGraalCxxApiVersion)
{
    Write-Error "\`$DxFeedGraalCxxApiVersion is not set"
}
else
{
    $thisDir = Split-Path $MyInvocation.MyCommand.Path -Parent
    $buildBundleDir = "$thisDir/../build-bundle"
    $graalNativeSdkVer = $( Get-Content .\deps.json -Raw | ConvertFrom-Json )."graal-native-sdk"
    Write-Host $graalNativeSdkVer

    $bundleName = "dxFeedGraalCxxApi-$DxFeedGraalCxxApiVersion-x86_64-windows-Full-Source-Bundle"
    $downloadPath = "$buildBundleDir/download"
    $bundlePath = "$buildBundleDir/$bundleName"

    New-Item -ItemType Directory -Force -Path $downloadPath
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
    Invoke-WebRequest -Uri "https://dxfeed.jfrog.io/artifactory/maven-open/com/dxfeed/graal-native-sdk/${graalNativeSdkVer}/graal-native-sdk-${graalNativeSdkVer}-amd64-windows.zip" -OutFile "$downloadPath/graal-native-sdk-${graalNativeSdkVer}-amd64-windows.zip"
    Expand-Archive -Path "$downloadPath/graal-native-sdk-${graalNativeSdkVer}-amd64-windows.zip" -Force -DestinationPath "$bundlePath/third_party/graal-native-sdk-${graalNativeSdkVer}-amd64-windows"
    Compress-Archive -Path "$bundlePath" -DestinationPath "$buildBundleDir/$bundleName.zip"
}