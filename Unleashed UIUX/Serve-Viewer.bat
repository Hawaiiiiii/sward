@echo off
rem ============================================================
rem  Serve the SWARD UI viewer over HTTP.
rem  The viewer uses in-browser Babel (loads the .jsx files via
rem  XHR) and fetch() for the manifests -- browsers BLOCK both
rem  over file://, which leaves the page black. Run this instead
rem  of double-clicking index.html.
rem ============================================================
setlocal
cd /d "%~dp0"
set PORT=8000
echo Serving %CD% at http://localhost:%PORT%/
echo Opening the viewer... (close this window to stop the server)
start "" "http://localhost:%PORT%/index.html"
python -m http.server %PORT%
