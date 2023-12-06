set CUR_DIR=%CD%

set CURRENT_BUILD_TYPE=
set BUILD_DIR=

set CURRENT_BUILD_TYPE=Release
set BUILD_DIR=%CUR_DIR%\build-win-%CURRENT_BUILD_TYPE%
rmdir "%BUILD_DIR%" /s /q
mkdir "%BUILD_DIR%"
cmake -G "Ninja" -B "%BUILD_DIR%" -DCMAKE_BUILD_TYPE=%CURRENT_BUILD_TYPE% -DDXFCXX_PACKAGE_SUFFIX=-%CURRENT_BUILD_TYPE% -DDXFCXX_INSTALL_SAMPLES=OFF -DDXFCXX_INSTALL_TOOLS=OFF
cmake --build "%BUILD_DIR%" --config %CURRENT_BUILD_TYPE% --parallel 8
cd "%BUILD_DIR%"
cpack -G ZIP -C %CURRENT_BUILD_TYPE% --config dxFeedGraalCxxApiPackConfig.cmake
cd "%CUR_DIR%"

set CURRENT_BUILD_TYPE=Debug
set BUILD_DIR=%CUR_DIR%\build-win-%CURRENT_BUILD_TYPE%
rmdir "%BUILD_DIR%" /s /q
mkdir "%BUILD_DIR%"
cmake -G "Ninja" -B "%BUILD_DIR%" -DCMAKE_BUILD_TYPE=%CURRENT_BUILD_TYPE% -DDXFCXX_PACKAGE_SUFFIX=-%CURRENT_BUILD_TYPE% -DDXFCXX_INSTALL_SAMPLES=OFF -DDXFCXX_INSTALL_TOOLS=OFF
cmake --build "%BUILD_DIR%" --config %CURRENT_BUILD_TYPE% --parallel 8
cd "%BUILD_DIR%"
cpack -G ZIP -C %CURRENT_BUILD_TYPE% --config dxFeedGraalCxxApiPackConfig.cmake
cd "%CUR_DIR%"

set CURRENT_BUILD_TYPE=Release
set BUILD_DIR=%CUR_DIR%\build-win-Samples
rmdir "%BUILD_DIR%" /s /q
mkdir "%BUILD_DIR%"
cmake -G "Ninja" -B "%BUILD_DIR%" -DCMAKE_BUILD_TYPE=%CURRENT_BUILD_TYPE% -DDXFCXX_PACKAGE_SUFFIX=-Samples -DDXFCXX_INSTALL_LIB=OFF -DDXFCXX_INSTALL_TOOLS=OFF
cmake --build "%BUILD_DIR%" --config %CURRENT_BUILD_TYPE% --parallel 8
cd "%BUILD_DIR%"
cpack -G ZIP -C %CURRENT_BUILD_TYPE% --config dxFeedGraalCxxApiPackConfig.cmake
cd "%CUR_DIR%"

set CURRENT_BUILD_TYPE=Release
set BUILD_DIR=%CUR_DIR%\build-win-Tools
rmdir "%BUILD_DIR%" /s /q
mkdir "%BUILD_DIR%"
cmake -G "Ninja" -B "%BUILD_DIR%" -DCMAKE_BUILD_TYPE=%CURRENT_BUILD_TYPE% -DDXFCXX_PACKAGE_SUFFIX=-Tools -DDXFCXX_INSTALL_LIB=OFF -DDXFCXX_INSTALL_SAMPLES=OFF
cmake --build "%BUILD_DIR%" --config %CURRENT_BUILD_TYPE% --parallel 8
cd "%BUILD_DIR%"
cpack -G ZIP -C %CURRENT_BUILD_TYPE% --config dxFeedGraalCxxApiPackConfig.cmake
cd "%CUR_DIR%"