@echo off
setlocal
set "ROOT=%~dp0"
set "SG_PREFLIGHT_OVERRIDE_DIR=%LOCALAPPDATA%\UnleashedRecomp\sg_overrides"
set "SG_PREFLIGHT_GAMEPLAY_SKIP=1"
set "SG_PREFLIGHT_UI_ONLY_INPUT=1"
set "SG_PREFLIGHT_LOG_LOADS=1"
set "SG_PREFLIGHT_BRIDGE_DISABLE="
set "BUILD_OUT=C:\ur103clean\b\ui_lab_runtime\UnleashedRecomp"
set "UR_DIR=%ROOT%Unleashed Recomp - Windows (Complete Installation) 1.0.3"
if exist "%BUILD_OUT%\UnleashedRecomp.exe" (
    copy /Y "%BUILD_OUT%\UnleashedRecomp.exe" "%UR_DIR%\UnleashedRecomp.exe" >nul
    if exist "%BUILD_OUT%\dxcompiler.dll" copy /Y "%BUILD_OUT%\dxcompiler.dll" "%UR_DIR%\dxcompiler.dll" >nul
    if exist "%BUILD_OUT%\dxil.dll" copy /Y "%BUILD_OUT%\dxil.dll" "%UR_DIR%\dxil.dll" >nul
) else (
    echo NO BUILD OUTPUT at %BUILD_OUT%\UnleashedRecomp.exe -- run _build_ur.bat first
    exit /b 1
)
cd /d "%UR_DIR%"
echo === launching UnleashedRecomp.exe (fresh Path B build, log only) ===
echo CWD: %CD%
"%UR_DIR%\UnleashedRecomp.exe"
echo === UR exited with %errorlevel% ===
exit /b 0
