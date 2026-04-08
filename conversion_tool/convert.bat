@echo off
REM Convert all pictures in ./pictures subfolder with Floyd Steinberg dithering

cd /d "%~dp0pictures"

if not exist "." (
    echo Error: pictures subfolder not found!
    pause
    exit /b 1
)

echo.
echo Converting all images in pictures subfolder with Floyd Steinberg dithering...
echo.

python ..\convert.py --all --dither floydsteinberg --mode cut

if errorlevel 1 (
    echo.
    echo Error during conversion!
    pause
    exit /b 1
)

echo.
echo Conversion completed successfully!
pause
