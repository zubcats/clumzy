@echo off
setlocal EnableExtensions EnableDelayedExpansion
cd /d "%~dp0.."

set "VCVARS="
for %%E in (Enterprise Professional Community BuildTools) do (
  if exist "%ProgramFiles%\Microsoft Visual Studio\2022\%%E\VC\Auxiliary\Build\vcvars64.bat" (
    set "VCVARS=%ProgramFiles%\Microsoft Visual Studio\2022\%%E\VC\Auxiliary\Build\vcvars64.bat"
  )
)
if not defined VCVARS (
  echo vcvars64.bat not found. Install Visual Studio 2022 with C++ tools.
  exit /b 1
)
call "%VCVARS%" || exit /b 1

if exist dist rmdir /s /q dist
mkdir dist\obj dist\stage || exit /b 1

rc /nologo /d NDEBUG /d X64 /fo dist\clumzy.res etc\clumsy.rc || exit /b 1

cl /nologo /c /O2 /DNDEBUG /DX64 /D_CRT_SECURE_NO_WARNINGS /wd4214 /std:c11 ^
  /I external\WinDivert-2.2.0-A\include ^
  /I external\iup-3.30_Win64_dll16_lib\include ^
  /Fodist\obj\ ^
  src\*.c || exit /b 1

link /nologo /SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup /SAFESEH:NO ^
  /OUT:dist\stage\Clumzy.exe ^
  dist\obj\*.obj dist\clumzy.res ^
  /LIBPATH:external\WinDivert-2.2.0-A\x64 ^
  /LIBPATH:external\iup-3.30_Win64_dll16_lib ^
  iup.lib WinDivert.lib comctl32.lib winmm.lib ws2_32.lib gdi32.lib comdlg32.lib uuid.lib ole32.lib kernel32.lib || exit /b 1

copy /y external\WinDivert-2.2.0-A\x64\WinDivert.dll dist\stage\ || exit /b 1
copy /y external\WinDivert-2.2.0-A\x64\WinDivert64.sys dist\stage\ || exit /b 1
copy /y external\iup-3.30_Win64_dll16_lib\iup.dll dist\stage\ || exit /b 1
copy /y etc\config.txt dist\stage\ || exit /b 1
copy /y etc\presets.ini dist\stage\ || exit /b 1
copy /y LICENSE dist\stage\LICENSE.txt || exit /b 1

powershell -NoProfile -Command "Compress-Archive -Path 'dist\stage\*' -DestinationPath 'dist\Clumzy-windows-x64.zip' -Force" || exit /b 1

echo Built dist\Clumzy-windows-x64.zip
dir dist\stage
dir dist\Clumzy-windows-x64.zip
exit /b 0
