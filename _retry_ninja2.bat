@echo off
setlocal
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
set "PATH=C:\Program Files\LLVM\bin;%PATH%"
set "VCPKG_ROOT=C:\ur103clean\thirdparty\vcpkg"
cd /d "C:\ur103clean\b\ui_lab_runtime"

echo === regenerating ninja with /EHsc ===
cmake .
if errorlevel 1 (
    echo CMAKE FAILED
    exit /b 1
)

echo === ninja UnleashedRecomp ===
ninja UnleashedRecomp
if errorlevel 1 (
    echo NINJA FAILED
    exit /b 2
)
echo NINJA OK
exit /b 0
