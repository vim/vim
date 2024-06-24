@echo off
:: Batch file for building/testing Vim on AppVeyor
set target=%1
set "GETTEXT_PATH=c:\gettext64\bin"

setlocal ENABLEDELAYEDEXPANSION
cd %APPVEYOR_BUILD_FOLDER%

:: Python3
set "PYTHON3_VER=311"
set "PYTHON3_RELEASE=3.11.1"
set "PYTHON3_URL=https://www.python.org/ftp/python/%PYTHON3_RELEASE%/python-%PYTHON3_RELEASE%-amd64.exe"
set "PYTHON3_DIR=C:\python%PYTHON3_VER%-x64"

:: Gettext-tools, iconv and libraries
set "GETTEXT64_URL=https://github.com/mlocati/gettext-iconv-windows/releases/download/v0.21-v1.16/gettext0.21-iconv1.16-shared-64.zip"
set "GETTEXT64_DIR=c:\gettext64"

set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"

if exist "%VSWHERE%" (
  for /f "usebackq delims=" %%i in (
    `"%VSWHERE%" -products * -latest -property installationPath`
  ) do (
    set "VCVARSALL=%%i\VC\Auxiliary\Build\vcvarsall.bat"
  )
)

if not exist "%VCVARSALL%" (
  set "VCVARSALL=%ProgramFiles(x86)%\Microsoft Visual Studio 14.0\VC\vcvarsall.bat"
)
call "%VCVARSALL%" x64

goto %target%
echo Unknown build target.
exit 1

:: ----------------------------------------------------------------------------
:install
@echo on
if not exist downloads mkdir downloads

:: Python 3
if not exist %PYTHON3_DIR% (
  call :downloadfile %PYTHON3_URL% downloads\python3.exe
  cmd /c start /wait downloads\python3.exe /quiet TargetDir=%PYTHON3_DIR% ^
      Include_pip=0 Include_tcltk=0 Include_test=0 Include_tools=0 ^
      AssociateFiles=0 Shortcuts=0 Include_doc=0 Include_launcher=0 ^
      InstallLauncherAllUsers=0
)
:: GETTEXT
if not exist %GETTEXT64_DIR% (
  mkdir %GETTEXT64_DIR%
  call :downloadfile %GETTEXT64_URL% downloads\gettext64.zip
  cmd /c powershell.exe -NoLogo -NoProfile -Command ^
    Add-Type -AssemblyName 'System.IO.Compression.FileSystem'; ^
    [System.IO.Compression.ZipFile]::ExtractToDirectory^('downloads\gettext64.zip', ^
    '%GETTEXT64_DIR%'^)
    copy /y %GETTEXT64_DIR%\bin\libintl-8.dll C:\projects\vim\src\ || exit 1
    copy /y %GETTEXT64_DIR%\bin\libiconv-2.dll C:\projects\vim\src\ || exit 1
)

@echo off
goto :eof

:: ----------------------------------------------------------------------------
:build

cd src

echo "Building MSVC 64bit console Version"
nmake.exe -f Make_mvc.mak CPU=AMD64 ^
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
    nmake.exe -f Make_mvc.mak CPU=AMD64 ^
        OLE=no GUI=yes IME=yes ICONV=yes DEBUG=no POSTSCRIPT=yes ^
        PYTHON_VER=27 DYNAMIC_PYTHON=yes PYTHON=C:\Python27-x64 ^
        PYTHON3_VER=%PYTHON3_VER% DYNAMIC_PYTHON3=yes PYTHON3=%PYTHON3_DIR% ^
        FEATURES=%FEATURE%
) ELSE (
    nmake.exe -f Make_mvc.mak CPU=AMD64 ^
        OLE=no GUI=yes IME=yes ICONV=yes DEBUG=no ^
        FEATURES=%FEATURE%
)
if not exist gvim.exe (
    echo Build failure.
    exit 1
)
.\gvim.exe -u NONE -c "redir @a | ver |0put a | wq" ver_msvc.txt || exit 1

echo "version output MSVC console"
.\vim.exe --version || exit 1
echo "version output MSVC GUI"
type ver_msvc.txt || exit 1

goto :eof

:: ----------------------------------------------------------------------------
:test
@echo on
cd src/testdir
:: Testing with MSVC gvim
path %PYTHON3_DIR%;%GETTEXT_PATH%;%PATH%
nmake.exe -f Make_mvc.mak "VIMPROG=..\gvim.exe"
nmake.exe -f Make_mvc.mak clean
:: Testing with MSVC console version
nmake.exe -f Make_mvc.mak "VIMPROG=..\vim.exe"

@echo off
goto :eof

:: ----------------------------------------------------------------------------
:downloadfile
:: call :downloadfile <URL> <localfile>
if not exist %2 (
	curl -f -L %1 -o %2
)
if ERRORLEVEL 1 (
	rem Retry once.
	curl -f -L %1 -o %2 || exit 1
)
@goto :eof
