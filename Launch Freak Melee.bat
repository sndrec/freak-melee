@echo off
setlocal

set "ROOT=%~dp0"
set "GAME=%ROOT%melee\build\native\melee_native.exe"
set "ISO=%ROOT%Super Smash Bros. Melee (USA) (v1.02).iso"

if not exist "%GAME%" goto missing_game
if not exist "%ISO%" goto missing_iso

pushd "%ROOT%melee"
"%GAME%" --iso "%ISO%"
set "RESULT=%ERRORLEVEL%"
popd

if not "%RESULT%"=="0" goto game_failed

exit /b 0

:missing_game
echo Release build not found:
echo   %GAME%
echo.
echo Build the melee_native target first.
pause
exit /b 1

:missing_iso
echo Melee USA v1.02 ISO not found:
echo   %ISO%
pause
exit /b 1

:game_failed
echo.
echo Freak Melee exited with code %RESULT%.
pause
exit /b %RESULT%
