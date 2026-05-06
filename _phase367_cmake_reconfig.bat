@echo off
setlocal
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
set "PATH=C:\Program Files\LLVM\bin;%PATH%"
cd /d "C:\ur103clean\b\ui_lab_runtime"
echo === cmake reconfigure ===
cmake .
set "RC=%errorlevel%"
echo === cmake exit=%RC% ===
exit /b %RC%
