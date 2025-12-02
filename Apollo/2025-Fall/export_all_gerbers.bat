@echo off
REM Batch script to export gerbers for all KiCad PCB files in Apollo folder

setlocal enabledelayedexpansion

set "KICAD_CLI=C:\Program Files\KiCad\9.0\bin\kicad-cli.exe"
set "APOLLO_DIR=%~dp0"
set "GERBERS_DIR=%APOLLO_DIR%Gerbers"

echo ============================================================
echo Exporting Gerbers for all PCB files in Apollo folder
echo ============================================================
echo.

REM Create Gerbers directory if it doesn't exist
if not exist "%GERBERS_DIR%" mkdir "%GERBERS_DIR%"

REM Counter for statistics
set /a SUCCESS=0
set /a FAILED=0

REM Process all .kicad_pcb files recursively
for /r "%APOLLO_DIR%" %%f in (*.kicad_pcb) do (
    REM Skip files in Gerbers directory
    echo %%f | findstr /i "\\Gerbers\\" >nul
    if errorlevel 1 (
        REM Get just the filename without extension
        set "FILENAME=%%~nf"
        set "PCB_PATH=%%f"

        echo Processing: !FILENAME!

        REM Create temp directory for this project
        set "TEMP_DIR=%GERBERS_DIR%\!FILENAME!_temp"
        if exist "!TEMP_DIR!" rmdir /s /q "!TEMP_DIR!"
        mkdir "!TEMP_DIR!"

        REM Export gerbers using kicad-cli
        "%KICAD_CLI%" pcb export gerbers -o "!TEMP_DIR!/" --no-protel-ext --board-plot-params "!PCB_PATH!"
        if !errorlevel! equ 0 (
            echo   [OK] Gerbers exported

            REM Export drill files
            "%KICAD_CLI%" pcb export drill -o "!TEMP_DIR!/" --format excellon "!PCB_PATH!"
            if !errorlevel! equ 0 (
                echo   [OK] Drill files exported
            )

            REM Create zip file
            set "ZIP_FILE=%GERBERS_DIR%\!FILENAME!_gerbers.zip"
            if exist "!ZIP_FILE!" del "!ZIP_FILE!"

            powershell -command "Compress-Archive -Path '!TEMP_DIR!\*' -DestinationPath '!ZIP_FILE!'" 2>nul
            if !errorlevel! equ 0 (
                echo   [OK] Created: !FILENAME!_gerbers.zip
                set /a SUCCESS+=1
            ) else (
                echo   [ERROR] Failed to create zip
                set /a FAILED+=1
            )

            REM Clean up temp directory
            rmdir /s /q "!TEMP_DIR!"
        ) else (
            echo   [ERROR] Failed to export gerbers
            set /a FAILED+=1
            if exist "!TEMP_DIR!" rmdir /s /q "!TEMP_DIR!"
        )
        echo.
    )
)

echo ============================================================
echo Export Complete!
echo   Successful: %SUCCESS%
echo   Failed: %FAILED%
echo   Output directory: %GERBERS_DIR%
echo ============================================================

endlocal
