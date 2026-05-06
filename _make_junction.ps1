$ErrorActionPreference = "Continue"
$repo = Split-Path -Parent $MyInvocation.MyCommand.Path
$src = Join-Path $repo "local_build_env\ur103clean"
Write-Output ("src exists: " + (Test-Path -LiteralPath $src))
Write-Output ("c:\ur103clean exists pre: " + (Test-Path 'C:\ur103clean'))
if (Test-Path 'C:\ur103clean') {
    & cmd /c "rmdir C:\ur103clean"
    Write-Output "removed pre-existing"
}
$ret = & cmd /c ('mklink /J C:\ur103clean "' + $src + '"') 2>&1
Write-Output ("mklink output: " + $ret)
Write-Output ("c:\ur103clean exists post: " + (Test-Path 'C:\ur103clean'))
Write-Output ("CMakeLists reachable: " + (Test-Path 'C:\ur103clean\UnleashedRecomp\CMakeLists.txt'))
