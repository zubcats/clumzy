@echo off
setlocal EnableExtensions
title Clumzy fresh install

net session >nul 2>&1
if not "%errorLevel%"=="0" (
  powershell -NoProfile -Command "Start-Process -FilePath '%~f0' -Verb RunAs"
  exit /b
)

set "PS1=%~dp0fresh-clumzy.ps1"
if not exist "%PS1%" (
  echo Downloading installer script...
  curl.exe -L --fail -o "%TEMP%\fresh-clumzy.ps1" "https://raw.githubusercontent.com/zubcats/clumzy/main/tools/fresh-clumzy.ps1"
  if errorlevel 1 (
    echo Could not download the installer script.
    pause
    exit /b 1
  )
  set "PS1=%TEMP%\fresh-clumzy.ps1"
)

powershell -NoProfile -ExecutionPolicy Bypass -File "%PS1%"
if errorlevel 1 pause
endlocal
