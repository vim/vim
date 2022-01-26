@echo off
:: Batch file for building/testing Vim on AppVeyor

setlocal ENABLEDELAYEDEXPANSION
cd %APPVEYOR_BUILD_FOLDER%

cd src

echo "Building MSVC 64bit console Version"
nmake -f Make_mvc2.mak CPU=AMD64 ^
    OLE=no GUI=no IME=yes ICONV=yes DEBUG=no ^
    FEATURES=%FEATURE%
if not exist vim.exe (
    echo Build failure.
    exit 1
)

:: build MSVC huge version with python and channel support
:: GUI needs to be last, so that testing works
echo "Building MSVC 64bit GUI Version"
if "%FEATURE%" == "HUGE" (
    nmake -f Make_mvc.mak CPU=AMD64 ^
        OLE=no GUI=yes IME=yes ICONV=yes DEBUG=no POSTSCRIPT=yes ^
        PYTHON_VER=27 DYNAMIC_PYTHON=yes PYTHON=C:\Python27-x64 ^
        PYTHON3_VER=35 DYNAMIC_PYTHON3=yes PYTHON3=C:\Python35-x64 ^
        FEATURES=%FEATURE%
) ELSE (
    nmake -f Make_mvc.mak CPU=AMD64 ^
        OLE=no GUI=yes IME=yes ICONV=yes DEBUG=no ^
        FEATURES=%FEATURE%
)
if not exist gvim.exe (
    echo Build failure.
    exit 1
)
.\gvim -u NONE -c "redir @a | ver |0put a | wq" ver_msvc.txt || exit 1

echo "version output MSVC console"
.\vim --version || exit 1
echo "version output MSVC GUI"
type ver_msvc.txt || exit 1
cd ..
