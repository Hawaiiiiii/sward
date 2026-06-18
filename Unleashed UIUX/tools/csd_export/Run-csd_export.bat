@echo off
rem Robust .yncp/.xncp -> clean JSON via SharpNeedle (the library Kunai uses).
rem   Run-csd_export.bat               -> export every extracted .yncp to .\out\
rem   Run-csd_export.bat in.yncp out.json
setlocal enableextensions
set "HERE=%~dp0"
set "PROJ=%HERE%..\..\.."
set "DOTNET_ROOT=%PROJ%\external_tools\dotnet8"
set "DN=%DOTNET_ROOT%\dotnet.exe"

echo [build]
"%DN%" build -c Release "%HERE%csd_export.csproj" >nul || (echo BUILD FAIL & exit /b 1)
set "EXE=%HERE%bin\Release\net8.0\csd_export.exe"

if not "%~1"=="" (
  "%EXE%" "%~1" "%~2"
  exit /b %errorlevel%
)

set "OUT=%HERE%out"
if not exist "%OUT%" mkdir "%OUT%"
echo [export all .yncp under extracted_assets -> %OUT%]
for /r "%PROJ%\extracted_assets" %%f in (*.yncp *.xncp) do (
  "%EXE%" "%%f" "%OUT%\%%~nf.json"
)
echo [done]
