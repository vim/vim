goto :START
::============================================================================
:: To be used on MS-Windows for Visual C++ 5.x-14.0 Express Edition
:: aka Microsoft Visual Studio 10.0.
:: See INSTALLpc.txt for information.
::============================================================================
:: Nedd add ia64 ^| x86_ia64 ^| arm ^| amd64_x86 | amd64_arm 
:: vc10 x86 | ia64 | amd64 | x86_amd64 | x86_ia64
:: vc14 x86 | amd64 | arm | x86_amd64 | x86_arm | amd64_x86 | amd64_arm
::============================================================================
:: Autodetect Visual Studio vc6 - vc14
:: Autodetect Architectures 
:: if exist "%PROGRAMFILES(X86)%" x64 else x86
:: http://ss64.com/nt/syntax-64bit.html
::============================================================================
:: For using Visual Studio 6.0 on Windows Versions from 6.x for 10.0x 
:: http://nuke.vbcorner.net/
:: Here you can found MS Visual Studio 5.x and 6.0
:: https://winworldpc.com/product/microsoft-visual-stu/60
::============================================================================
:: If autodetect not work modificate or create 'before_settings.bat' file.
:: You can use Before build [before_settings.bat]
:: You can use After  build [after_settings.bat]
::==== TODO ==================================================================
:: Switch 4 mo sensetive. /depricated
::
:: GOTO CASE_%I%
::  :CASE_0
::      ECHO i equals 0
::      GOTO END_SWITCH
::  :CASE_1
::      ECHO i equals 1
::      GOTO END_SWITCH
::  :END_SWITCH
::
::============================================================================
:START
@cls
@echo off

:: Before build.
if exist "before_settings.bat" echo Found before_settings.bat & call "before_settings.bat"

@setlocal enableextensions enabledelayedexpansion
::============================================================================
:: Global variable
pushd ".\src"
set "SVNSRC=%CD%"
::======================= Use Microsoft Visual Studio  =======================

call :set_vcvars %*
call :set_vcvars_env %*
if /i "%~1" == "" call :usage %*


  title "Command Prompt Setting Additionals Interfaces"
  echo ==============================================================================
  echo Setting Additionals Interfaces
  echo ==============================================================================

  echo Setting Lua interface
rem    set LUA=[Path to Lua directory]
rem    set DYNAMIC_LUA=yes
rem    set LUA_VER=[Lua version]  (default is 51)
  echo Setting MzScheme interface
rem    set MZSCHEME=[Path to MzScheme directory]
rem    set DYNAMIC_MZSCHEME=yes (to load the MzScheme DLLs dynamically)
rem    set MZSCHEME_VER=[version, 205_000, ...]
rem    set MZSCHEME_DEBUG=no
  echo Setting Perl interface
rem    set PERL=[Path to Perl directory]
rem    set DYNAMIC_PERL=yes (to load the Perl DLL dynamically)
rem    set PERL_VER=[Perl version, in the form 55 (5.005), 56 (5.6.x),510 (5.10.x), etc] (default is 56)
  echo Setting Python 2.7.10 interface
    set PYTHON=C:\Python27
    set DYNAMIC_PYTHON=yes
    set PYTHON_VER=27
  echo Setting Python 3.5 interface
    set PYTHON3=C:\Python35
    set DYNAMIC_PYTHON3=yes
    set PYTHON3_VER=35
  echo Setting Ruby interface
    rem  You must set RUBY_VER_LONG when change RUBY_VER.
    rem  RUBY_API_VER is derived from RUBY_VER_LONG.
    rem  Note: If you use Ruby 1.9.3, set as follows:
rem    set RUBY_VER=19
rem    set RUBY_VER_LONG=1.9.1 (not 1.9.3, because the API version is 1.9.1.)
rem    set RUBY=[Path to Ruby directory]
rem    set DYNAMIC_RUBY=yes (to load the Ruby DLL dynamically)
rem    set RUBY_VER=[Ruby version, eg 18, 19, 20] (default is 18)
rem    set RUBY_VER_LONG=[Ruby version, eg 1.8, 1.9.1, 2.0.0] (default is 1.8)
  echo Setting Tcl interface
rem    set TCL=[Path to Tcl directory]
    rem load the Tcl DLL dynamically
