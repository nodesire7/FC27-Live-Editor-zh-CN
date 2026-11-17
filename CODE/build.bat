@echo off
setlocal
where cl >nul 2>&1
if errorlevel 1 (
    echo Please run from an x64 Native Tools Command Prompt for Visual Studio.
    exit /b 1
)
cd /d "%~dp0"
if not exist build mkdir build
pushd build
cl /nologo /LD /O2 /MT /W3 /EHsc /utf-8 ..\src\FC27LEzhFont.cpp /Fe:FC27LEzh.dll
if errorlevel 1 (popd & exit /b 1)
popd
copy /y build\FC27LEzh.dll ..\FC27LEzh.dll >nul
exit /b %errorlevel%
