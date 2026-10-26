@echo off
setlocal
rem Keep relative application paths stable, including Explorer/shortcut launches.
pushd "%~dp0"
if errorlevel 1 exit /b 1
rem ExecutionPolicy applies only to this child process; no registry changes.
powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%~dp0scripts\run-local.ps1" %*
set "launchExitCode=%ERRORLEVEL%"
popd
exit /b %launchExitCode%
