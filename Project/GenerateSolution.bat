@ECHO OFF

PUSHD %~dp0

REM Update these lines if the currently installed version of Visual Studio is not 2017.
SET VSWHERE="%~dp0\vswhere.exe"
SET CMAKE=cmake

REM Detect latest version of Visual Studio.
FOR /F "usebackq delims=." %%i IN (`%VSWHERE% -latest -prerelease -requires Microsoft.VisualStudio.Workload.NativeGame -property installationVersion`) DO (
    SET VS_VERSION=%%i
)

IF %VS_VERSION% == 16 (
    SET CMAKE_GENERATOR="Visual Studio 16 2019"
    SET CMAKE_BINARY_DIR=%~dp0..\Build
) ELSE (
    ECHO.
    ECHO ***********************************************************************
    ECHO *                                                                     *
    ECHO *                                ERROR                                *
    ECHO *                                                                     *
    ECHO ***********************************************************************
    ECHO No compatible version of Microsoft Visual Studio detected.
    ECHO Please make sure you have Visual Studio 2015 ^(or newer^) and the 
    ECHO "Game Development with C++" workload installed before running this script.
    ECHO. 
    PAUSE
    GOTO :Exit
)

ECHO CMake Generator: %CMAKE_GENERATOR%
ECHO CMake Binary Directory: %CMAKE_BINARY_DIR%
ECHO.

MKDIR %CMAKE_BINARY_DIR% 2>NUL
PUSHD %CMAKE_BINARY_DIR%

%CMAKE% -G %CMAKE_GENERATOR% -Wno-dev "%~dp0"

IF %ERRORLEVEL% NEQ 0 (
    PAUSE
) ELSE (
REM    START OpenTech.sln
)

:Exit

POPD
POPD
