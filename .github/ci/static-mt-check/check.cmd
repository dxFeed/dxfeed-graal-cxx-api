@echo off
rem Checks an unpacked -static-mt archive of the C++ API; run from a VS 2022 (17.14+) x64 developer command prompt.
rem Usage: check.cmd <archive dir> <output dir> [dumpbin-only]
rem 1. No DLL of the archive imports the dynamic C runtime.
rem 2. main.cpp is linked in the two ways of the archive, each into <output dir>\<way>:
rem    static - dxFeedGraalCxxApi_static.lib, the SDK import library and the SDK DLL;
rem    single - dxFeedGraalCxxApi_static.lib and the static library of the SDK: one exe that depends only on system
rem             DLLs and exports nothing.
rem The archive of a Debug build is checked with /MTd. Running the programs is up to the caller.
setlocal
set "ARCHIVE=%~f1"
set "OUT=%~f2"
set "SRC=%~dp0main.cpp"
if not exist "%ARCHIVE%\lib\dxFeedGraalCxxApi_static.lib" (
    echo FAIL: %ARCHIVE% is not an unpacked -static-mt archive
    exit /b 1
)
set "CRT_DLLS=msvcp vcruntime api-ms-win-crt ucrtbase concrt"
set "DUMP=%TEMP%\static-mt-check-dumpbin.txt"

for %%f in (dxFeedGraalCxxApi.dll DxFeedGraalNativeSdk.dll) do (
    if not exist "%ARCHIVE%\bin\%%f" (
        echo FAIL: %ARCHIVE%\bin\%%f is missing
        exit /b 1
    )
)
for %%f in ("%ARCHIVE%\bin\*.dll") do (
    call :no_dynamic_crt "%%f" || exit /b 1
)
if "%~3"=="dumpbin-only" exit /b 0

set "MT=/MT"
echo %ARCHIVE% | findstr /i /c:"-Debug-" >nul && set "MT=/MTd"
set "CL_FLAGS=/nologo /std:c++20 %MT% /EHsc /O2 /utf-8 /I "%ARCHIVE%\include""

for %%w in (static single) do (
    if exist "%OUT%\%%w" rmdir /s /q "%OUT%\%%w"
    mkdir "%OUT%\%%w" || exit /b 1
)

cl %CL_FLAGS% "%SRC%" "%ARCHIVE%\lib\dxFeedGraalCxxApi_static.lib" "%ARCHIVE%\lib\DxFeedGraalNativeSdk.lib" /Fo"%OUT%\static\\" /Fe"%OUT%\static\check.exe" || exit /b 1
copy /y "%ARCHIVE%\bin\DxFeedGraalNativeSdk.dll" "%OUT%\static\" >nul || exit /b 1

cl %CL_FLAGS% "%SRC%" "%ARCHIVE%\lib\dxFeedGraalCxxApi_static.lib" "%ARCHIVE%\lib\DxFeedGraalNativeSdk_static.lib" @"%ARCHIVE%\lib\link-flags.txt" /Fo"%OUT%\single\\" /Fe"%OUT%\single\check.exe" || exit /b 1

for %%w in (static single) do (
    call :no_dynamic_crt "%OUT%\%%w\check.exe" || exit /b 1
)
call :dumpbin /dependents "%OUT%\single\check.exe" || exit /b 1
findstr /i /c:"DxFeedGraalNativeSdk.dll" /c:"dxFeedGraalCxxApi.dll" "%DUMP%" >nul && (
    echo FAIL: %OUT%\single\check.exe depends on a DLL of the archive
    exit /b 1
)
call :dumpbin /exports "%OUT%\single\check.exe" || exit /b 1
findstr /r /c:"^ *[0-9][0-9]* *[0-9A-F][0-9A-F]* [0-9A-F]" "%DUMP%" >nul && (
    echo FAIL: %OUT%\single\check.exe exports symbols
    exit /b 1
)
echo OK: %ARCHIVE%
exit /b 0

:no_dynamic_crt
call :dumpbin /dependents "%~1" || exit /b 1
for %%d in (%CRT_DLLS%) do (
    findstr /i /c:"    %%d" "%DUMP%" >nul && (
        echo FAIL: %~1 imports the dynamic C runtime ^(%%d^)
        type "%DUMP%"
        exit /b 1
    )
)
echo OK: %~nx1 imports no dynamic C runtime
exit /b 0

rem dumpbin <option> <file> into %DUMP%; fails unless dumpbin read the image ("File Type:"), so that a failed dumpbin
rem does not pass the checks that look for a line in its output.
:dumpbin
dumpbin /nologo %1 "%~2" > "%DUMP%" 2>&1 || (
    echo FAIL: dumpbin %1 %~2
    type "%DUMP%"
    exit /b 1
)
findstr /c:"File Type:" "%DUMP%" >nul || (
    echo FAIL: dumpbin %1 %~2 did not read the image
    type "%DUMP%"
    exit /b 1
)
exit /b 0
