@echo off
setlocal enabledelayedexpansion

rem Phase 368/370A SGFX shell launch profile.
rem
rem Wraps the canonical environment for Path B (UnleashedRecomp running
rem retail Sonic Unleashed UI under SGFX overrides + gameplay-skip +
rem UI-only input). Run from this repo's root or any working dir; the
rem script discovers the install dir relative to its own location.
rem
rem Optional positional argument 1: override pack dir. Defaults to
rem   %LOCALAPPDATA%\UnleashedRecomp\sg_overrides_sgfx_shell\
rem
rem Optional env vars (set BEFORE invoking this script):
rem   SGFX_NO_AUTOLOAD=1   -> sets SG_PREFLIGHT_NO_AUTOLOAD=1 so retail
rem                          SU's CTitleStateMenu cannot auto-resume the
rem                          existing save; the menu sits at New Save.
rem                          UR's save data on disk is never touched.
rem   SGFX_LOG_SETTEXT=1   -> turns on the bounded CSD SetText literal
rem                          probe (`Text:CsdSetTextSample:<literal>`).
rem   SGFX_SHELL_WINDOW_TITLE -> Phase 370A: replace the SDL window
rem                          title text via SGBranding::GetTitle hook.
rem   SGFX_SHELL_BUILD_LABEL  -> Phase 370A: log + bridge-emit a
rem                          custom build label string at boot.
rem   SGFX_SHELL_EXE_NAME  -> Phase 370A: copy UnleashedRecomp.exe to
rem                          this filename inside the install dir and
rem                          launch THAT copy. Default `SGFX_Shell.exe`.
rem                          Original UnleashedRecomp.exe is left
rem                          untouched. Use `UnleashedRecomp.exe` to
rem                          launch the original filename directly.

set "REPO=%~dp0"
set "INSTALL_DIR=%REPO%Unleashed Recomp - Windows (Complete Installation) 1.0.3"
if not exist "%INSTALL_DIR%\UnleashedRecomp.exe" (
    echo [sgfx-shell] missing UnleashedRecomp.exe at %INSTALL_DIR%
    exit /b 2
)

if "%~1"=="" (
    set "OVERRIDE_DIR=%LOCALAPPDATA%\UnleashedRecomp\sg_overrides_sgfx_shell"
) else (
    set "OVERRIDE_DIR=%~1"
    shift /1
)
if not exist "%OVERRIDE_DIR%" mkdir "%OVERRIDE_DIR%"

set "EXE_ARGS="
:collect_args
if "%~1"=="" goto args_done
set "EXE_ARGS=!EXE_ARGS! ^"%~1^""
shift /1
goto collect_args
:args_done

set "SG_PREFLIGHT_OVERRIDE_DIR=%OVERRIDE_DIR%"
set "SG_PREFLIGHT_GAMEPLAY_SKIP=1"
set "SG_PREFLIGHT_UI_ONLY_INPUT=1"
set "SG_PREFLIGHT_LOG_LOADS=1"
if /I "%SGFX_NO_AUTOLOAD%"=="1" (
    set "SG_PREFLIGHT_NO_AUTOLOAD=1"
)
if /I "%SGFX_LOG_SETTEXT%"=="1" (
    set "SG_PREFLIGHT_LOG_SETTEXT=1"
)
set "SG_PREFLIGHT_BRIDGE_DISABLE="

rem Phase 370A: SGFX_SHELL_EXE_NAME defaults to SGFX_Shell.exe so a
rem fresh launch lands a side-by-side copy of UnleashedRecomp.exe
rem with the SGFX-branded filename. The original UnleashedRecomp.exe
rem is never overwritten, so a `pkill UnleashedRecomp` workflow still
rem works for the unmodified runtime.
if "%SGFX_SHELL_EXE_NAME%"=="" (
    set "SGFX_SHELL_EXE_NAME=SGFX_Shell.exe"
)
if /I "%SGFX_SHELL_EXE_NAME%"=="UnleashedRecomp.exe" (
    set "LAUNCH_EXE=UnleashedRecomp.exe"
) else (
    rem Re-copy on every launch so the SGFX shell exe always tracks
    rem the latest build of UnleashedRecomp.exe.
    copy /Y "%INSTALL_DIR%\UnleashedRecomp.exe" "%INSTALL_DIR%\%SGFX_SHELL_EXE_NAME%" >nul
    if errorlevel 1 (
        echo [sgfx-shell] failed to copy UnleashedRecomp.exe -> %SGFX_SHELL_EXE_NAME%
        exit /b 2
    )
    set "LAUNCH_EXE=%SGFX_SHELL_EXE_NAME%"
)

echo === SGFX shell launch profile (Phase 368/370A) ===
echo INSTALL_DIR    = %INSTALL_DIR%
echo OVERRIDE_DIR   = %OVERRIDE_DIR%
echo NO_AUTOLOAD    = %SG_PREFLIGHT_NO_AUTOLOAD%
echo LOG_SETTEXT    = %SG_PREFLIGHT_LOG_SETTEXT%
echo SHELL_EXE_NAME = %SGFX_SHELL_EXE_NAME%
echo WINDOW_TITLE   = %SGFX_SHELL_WINDOW_TITLE%
echo BUILD_LABEL    = %SGFX_SHELL_BUILD_LABEL%
echo BRIDGE_DIR     = %APPDATA%\UnleashedRecomp\sgfx_bridge
echo LAUNCHING      = %INSTALL_DIR%\%LAUNCH_EXE%

pushd "%INSTALL_DIR%"
"%INSTALL_DIR%\%LAUNCH_EXE%" !EXE_ARGS!
set "RC=%errorlevel%"
popd
echo === sgfx-shell exit=%RC% ===
exit /b %RC%
