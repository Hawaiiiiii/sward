@echo off
setlocal
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
set "PATH=C:\Program Files\LLVM\bin;%PATH%"
cd /d "C:\ur103clean\b\ui_lab_runtime"
echo === starting ninja UnleashedRecomp ===
ninja UnleashedRecomp
if errorlevel 1 (
    echo NINJA FAILED
    exit /b 1
)
echo NINJA OK
exit /b 0
