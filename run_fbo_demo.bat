@echo off
setlocal EnableExtensions

title Learn-openGL - fbo_demo

rem Some launchers provide both Path and PATH in the raw Windows environment.
rem cmd.exe cannot reliably remove only the duplicate entry, while MSBuild and
rem Start-Process reject it as a duplicate dictionary key. Re-enter this script
rem once through PowerShell after rebuilding a canonical single Path entry.
if /I "%LEARN_OPENGL_ENV_NORMALIZED%"=="1" goto :environment_ready
set "LEARN_OPENGL_RUN_SCRIPT=%~f0"
set "LEARN_OPENGL_RUN_ARG1=%~1"
set "LEARN_OPENGL_RUN_ARG2=%~2"
powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -Command "$pathValue = $env:Path; Remove-Item Env:Path -ErrorAction SilentlyContinue; $env:Path = $pathValue; $env:LEARN_OPENGL_ENV_NORMALIZED = '1'; & $env:LEARN_OPENGL_RUN_SCRIPT $env:LEARN_OPENGL_RUN_ARG1 $env:LEARN_OPENGL_RUN_ARG2; exit $LASTEXITCODE"
exit /b %ERRORLEVEL%

:environment_ready
set "LEARN_OPENGL_ENV_NORMALIZED="
set "LEARN_OPENGL_RUN_SCRIPT="
set "LEARN_OPENGL_RUN_ARG1="
set "LEARN_OPENGL_RUN_ARG2="

for %%I in ("%~dp0.") do set "PROJECT_DIR=%%~fI"
set "BUILD_DIR=%PROJECT_DIR%\build"
set "CONFIG=%~1"
if not defined CONFIG set "CONFIG=Release"

if /I not "%CONFIG%"=="Release" if /I not "%CONFIG%"=="Debug" (
    echo [ERROR] Unknown configuration: %CONFIG%
    echo Usage: %~nx0 [Release^|Debug] [--build-only]
    pause
    exit /b 2
)

set "CMAKE_EXE=cmake"
where cmake >nul 2>nul
if errorlevel 1 (
    set "CMAKE_EXE=C:\Program Files\CMake\bin\cmake.exe"
    if not exist "%CMAKE_EXE%" (
        echo [ERROR] CMake was not found. Add it to PATH or install CMake.
        pause
        exit /b 1
    )
)

set "VCPKG_TOOLCHAIN=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake"
if not defined VCPKG_ROOT set "VCPKG_TOOLCHAIN=%USERPROFILE%\vcpkg\scripts\buildsystems\vcpkg.cmake"

pushd "%PROJECT_DIR%" || (
    echo [ERROR] Cannot enter project directory: %PROJECT_DIR%
    pause
    exit /b 1
)

echo [1/3] Configuring CMake...
if exist "%BUILD_DIR%\CMakeCache.txt" (
    "%CMAKE_EXE%" -S "%PROJECT_DIR%" -B "%BUILD_DIR%"
) else if exist "%VCPKG_TOOLCHAIN%" (
    "%CMAKE_EXE%" -S "%PROJECT_DIR%" -B "%BUILD_DIR%" -DCMAKE_TOOLCHAIN_FILE="%VCPKG_TOOLCHAIN%" -DVCPKG_TARGET_TRIPLET=x64-windows
) else (
    "%CMAKE_EXE%" -S "%PROJECT_DIR%" -B "%BUILD_DIR%"
)
if errorlevel 1 goto :fail

echo [2/3] Building fbo_demo (%CONFIG%)...
"%CMAKE_EXE%" --build "%BUILD_DIR%" --config "%CONFIG%" --target fbo_demo --parallel
if errorlevel 1 goto :fail

set "EXE=%BUILD_DIR%\%CONFIG%\fbo_demo.exe"
if not exist "%EXE%" set "EXE=%BUILD_DIR%\fbo_demo.exe"
if not exist "%EXE%" (
    echo [ERROR] fbo_demo.exe was not found after the build.
    goto :fail
)

if /I "%~2"=="--build-only" (
    echo [3/3] Build completed: %EXE%
    popd
    exit /b 0
)

echo [3/3] Launching %EXE%
pushd "%BUILD_DIR%" || (
    echo [ERROR] Cannot enter runtime directory: %BUILD_DIR%
    goto :fail
)
"%EXE%"
set "APP_EXIT_CODE=%ERRORLEVEL%"
popd
popd

if not "%APP_EXIT_CODE%"=="0" (
    echo [ERROR] fbo_demo exited with code %APP_EXIT_CODE%.
    pause
)
exit /b %APP_EXIT_CODE%

:fail
set "BUILD_EXIT_CODE=%ERRORLEVEL%"
if "%BUILD_EXIT_CODE%"=="0" set "BUILD_EXIT_CODE=1"
popd
echo.
echo Build or launch failed with code %BUILD_EXIT_CODE%.
pause
exit /b %BUILD_EXIT_CODE%
