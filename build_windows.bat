@echo off
setlocal EnableDelayedExpansion

:: ═══════════════════════════════════════════════════════════════════════════
:: build_windows.bat  –  Compile + windeployqt + NSIS installer
::
:: Prerequisites (must be in PATH or configured below):
::   - CMake >= 3.16          https://cmake.org/download/
::   - Qt6 MSVC 2022 x64      https://www.qt.io/download-qt-installer
::   - MSVC 2022 (Visual Studio Build Tools)
::   - NSIS >= 3.x            https://nsis.sourceforge.io/
:: ═══════════════════════════════════════════════════════════════════════════

:: ── User-configurable paths ─────────────────────────────────────────────────
:: Adjust Qt6 path to match your installation, e.g. C:\Qt\6.7.3\msvc2022_64
set QT6_DIR=C:\Qt\6.10.2\msvc2022_64
set BUILD_DIR=%~dp0build_release
set DEPLOY_DIR=%~dp0deploy

:: ── Derived paths ────────────────────────────────────────────────────────────
set WINDEPLOYQT=%QT6_DIR%\bin\windeployqt.exe
set CMAKE_PREFIX_PATH=%QT6_DIR%

:: ── Parse arguments ────────────────────────────────────────────────────────
set BUILD_INSTALLER=0
for %%A in (%*) do (
    if /i "%%A"=="--installer" set BUILD_INSTALLER=1
)

:: ── Verify tools ────────────────────────────────────────────────────────────
where cmake >nul 2>&1 || (echo [ERROR] cmake not found in PATH. & exit /b 1)
where msbuild >nul 2>&1 || where ninja >nul 2>&1 || (
    echo [ERROR] Neither msbuild nor ninja found. Run from a Visual Studio Developer Command Prompt.
    exit /b 1
)
if not exist "%WINDEPLOYQT%" (
    echo [ERROR] windeployqt not found at %WINDEPLOYQT%
    echo         Edit QT6_DIR in this script.
    exit /b 1
)

echo.
echo == 1/4  Configure ===================================================================
cmake -B "%BUILD_DIR%" ^
      -DCMAKE_BUILD_TYPE=Release ^
      -DCMAKE_PREFIX_PATH="%CMAKE_PREFIX_PATH%" ^
      -DBUILD_TESTS=OFF
if errorlevel 1 (echo [ERROR] CMake configure failed. & exit /b 1)

echo.
echo == 2/4  Build =======================================================================
cmake --build "%BUILD_DIR%" --config Release -j
if errorlevel 1 (echo [ERROR] Build failed. & exit /b 1)

echo.
echo == 3/4  Deploy Qt DLLs (windeployqt) =================================================
:: Kill any running instance so files are not locked
taskkill /f /im HitzGurutzatuak.exe >nul 2>&1

:: Ensure deploy/bin exists
if not exist "%DEPLOY_DIR%\bin" mkdir "%DEPLOY_DIR%\bin"

:: Copy exe — use robocopy with retries (Defender may briefly lock newly-built exe)
robocopy "%BUILD_DIR%\Release" "%DEPLOY_DIR%\bin" HitzGurutzatuak.exe /COPY:DAT /R:10 /W:2 /NP /NJH /NJS
powershell -Command "Unblock-File '%DEPLOY_DIR%\bin\HitzGurutzatuak.exe'" >nul 2>&1

:: Deploy Qt DLLs alongside the exe
"%WINDEPLOYQT%" --release --no-translations --no-opengl-sw "%DEPLOY_DIR%\bin\HitzGurutzatuak.exe"
if errorlevel 1 (echo [ERROR] windeployqt failed. & exit /b 1)

:: Copy assets
xcopy /y /s /e "%~dp0assets" "%DEPLOY_DIR%\assets\"

echo.
if "%BUILD_INSTALLER%"=="1" (
    echo == 4/4  Create NSIS installer =================================================
    :: CPack runs cmake --install internally (which also calls windeployqt) and then
    :: packages everything. No need to call cmake --install manually here.
    pushd "%BUILD_DIR%"
    cpack -G NSIS -C Release
    if errorlevel 1 (echo [ERROR] CPack/NSIS failed. Make sure NSIS is installed and makensis.exe is in PATH. & popd & exit /b 1)
    popd
) else (
    echo == 4/4  Skipping NSIS installer ^(pass --installer to build it^) ===============
)

echo.
echo == Done =======================================================================
if "%BUILD_INSTALLER%"=="1" echo Installer can be found in: %BUILD_DIR%\
echo Deploy folder:             %DEPLOY_DIR%\
echo.
