@echo off
REM Build the standalone preview harness: imgui core + the authentic recomp options_menu
REM (decoupled) + the demo platform/render impl, linked against SDL2-static.
setlocal
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul

set ROOT=%~dp0..
set IMGUI=C:\swardbuild\thirdparty\imgui
set SDLINC=C:\swardbuild\thirdparty\SDL\include
set STB=C:\swardbuild\thirdparty\stb
set SDLLIB=C:\swardbuild\sgfx_ui\build_msvc\sdl_build\SDL2-static.lib

cl /nologo /std:c++20 /EHsc /MD /O2 /DSDL_MAIN_HANDLED /D_CRT_SECURE_NO_WARNINGS ^
  /I "%IMGUI%" /I "%ROOT%" /I "%ROOT%\compat" /I "%SDLINC%" /I "%STB%" ^
  "%IMGUI%\imgui.cpp" "%IMGUI%\imgui_draw.cpp" "%IMGUI%\imgui_tables.cpp" "%IMGUI%\imgui_widgets.cpp" ^
  "%ROOT%\ui\imgui_utils.cpp" "%ROOT%\ui\black_bar.cpp" "%ROOT%\ui\fader.cpp" "%ROOT%\ui\tv_static.cpp" ^
  "%ROOT%\ui\button_guide.cpp" "%ROOT%\ui\options_menu_thumbnails.cpp" "%ROOT%\ui\options_menu.cpp" ^
  "%ROOT%\ui\achievement_menu.cpp" "%ROOT%\ui\message_window.cpp" "%ROOT%\ui\installer_wizard.cpp" ^
  "%ROOT%\platform\sgfx_platform_default.cpp" ^
  "%ROOT%\preview\preview_main.cpp" "%ROOT%\preview\preview_render.cpp" "%ROOT%\preview\preview_installer_stub.cpp" ^
  /Fe:"%ROOT%\preview\preview.exe" /Fo:"%ROOT%\preview\obj\\" ^
  /link "%SDLLIB%" winmm.lib imm32.lib version.lib setupapi.lib advapi32.lib ole32.lib oleaut32.lib ^
  gdi32.lib user32.lib shell32.lib cfgmgr32.lib hid.lib kernel32.lib

endlocal
