@echo off
setlocal
set "ROOT=%~dp0"

rem Phase 362 launch script: daemon + UnleashedRecomp with all the env vars
rem the new Phase 359 / 360 / 361 hooks honor.
rem
rem Run from anywhere; relative paths assume sg-preflight + the build artifacts
rem are at their canonical locations on this machine.

if not defined SGP_ROOT set "SGP_ROOT=%USERPROFILE%\Downloads\sg-preflight"
set "BUILD_OUT=C:\ur103clean\b\ui_lab_runtime\UnleashedRecomp"
set "TARGET_EXE_DIR=%ROOT%Unleashed Recomp - Windows (Complete Installation) 1.0.3"
rem UR's bridge_runtime resolves the bridge dir from APPDATA (Roaming), not LOCALAPPDATA.
rem Daemon must watch the same path UR writes to or events/state will diverge.
set "BRIDGE_DIR=%APPDATA%\UnleashedRecomp\sgfx_bridge"
set "OVERRIDE_DIR=%LOCALAPPDATA%\UnleashedRecomp\sg_overrides"

rem Phase 359 + 360 + 361 env vars.
set "SG_PREFLIGHT_OVERRIDE_DIR=%OVERRIDE_DIR%"
set "SG_PREFLIGHT_GAMEPLAY_SKIP=1"
set "SG_PREFLIGHT_LOG_LOADS=1"

rem Make sure the override + bridge dirs exist (mounts auto-detect them).
if not exist "%OVERRIDE_DIR%" mkdir "%OVERRIDE_DIR%"
if not exist "%BRIDGE_DIR%" mkdir "%BRIDGE_DIR%"

echo === copying fresh exe to sg-preflight ===
if exist "%BUILD_OUT%\UnleashedRecomp.exe" (
    copy /Y "%BUILD_OUT%\UnleashedRecomp.exe" "%TARGET_EXE_DIR%\UnleashedRecomp.exe"
    if exist "%BUILD_OUT%\dxcompiler.dll" copy /Y "%BUILD_OUT%\dxcompiler.dll" "%TARGET_EXE_DIR%\dxcompiler.dll"
    if exist "%BUILD_OUT%\dxil.dll" copy /Y "%BUILD_OUT%\dxil.dll" "%TARGET_EXE_DIR%\dxil.dll"
) else (
    echo NO BUILD OUTPUT at %BUILD_OUT%\UnleashedRecomp.exe -- run _build_ur.bat first
    exit /b 1
)

echo === starting bridge daemon ===
start "sgfx-bridge-daemon" /MIN ^
    "%SGP_ROOT%\.venv\Scripts\python.exe" -m sg_preflight bridge-daemon ^
    --bridge-dir "%BRIDGE_DIR%"

rem give the daemon ~10s to write initial state.json (subprocess overhead)
timeout /T 10 /NOBREAK >nul

echo === launching UnleashedRecomp ===
echo (Phase 359 mount: %OVERRIDE_DIR%)
echo (Phase 360 gameplay-skip: enabled)
echo (Phase 361 bridge dir:    %BRIDGE_DIR%)
"%TARGET_EXE_DIR%\UnleashedRecomp.exe"

echo === UnleashedRecomp exited; bridge daemon is still running in the minimised window ===
echo === close it manually when done. ===
exit /b 0
