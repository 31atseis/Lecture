@echo off
cd /d "%~dp0"
where gcc >nul 2>&1
if %errorlevel%==0 goto usegcc
where cl >nul 2>&1
if %errorlevel%==0 goto usecl
echo No gcc and no cl in PATH.
echo Install MinGW-w64 and tick "Add to PATH", then run this file again.
pause
exit /b 1

:usegcc
gcc -finput-charset=UTF-8 -fexec-charset=UTF-8 -std=c11 -O2 -o game.exe main.c field.c player.c net.c con.c
if errorlevel 1 (
  echo Build failed.
  pause
  exit /b 1
)
echo OK: game.exe
exit /b 0

:usecl
cl /nologo /utf-8 /W3 /D_CRT_SECURE_NO_WARNINGS /Fe:game.exe main.c field.c player.c net.c con.c
if errorlevel 1 (
  echo Build failed.
  pause
  exit /b 1
)
echo OK: game.exe
exit /b 0