rem    set DYNAMIC_TCL=yes
rem    set TCL_VER=[Tcl version, e.g. 80, 83]  (default is 83)
rem    set TCL_VER_LONG=[Tcl version, eg 8.3] (default is 8.3)
    rem You must set TCL_VER_LONG when you set TCL_VER.

  title "Command Prompt Setting Vi IMproved"
  echo ==============================================================================
  echo We Setting Vi IMproved
  echo ==============================================================================

  echo Feature Setings
::   Specifies what optional features to use, as given in feature.h of the Vim source.
::   Available options are TINY, SMALL, NORMAL, BIG, and HUGE 
    set FEATURES=HUGE
  echo Multibyte support
    set MBYTE=yes
  echo SNiFF+ interface                                 (default is yes)
    set SNIFF=yes
  echo Cscope support                                   (default is yes)
rem    set CSCOPE=no
  echo Iconv library support (always dynamically loaded)(default is yes)
rem    set ICONV=no
  echo Intl library support (always dynamically loaded) (default is yes)
  echo See http://sourceforge.net/projects/gettext/
rem    set GETTEXT=no
  echo PostScript printing: POSTSCRIPT=yes              (default is no )
  echo See https://en.wikipedia.org/wiki/PostScript#PostScript_printing
rem    set POSTSCRIPT=yes
  echo Netbeans Support                                 (default is yes if GUI build)
rem    set NETBEANS=no
  echo XPM Image Support (Not worked with Microsoft Visual Studio 12/15 [vc12/vc14])
::   Default is "xpm", using the files included in the distribution.
::   Use "no" to disable this feature.
rem    set "XPM=no"
  echo Optimization                                     (default is MAXSPEED)
::   Specifies the optimization level of the executable. Available settings are:
::   SPACE (for the smallest executable) (key /O1)
::   SPEED (for a well-balanced executable) (key /O2)
::   MAXSPEED (for a large but fast executable) (key /Ox). 
    set OPTIMIZE=MAXSPEED
  if "%vcver%" leq "7" echo Processor Version           (default is i386)
    if "%vcver%" leq "7" set CPUNR=pentium4
rem  echo Netbeans Debugging Support (should be no, yes doesnt work)  (default is no)
rem    set NBDEBUG=yes
rem  echo Visual C Version: (default derived from nmake if undefined)
rem set MSVCVER=5.0
:: !!! if "%vcver%" geq "10" set "ANALYZE=yes" & echo Static Code Analysis                  (works with VisualStudio 10 or greater) &  echo See Make_mvc.mak Need update for vs14
rem  echo See Make_mvc.mak for DEBUG informatin
rem echo Adding DEBUG
rem set DEBUG=yes
  echo See Make_mvc.mak or feature.h for a list of optionals.
echo ==============================================================================
rem  set "WINVER=0x0400"
  echo WINVER=%WINVER% (WINVER=0x0500 or 0x0400 defoult 0x0400)

  echo Get all sorts of useful, standard macros from the Platform SDK.
  echo Default is %ProgramFiles(x86)%\Microsoft SDKs\Windows\v7.1A\include\Win32.mak)
    if "%vcver%" geq "12" set "SDK_INCLUDE_DIR=%ProgramFiles(x86)%\Microsoft SDKs\Windows\v7.1A\include"

if /i "%vcmod%" == "x64" set "SUBSYSTEM_VER=5.02" & set CPU=AMD64
if /i "%vcmod%" == "x86_amd64" set "SUBSYSTEM_VER=5.02" & set CPU=AMD64
if /i "%vcmod%" == "x86"   set "SUBSYSTEM_VER=5.01"

if /i "%WINVER%" == "0x0400" set "SUBSYSTEM_VER=4.00"
  title "Command Prompt (VC++ %vcver% %vcmod% %buildt%) nmake Vim"

::  Don't remove remark here while not fixed
  if "%vcver%" geq "12" xpm=no

:: if OLE undefiend OLE=NO
if /i "%buildt%" == "CUI" goto CUI
  set GUI=yes
  set IME=yes
  set GIME=yes
  set DIRECTX=yes
if /i "%buildt%" == "ALL" (
  call :nmake_mvc
  set "OLE=yes"
)
:GUI
  call :nmake_mvc
if /i "%buildt%" == "GUI" goto after_build
:CUI
  set GUI=
  set IME=
  set GIME=
  set DIRECTX=
  call :nmake_mvc
