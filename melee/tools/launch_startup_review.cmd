@echo off
setlocal
set "REVIEW_RUN="
for /f %%I in ('powershell.exe -NoProfile -NonInteractive -Command "[guid]::NewGuid().ToString('N')"') do set "REVIEW_RUN=%%I"
if not defined REVIEW_RUN exit /b 1
set "LOCALAPPDATA=%~dp0runs\%REVIEW_RUN%"
if exist "%LOCALAPPDATA%" exit /b 1
mkdir "%LOCALAPPDATA%" || exit /b 1
set "FREAK_MELEE_CONTROL_ID=startup-review-%REVIEW_RUN%"
pushd "%LOCALAPPDATA%"
"%~dp0..\..\melee\build\native\melee_native.exe" --iso "%~dp0..\..\Super Smash Bros. Melee (USA) (v1.02).iso"
popd
