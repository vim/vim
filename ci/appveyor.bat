@echo off
:: Batch file for building/testing Vim on AppVeyor

:: Python3
set PYTHON3_VER=311
set PYTHON3_RELEASE=3.11.1
set PYTHON3_URL=https://www.python.org/ftp/python/%PYTHON3_RELEASE%/python-%PYTHON3_RELEASE%-amd64.exe
set PYTHON3_DIR=C:\python%PYTHON3_VER%-x64


setlocal ENABLEDELAYEDEXPANSION
cd %APPVEYOR_BUILD_FOLDER%

if not exist downloads mkdir downloads

:: Python 3
if not exist %PYTHON3_DIR% (
  call :downloadfile %PYTHON3_URL% downloads\python3.exe
  cmd /c start /wait downloads\python3.exe /quiet TargetDir=%PYTHON3_DIR%  Include_pip=0 Include_tcltk=0 Include_test=0 Include_tools=0 AssociateFiles=0 Shortcuts=0 Include_doc=0 Include_launcher=0 InstallLauncherAllUsers=0
)

cd src

:: build MSVC huge version with python and channel support
echo "Building MSVC 64bit GUI and console Version"

nmake -f Make_mvc.mak CPU=AMD64 ^
    OLE=no GUI=yes VIMDLL=yes TERMINAL=yes IME=yes ICONV=yes DEBUG=no POSTSCRIPT=yes ^
    PYTHON_VER=27 DYNAMIC_PYTHON=yes PYTHON=C:\Python27-x64 ^
    PYTHON3_VER=%PYTHON3_VER% DYNAMIC_PYTHON3=yes PYTHON3=%PYTHON3_DIR% ^
    FEATURES=HUGE

if not exist gvim.exe (
    echo Build failure: no gvim.exe
    exit 1
)
if not exist vim.exe (
    echo Build failure: no vim.exe
    exit 1
)
.\gvim -u NONE -c "redir @a | ver |0put a | wq" ver_msvc.txt || exit 1

echo "version output MSVC console"
.\vim --version || exit 1
echo "version output MSVC GUI"
type ver_msvc.txt || exit 1
cd ..

goto :eof

:downloadfile
:: ----------------------------------------------------------------------
:: call :downloadfile <URL> <localfile>
if not exist %2 (
	curl -f -L %1 -o %2
)
if ERRORLEVEL 1 (
	rem Retry once.
	curl -f -L %1 -o %2 || exit 1
)
@goto :eof
