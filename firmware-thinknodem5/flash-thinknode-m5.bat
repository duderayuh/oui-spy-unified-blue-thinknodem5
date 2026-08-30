@echo off
setlocal EnableDelayedExpansion
title OUI-SPY ThinkNode M5 Flasher
cd /d "%~dp0"

echo.
echo  ==============================================
echo    OUI-SPY  -  ThinkNode M5  one-click flasher
echo  ==============================================
echo.

REM ---- locate Python ----
set "PY=python"
%PY% --version >nul 2>&1
if errorlevel 1 (
    set "PY=py -3"
    py -3 --version >nul 2>&1
    if errorlevel 1 (
        echo  [ERROR] Python not found.
        echo          Install from https://www.python.org/downloads/
        echo          IMPORTANT: tick "Add python.exe to PATH" during install.
        pause
        exit /b 1
    )
)
echo  [ok] Python found

REM ---- find COM port(s) ----
set "PORTS="
for /f "usebackq delims=" %%P in (`powershell -NoProfile -Command "[System.IO.Ports.SerialPort]::GetPortNames()"`) do set "PORTS=!PORTS! %%P"
if not defined PORTS (
    echo.
    echo  [ERROR] No COM port detected.
    echo          ^> Is the ThinkNode plugged in with a DATA cable?
    echo          ^> Is the CH340 driver installed? ^(Device Manager^)
    pause
    exit /b 1
)
echo  [ok] COM port(s) found:%PORTS%

REM pick single port automatically; if multiple, ask
set "N=0"
for %%A in (%PORTS%) do set /a N+=1
set "PORT="
if !N! EQU 1 (
    for %%A in (%PORTS%) do set "PORT=%%A"
) else (
    echo.
    set /p PORT=  Multiple ports found - type the ThinkNode's port (e.g. COM3): 
)
echo  Using port: !PORT!

REM ---- install esptool if missing ----
%PY% -m esptool version >nul 2>&1
if errorlevel 1 (
    echo  Installing esptool (one-time)...
    %PY% -m pip install --quiet esptool pyserial
)

REM ---- always download the latest firmware image (overwrites any stale copy) ----
echo  Downloading latest firmware image (4 MB)...
curl.exe -L --fail -s -o merged-flash.bin "https://github.com/duderayuh/oui-spy-unified-blue-thinknodem5/raw/master/firmware-thinknodem5/merged-flash.bin"
if errorlevel 1 (
    echo  [ERROR] Download failed. Check your internet connection.
    pause
    exit /b 1
)
echo  [ok] Firmware ready

echo.
echo  Flashing... this takes ~30 seconds. Do NOT unplug the board.
echo.
%PY% -m esptool --chip esp32s3 --port !PORT! --baud 460800 write_flash 0x0 merged-flash.bin

if errorlevel 1 (
    echo.
    echo  [FAILED] Flashing failed - see the error above.
    echo     - Try a different USB *data* cable
    echo     - Reinstall the CH340 driver, then replug and rerun
) else (
    echo.
    echo  [DONE] Flash successful!
    echo         Unplug/replug the ThinkNode to boot OUI-SPY.
)
echo.
pause
