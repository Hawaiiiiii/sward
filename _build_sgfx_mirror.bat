@echo off
setlocal
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
set "PATH=C:\Program Files\LLVM\bin;%PATH%"
set "ROOT=%~dp0"

set "BUILD_DIR=%ROOT%b\sgfx_phase365"
set "SRC_DIR=%ROOT%research_uiux\runtime_reference"

if not exist "%BUILD_DIR%\build.ninja" (
    echo === cmake configure ===
    cmake -S "%SRC_DIR%" -B "%BUILD_DIR%" -G Ninja ^
        -DCMAKE_BUILD_TYPE=RelWithDebInfo ^
        -DCMAKE_C_COMPILER=clang-cl.exe ^
        -DCMAKE_CXX_COMPILER=clang-cl.exe ^
        -DCMAKE_CXX_FLAGS=/EHsc
    if errorlevel 1 (
        echo CMAKE FAILED
        exit /b 1
    )
)

echo === ninja sgfx_ui_mirror ===
cmake --build "%BUILD_DIR%" --target sgfx_ui_mirror
if errorlevel 1 (
    echo NINJA FAILED
    exit /b 2
)
echo NINJA OK
exit /b 0