:after_build
set "SVNDST=..\bin\%vcmod%"
echo ==============================================================================
echo Copy any new Vim exe + runtime files to current install dir.
echo from [%SVNSRC%]
echo to   [%SVNDST%]
echo ==============================================================================
xcopy   "%SVNSRC%\..\runtime"          %SVNDST% /E /H /I /Y
move /Y "%SVNSRC%\xxd\xxd.exe"         %SVNDST%
move /Y "%SVNSRC%\GvimExt\gvimext.dll" %SVNDST%
move /Y "%SVNSRC%\vim.exe"             %SVNDST%
move /Y "%SVNSRC%\gvim.exe"            %SVNDST%
move /Y "%SVNSRC%\gvim_noOLE.exe"      %SVNDST%
move /Y "%SVNSRC%\vimrun.exe"          %SVNDST%
move /Y "%SVNSRC%\install.exe"         %SVNDST%
move /Y "%SVNSRC%\uninstal.exe"        %SVNDST%
popd
::endlocal

:: After build additional config you can use modificator %modset%.
if exist "after_settings.bat" echo Found after_settings.bat & call "after_settings.bat"
goto :eof

:nmake_mvc
  nmake /C /S /f Make_mvc.mak clean
  nmake /C /S /f Make_mvc.mak
  if not defined OLE if exist gvim.exe ren gvim.exe gvim_noOLE.exe & echo rename gvim.exe to gvim_noOLE.exe
goto :eof

:set_vcvars
if not defined vcver (
    for %%v in (5 6 7 7.1 8 9 10 11 12 14) do for %%a in (%*) do if /i "vc%%v" == "%%a" set "vcver=%%v"
  )
if not defined vcmod (
::80386 or 80486 or i586 or i686 or i786
    for %%i in (32 x32 86x86)          do for %%a in (%*) do if /i "%%i" == "%%a" set "vcmod=x86"
::amd64
    for %%i in (amd64 64 x64)          do for %%a in (%*) do if /i "%%i" == "%%a" set "vcmod=x64"
::from x86 build amd64
    for %%i in (x86_amd64)             do for %%a in (%*) do if /i "%%i" == "%%a" set "vcmod=%%i"
::Itanium not tested!
    for %%i in (ia64 x86_ia64)         do for %%a in (%*) do if /i "%%i" == "%%a" set "vcmod=%%i"
::ARM not tested!
    for %%i in (arm x86_arm amd64_arm) do for %%a in (%*) do if /i "%%i" == "%%a" set "vcmod=%%i"
  )
if not defined buildt (
  if "" == "%1" set buildt=ALL
  for %%i in (ALL)   do for %%a in (%*) do if /i "%%i" == "%%a" set "buildt=ALL"
  for %%i in (GUI)   do for %%a in (%*) do if /i "%%i" == "%%a" set "buildt=GUI" & set "OLE=yes"
  for %%i in (CUI)   do for %%a in (%*) do if /i "%%i" == "%%a" set "buildt=CUI"
  for %%i in (noOLE) do for %%a in (%*) do if /i "%%i" == "%%a" set "buildt=GUI"
  )
goto :eof

