@echo off
setlocal enabledelayedexpansion

rem Phase 367b incremental build wrapper.
rem
rem Mirrors the tracked repo `UnleashedRecomp/` tree into the canonical
rem build source root at C:\ur103clean\UnleashedRecomp BEFORE invoking
rem ninja. The CMake/ninja project resolves sources from the build root
rem (UnleashedRecomp_SOURCE_DIR=C:/ur103clean/UnleashedRecomp), not from
rem the repo, so without this sync any edits in the repo would silently
rem be ignored and ninja would report `no work to do` while still
rem producing a stale exe.

set "REPO_DIR=%~dp0"
set "BUILD_SRC=C:\ur103clean\UnleashedRecomp"
set "BUILD_OUT=C:\ur103clean\b\ui_lab_runtime"

if not exist "%BUILD_SRC%" (
    echo [phase367_build] missing build source root: %BUILD_SRC%
    exit /b 2
)
if not exist "%BUILD_OUT%\build.ninja" (
    echo [phase367_build] missing ninja build dir: %BUILD_OUT%
    exit /b 2
)

echo === [1/3] sync repo UnleashedRecomp to %BUILD_SRC% ===
rem Use /E (recurse including empty) but NOT /MIR (which would purge build-tree
rem files that are intentionally not in the repo, like the cmake-generated
rem version.cpp / version.h pair, the SWA.h api/ git submodule, and any
rem build-side intermediate state). robocopy without /MIR leaves
rem destination-only files (generated headers, submodule trees) alone, while
rem copying changed repo files even when their timestamp is older than a stale
rem build-tree copy.
robocopy "%REPO_DIR%UnleashedRecomp" "%BUILD_SRC%" /E /R:1 /W:1 /NFL /NDL /NJH /NJS /NP /NS /NC ^
    /XD .git CMakeFiles api res ^
    /XF *.obj *.pdb *.exp *.lib version.cpp version.h
set "RC=%errorlevel%"
rem robocopy: <8 = success (0=no change, 1+=copied), >=8 = error
if %RC% GEQ 8 (
    echo [phase367_build] robocopy failed with exit=%RC%
    exit /b 3
)

echo === [2/3] vcvars64 + LLVM PATH ===
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
set "PATH=C:\Program Files\LLVM\bin;%PATH%"

echo === [3/3] ninja UnleashedRecomp ===
pushd "%BUILD_OUT%"
ninja UnleashedRecomp
set "NINJA_RC=%errorlevel%"
popd
echo === ninja exit=%NINJA_RC% ===
exit /b %NINJA_RC%
