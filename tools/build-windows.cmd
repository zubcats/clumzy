@echo off
setlocal EnableExtensions EnableDelayedExpansion
cd /d "%~dp0.."

where cl >nul 2>&1
if errorlevel 1 (
  set "VCVARS="
  for /f "usebackq tokens=*" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
    if exist "%%i\VC\Auxiliary\Build\vcvars64.bat" set "VCVARS=%%i\VC\Auxiliary\Build\vcvars64.bat"
  )
  if not defined VCVARS (
    for %%Y in (2022 2025 18) do (
      for %%E in (Enterprise Professional Community BuildTools) do (
        if exist "%ProgramFiles%\Microsoft Visual Studio\%%Y\%%E\VC\Auxiliary\Build\vcvars64.bat" (
          set "VCVARS=%ProgramFiles%\Microsoft Visual Studio\%%Y\%%E\VC\Auxiliary\Build\vcvars64.bat"
        )
      )
    )
  )
  if not defined VCVARS (
    echo vcvars64.bat not found. Install Visual Studio with C++ tools.
    exit /b 1
  )
  call "!VCVARS!" || exit /b 1
)

where python >nul 2>&1
if errorlevel 1 (
  echo Python is required to build the Qt UI.
  exit /b 1
)

if exist dist rmdir /s /q dist
mkdir dist\obj_engine dist\stage || exit /b 1

cl /nologo /c /O2 /DNDEBUG /DX64 /DCLUMZY_ENGINE_ONLY /D_CRT_SECURE_NO_WARNINGS /wd4214 /std:c11 ^
  /I external\WinDivert-2.2.0-A\include ^
  /I external\iup-3.30_Win64_dll16_lib\include ^
  /Fodist\obj_engine\ ^
  src\bandwidth.c src\clumzy_api.c src\disconnect.c src\divert.c src\drop.c src\duplicate.c ^
  src\elevate.c src\lag.c src\modules.c src\ood.c src\packet.c src\reset.c src\tamper.c ^
  src\throttle.c src\utils.c || exit /b 1

link /nologo /DLL /SAFESEH:NO /OUT:dist\stage\clumzy_engine.dll ^
  dist\obj_engine\*.obj ^
  /LIBPATH:external\WinDivert-2.2.0-A\x64 ^
  /LIBPATH:external\iup-3.30_Win64_dll16_lib ^
  iup.lib WinDivert.lib winmm.lib ws2_32.lib kernel32.lib user32.lib advapi32.lib shell32.lib || exit /b 1

copy /y external\WinDivert-2.2.0-A\x64\WinDivert.dll dist\stage\ || exit /b 1
copy /y external\WinDivert-2.2.0-A\x64\WinDivert64.sys dist\stage\ || exit /b 1
copy /y external\iup-3.30_Win64_dll16_lib\iup.dll dist\stage\ || exit /b 1
copy /y etc\config.txt dist\stage\ || exit /b 1
copy /y etc\presets.ini dist\stage\ || exit /b 1
copy /y etc\clumzy-logo.png dist\stage\ || exit /b 1
copy /y etc\clumzy-icon.ico dist\stage\ || exit /b 1
copy /y LICENSE dist\stage\LICENSE.txt || exit /b 1

python -m pip install --disable-pip-version-check -q -r gui\requirements.txt || exit /b 1
python tools\build_clumzy_icon.py || exit /b 1
copy /y etc\clumzy-icon.ico dist\stage\ || exit /b 1

set "CLUMZY_ICON=%CD%\etc\clumzy-icon.ico"
set "CLUMZY_LOGO=%CD%\etc\clumzy-logo.png"
if not exist "!CLUMZY_LOGO!" (
  echo Missing logo: !CLUMZY_LOGO!
  exit /b 1
)
if not exist "!CLUMZY_ICON!" (
  echo Missing icon: !CLUMZY_ICON!
  exit /b 1
)

python -m PyInstaller --noconfirm --clean --windowed --uac-admin ^
  --name Clumzy ^
  --icon "!CLUMZY_ICON!" ^
  --add-data "!CLUMZY_LOGO!;." ^
  --add-data "!CLUMZY_ICON!;." ^
  --hidden-import PyQt5.sip ^
  --distpath dist\py --workpath dist\pybuild --specpath dist\pybuild ^
  gui\clumzy_app.py || exit /b 1

xcopy /e /y /q dist\py\Clumzy\* dist\stage\ || exit /b 1

powershell -NoProfile -Command "Compress-Archive -Path 'dist\stage\*' -DestinationPath 'dist\Clumzy-windows-x64.zip' -Force" || exit /b 1

echo Built dist\Clumzy-windows-x64.zip
dir dist\stage
dir dist\Clumzy-windows-x64.zip
exit /b 0
