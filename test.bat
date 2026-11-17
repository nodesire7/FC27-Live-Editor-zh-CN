@echo off
setlocal
if "%~1"=="" (
    echo Usage: test.bat "game directory"
    exit /b 1
)
where cl >nul 2>&1
if errorlevel 1 (
    echo Please run from an x64 Native Tools Command Prompt for Visual Studio.
    exit /b 1
)
cd /d "%~dp0"
if not exist build mkdir build
pushd build
cl /nologo /O2 /EHsc /utf-8 ..\tests\check_patch.cpp /Fe:check_patch.exe
if errorlevel 1 (popd & exit /b 1)
check_patch.exe "%~1"
set CHECK_RESULT=%errorlevel%
popd
exit /b %CHECK_RESULT%
