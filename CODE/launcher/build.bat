@echo off
setlocal
if "%~1"=="" (echo Usage: build.bat path-to-upstream-checkout & exit /b 1)
set "EXT=%~f1\Launcher\external"
pushd "%~dp0"
if not exist build mkdir build
pushd build
cl /nologo /O2 /MT /EHsc /std:c++17 /utf-8 /DUNICODE /D_UNICODE /I"%EXT%\imgui" /I"%EXT%\nlohmann" ..\main.cpp "%EXT%\imgui\imgui.cpp" "%EXT%\imgui\imgui_draw.cpp" "%EXT%\imgui\imgui_widgets.cpp" "%EXT%\imgui\imgui_tables.cpp" "%EXT%\imgui\imgui_stdlib.cpp" "%EXT%\imgui\imgui_impl_dx11.cpp" "%EXT%\imgui\imgui_impl_win32.cpp" /Fe:..\..\..\GBTGame.FC27.exe /link user32.lib gdi32.lib dwmapi.lib d3d11.lib /subsystem:windows
exit /b %errorlevel%
