@echo off
rem Checks an unpacked -static-mt archive of the C++ API; run from a VS 2022 (17.13+) x64 developer command prompt.
rem Usage: check.cmd <archive dir> <output dir> [dumpbin-only]
rem 1. No DLL of the archive imports the dynamic C runtime.
rem 2. main.cpp is linked in the three ways of the archive, each into <output dir>\<way>:
rem    dll    - the C++ API DLL (DXFCPP_USE_DLLS) and the SDK DLL;
rem    static - dxFeedGraalCxxApi_static.lib, the SDK import library and the SDK DLL;
rem    single - dxFeedGraalCxxApi_static.lib and the static library of the SDK: one exe that depends only on system
rem             DLLs and exports nothing.
rem The archive of a Debug build is checked with /MTd. Running the programs is up to the caller.
setlocal EnableDelayedExpansion
set "ARCHIVE=%~f1"
set "OUT=%~f2"
set "SRC=%~dp0main.cpp"
if not exist "%ARCHIVE%\lib\dxFeedGraalCxxApi_static.lib" (
    echo FAIL: %ARCHIVE% is not an unpacked -static-mt archive
    exit /b 1
)
set "CRT_DLLS=msvcp vcruntime api-ms-win-crt ucrtbase concrt"

for %%f in ("%ARCHIVE%\bin\*.dll") do (
    call :no_dynamic_crt "%%f" || exit /b 1
)
if "%~3"=="dumpbin-only" exit /b 0

set "MT=/MT"
echo %ARCHIVE% | findstr /i /c:"-Debug-" >nul && set "MT=/MTd"
set "CL_FLAGS=/nologo /std:c++20 %MT% /EHsc /O2 /utf-8 /I "%ARCHIVE%\include""

set "SYSTEM_LIBS="
for /f "usebackq delims=" %%l in ("%ARCHIVE%\lib\link-flags.txt") do set "SYSTEM_LIBS=!SYSTEM_LIBS! %%l"

for %%w in (dll static single) do (
    if exist "%OUT%\%%w" rmdir /s /q "%OUT%\%%w"
    mkdir "%OUT%\%%w" || exit /b 1
)

cl %CL_FLAGS% /DDXFCPP_USE_DLLS "%SRC%" "%ARCHIVE%\lib\dxFeedGraalCxxApi.lib" /Fo"%OUT%\dll\\" /Fe"%OUT%\dll\check.exe" || exit /b 1
copy /y "%ARCHIVE%\bin\*.dll" "%OUT%\dll\" >nul || exit /b 1

cl %CL_FLAGS% "%SRC%" "%ARCHIVE%\lib\dxFeedGraalCxxApi_static.lib" "%ARCHIVE%\lib\DxFeedGraalNativeSdk.lib" /Fo"%OUT%\static\\" /Fe"%OUT%\static\check.exe" || exit /b 1
copy /y "%ARCHIVE%\bin\DxFeedGraalNativeSdk.dll" "%OUT%\static\" >nul || exit /b 1

cl %CL_FLAGS% "%SRC%" "%ARCHIVE%\lib\dxFeedGraalCxxApi_static.lib" "%ARCHIVE%\lib\DxFeedGraalNativeSdk_static.lib" %SYSTEM_LIBS% /Fo"%OUT%\single\\" /Fe"%OUT%\single\check.exe" || exit /b 1

for %%w in (dll static single) do (
    call :no_dynamic_crt "%OUT%\%%w\check.exe" || exit /b 1
)
dumpbin /nologo /dependents "%OUT%\single\check.exe" | findstr /i /c:"DxFeedGraalNativeSdk.dll" /c:"dxFeedGraalCxxApi.dll" >nul && (
    echo FAIL: %OUT%\single\check.exe depends on a DLL of the archive
    exit /b 1
)
dumpbin /nologo /exports "%OUT%\single\check.exe" | findstr /r /c:"^ *[0-9][0-9]* *[0-9A-F][0-9A-F]* [0-9A-F]" >nul && (
    echo FAIL: %OUT%\single\check.exe exports symbols
    exit /b 1
)
echo OK: %ARCHIVE%
exit /b 0

:no_dynamic_crt
for %%d in (%CRT_DLLS%) do (
    dumpbin /nologo /dependents "%~1" | findstr /i /c:"    %%d" >nul && (
        echo FAIL: %~1 imports the dynamic C runtime ^(%%d^)
        dumpbin /nologo /dependents "%~1"
        exit /b 1
    )
)
echo OK: %~nx1 imports no dynamic C runtime
exit /b 0
