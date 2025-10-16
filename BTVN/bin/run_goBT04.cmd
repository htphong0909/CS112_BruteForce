@echo off
rem ==== Auto-elevate to Administrator (UAC) ====
fltmc >nul 2>&1
if %errorlevel% neq 0 (
  powershell -NoProfile -ExecutionPolicy Bypass -Command "Start-Process -FilePath 'cmd.exe' -Verb RunAs -WorkingDirectory '%cd%' -ArgumentList @('/c','"%~f0"','%*')"
  exit /b
)
rem ==== You are admin now; run target and log stderr ====
setlocal
set "BIN=%~dp0"
set "ROOT=%BIN%.."
set "LOGDIR=%ROOT%\logs"
if not exist "%LOGDIR%" mkdir "%LOGDIR%"
"%BIN%goBT04.exe" %* 2> "%LOGDIR%\04.log"
