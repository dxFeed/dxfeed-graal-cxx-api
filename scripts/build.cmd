set BUILD_TYPE=Release
set BUILD_DIR=%CD%\build-win-%BUILD_TYPE%

rmdir %BUILD_DIR% /s /q
mkdir "%BUILD_DIR%"
cmake -G "Ninja" -B "%BUILD_DIR%" -DCMAKE_BUILD_TYPE=%BUILD_TYPE% -DDXFCXX_PACKAGE_SUFFIX=-%BUILD_TYPE%
cmake --build "%BUILD_DIR%" --config %BUILD_TYPE% --parallel 8
cd "%BUILD_DIR%"
cpack -G ZIP -C %BUILD_TYPE% --config dxFeedGraalCxxApiPackConfig.cmake