:set_vcvars_env
if "%vcmod%" == "" if exist "%PROGRAMFILES(X86)%"     set "vcmod=x64"
if "%vcmod%" == "" if not exist "%PROGRAMFILES(X86)%" set "vcmod=x86"
if defined V5TOOLS set "VS50COMNTOOLS=%V5TOOLS%"
if defined V6TOOLS set "VS60COMNTOOLS=%V6TOOLS%"

  if defined VS140COMNTOOLS if exist "%VS140COMNTOOLS%\..\..\vc\vcvarsall.bat"  set "vcver=14"
  if defined VS120COMNTOOLS if exist "%VS120COMNTOOLS%\..\..\vc\vcvarsall.bat"  set "vcver=12"
  if defined VS110COMNTOOLS if exist "%VS110COMNTOOLS%\..\..\vc\vcvarsall.bat"  set "vcver=11"
  if defined VS100COMNTOOLS if exist "%VS100COMNTOOLS%\..\..\vc\vcvarsall.bat"  set "vcver=10"
  if defined VS90COMNTOOLS  if exist "%VS90COMNTOOLS%\..\..\vc\vcvarsall.bat"   set "vcver=9"
  if defined VS80COMNTOOLS  if exist "%VS80COMNTOOLS%\..\..\vc\vcvarsall.bat"   set "vcver=8"
  if defined VS71COMNTOOLS  if exist "%VS71COMNTOOLS%\..\..\vc\vcvarsall.bat"   set "vcver=7.1"
  if defined VS70COMNTOOLS  if exist "%VS70COMNTOOLS%\..\..\vc\vcvarsall.bat"   set "vcver=7"
  if defined VS60COMNTOOLS  if exist "%VS60COMNTOOLS%\VC98\Bin\vcvars32.bat"    set "vcver=6"
  if defined VS60COMNTOOLS  if exist "%VS60COMNTOOLS%\VC97\Bin\vcvars32.bat"    set "vcver=5"
  if defined VS50COMNTOOLS  if exist "%VS50COMNTOOLS%\VC\Bin\vcvars32.bat"      set "vcver=5"

  if "%vcver%" == "14"  call "%VS140COMNTOOLS%\..\..\vc\vcvarsall.bat" %vcmod%
  if "%vcver%" == "12"  call "%VS120COMNTOOLS%\..\..\vc\vcvarsall.bat" %vcmod%
  if "%vcver%" == "11"  call "%VS110COMNTOOLS%\..\..\vc\vcvarsall.bat" %vcmod%
  if "%vcver%" == "10"  call "%VS100COMNTOOLS%\..\..\vc\vcvarsall.bat" %vcmod%
  if "%vcver%" == "9"   call "%VS90COMNTOOLS%\..\..\vc\vcvarsall.bat"  %vcmod%
  if "%vcver%" == "8"   call "%VS80COMNTOOLS%\..\..\vc\vcvarsall.bat"  %vcmod%
  if "%vcver%" == "7.1" call "%VS71COMNTOOLS%\..\..\vc\vcvarsall.bat"  %vcmod%
  if "%vcver%" == "7"   call "%VS70COMNTOOLS%\..\..\vc\vcvarsall.bat"  %vcmod%
  if "%vcver%" == "6"   call "%VS60COMNTOOLS%\VC98\Bin\vcvars32.bat"   %vcmod%
  if "%vcver%" == "5"   call "%VS60COMNTOOLS%\VC97\Bin\vcvars32.bat"   %vcmod%
  if "%vcver%" == "5"   call "%VS50COMNTOOLS%\VC\Bin\vcvars32.bat"     %vcmod%
goto :eof

:usage
  title "Command Prompt Error in script usage. Defoult configuration!"
  echo ================ Error in script usage. Defoult configuration! ===============
  echo:
  echo ==================== Visual Studio Autodetect Configuration ==================
  echo Usage: msvc.bat [options] or Use Autodetect Configuration.
  echo ==============================================================================
  echo Options:      Auto detect respond Visual Studio Versions and use (vc%vcver%):
  echo    vc5        %VS50COMNTOOLS%\VC\Bin\vcvars32.bat
  echo               %VS60COMNTOOLS%\VC97\Bin\vcvars32.bat
  echo    vc6        %VS60COMNTOOLS%\VC98\Bin\vcvars32.bat
  echo    vc7        %VS70COMNTOOLS%\..\..\vc\vcvarsall.bat
  echo    vc7.1      %VS71COMNTOOLS%\..\..\vc\vcvarsall.bat
  echo    vc8        %VS80COMNTOOLS%\..\..\vc\vcvarsall.bat
  echo    vc9        %VS90COMNTOOLS%\..\..\vc\vcvarsall.bat
  echo    vc10       %VS100COMNTOOLS%\..\..\vc\vcvarsall.bat
  echo    vc11       %VS110COMNTOOLS%\..\..\vc\vcvarsall.bat
  echo    vc12       %VS120COMNTOOLS%\..\..\vc\vcvarsall.bat
  echo    vc14       %VS140COMNTOOLS%\..\..\vc\vcvarsall.bat
  echo ==============================================================================
  echo               Auto detect respond your architecture (CPU %vcmod%)
  echo    x32        vcmod=x86
  echo     32        vcmod=x86
  echo    x86        vcmod=x86
  echo     86        vcmod=x86
  echo     64        vcmod=x64
  echo    x64        vcmod=x64
  echo    amd64      vcmod=x64
  echo    x86_amd64  vcmod=x86_amd64 (Use if yourse PC have not installed x64 system)
  echo ==============================================================================
  echo               Auto detect build type (ALL)
  echo    ALL        Full Build (GUI+CUI+noOLE)
  echo    CUI        CUI  Build
  echo    GUI        GUI  Build
  echo    noOLE        GUI  Build without OLE
  echo ==============================================================================
  echo For example:
  echo     msvc.bat vc%vcver% %vcmod% ALL
  echo:
  echo Auto detect build.
  pause
goto :eof