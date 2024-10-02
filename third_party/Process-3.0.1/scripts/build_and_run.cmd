@echo off
for /f %%i in ('dir /a:d /s /b build-win*') do rd /s /q %%i
set CUR_YYYY=%date:~10,4%
set CUR_MM=%date:~4,2%
set CUR_DD=%date:~7,2%
set CUR_HH=%time:~0,2%
if %CUR_HH% lss 10 (set CUR_HH=0%time:~1,1%)
set CUR_NN=%time:~3,2%
set CUR_SS=%time:~6,2%
set CUR_MS=%time:~9,2%
set SUBFILENAME=%CUR_YYYY%%CUR_MM%%CUR_DD%-%CUR_HH%%CUR_NN%%CUR_SS%
mkdir build-win-%SUBFILENAME%
cd build-win-%SUBFILENAME%
cmake -DCMAKE_BUILD_TYPE=Release ..\..
cmake --build . --config Release -j 4
@rem dir
Release\process_test.exe

pause
