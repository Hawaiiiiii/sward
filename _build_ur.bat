@echo off
setlocal

rem Activate MSVC build env (provides cl.exe / link.exe / lib paths)
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul

rem Put LLVM/clang-cl on PATH (CMake will detect it)
set "PATH=C:\Program Files\LLVM\bin;%PATH%"

rem Make Vulkan/etc available if installed
set "PATH=C:\VulkanSDK\1.4.341.1\Bin;%PATH%"

rem vcpkg required by the root CMakeLists
set "VCPKG_ROOT=C:\ur103clean\thirdparty\vcpkg"

rem Build dir + source dir using the C: junction
set "BUILD_DIR=C:\ur103clean\b\ui_lab_runtime"
set "SOURCE_DIR=C:\ur103clean"

if "%1"=="--reconfigure" (
    echo === wiping build dir for clean reconfigure ===
    rmdir /S /Q "%BUILD_DIR%\CMakeFiles" 2>nul
    del /Q "%BUILD_DIR%\CMakeCache.txt" 2>nul
    del /Q "%BUILD_DIR%\build.ninja" 2>nul
    del /Q "%BUILD_DIR%\rules.ninja" 2>nul
)

if "%1"=="--clean" (
    echo === wiping ENTIRE build dir ===
    rmdir /S /Q "%BUILD_DIR%" 2>nul
    mkdir "%BUILD_DIR%" 2>nul
)

echo === configuring with cmake ===
cd /d "%BUILD_DIR%"
cmake -G Ninja ^
  "-DCMAKE_TOOLCHAIN_FILE=C:/ur103clean/thirdparty/vcpkg/scripts/buildsystems/vcpkg.cmake" ^
  -DVCPKG_TARGET_TRIPLET=x64-windows-static ^
  -DCMAKE_BUILD_TYPE=RelWithDebInfo ^
  -DCMAKE_C_COMPILER=clang-cl ^
  -DCMAKE_CXX_COMPILER=clang-cl ^
  "%SOURCE_DIR%"
if errorlevel 1 (
    echo CONFIGURE FAILED
    exit /b 1
)

echo === ninja UnleashedRecomp ===
ninja UnleashedRecomp
if errorlevel 1 (
    echo BUILD FAILED
    exit /b 2
)
echo BUILD OK
exit /b 0
