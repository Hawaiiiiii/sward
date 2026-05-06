@echo off
setlocal
set "ROOT=%~dp0"
cd /d "%ROOT%"
"%ROOT%b\sgfx_phase365\sgfx_ui_mirror.exe" ^
    --asset-root="%ROOT%extracted_assets\full_install_archives" ^
    --demo-seconds=4
exit /b %errorlevel%
