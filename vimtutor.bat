:: Start Vim on a copy of the tutor file.
@echo off
SetLocal

:: Usage:
:: vimtutor [/?|{/ | -}h|{/ | --}help] [{/ | -}c|{/ | --}chapter NUMBER] [{/ | --}console] [xx]
::
:: -console means gvim will not be used
:: xx is a language code like "es" or "nl".
:: When an xx argument is given, it tries loading that tutor.
:: When this fails or no xx argument was given, it tries using 'v:lang'
:: When that also fails, it uses the English version.

:: Use Vim to copy the tutor, it knows the value of $VIMRUNTIME
for %%G in (. %TMP% %TEMP%) do (
  call :TestDirWritable "%~f0" %%G
  if not ERRORLEVEL 1 goto DirOk
)

echo:
echo:No working directory is found.
goto End

:TestDirWritable
set TUTORCOPY=%2\$tutor$
copy %1 %TUTORCOPY% 2>&1> nul
goto DelTmpCopy

:DirOk
title Tutorial on the Vim editor
set "use=Gui"

for /F "usebackq tokens=2 delims=:" %%G in (`chcp`) do (
  set /a "_sav_chcp=%%G"
  1> nul chcp 65001
)

:GetChptLngs
for %%G in (tutor1;tutor2) do (
  if exist "%~dp0tutor\%%G" (set "lngs_%%G=en;") else (
    if exist "%~dp0tutor\%%G.utf-8" set "lngs_%%G=en;")
  for /F "tokens=2 delims=._" %%H in (
    '2^> nul dir /B /A:-D "%~dp0tutor\%%G.???.utf-8"') do (
      call set "lngs_%%G=%%lngs_%%G%%%%H;")
)
:EndGetChptLngs

:ParseArgs
if "%*"=="" goto Use%use%
if "%1"=="/?" goto Usage
if "%1"=="/h" goto Usage
if "%1"=="-h" goto Usage
if "%1"=="/help" goto Usage
if "%1"=="--help" goto Usage
if "%1"=="/list" goto List
if "%1"=="--list" goto List
:DoShift
if "%1"=="/c" (call :ChkChpt %2 && (shift & shift & goto DoShift) || goto End)
if "%1"=="-c" (call :ChkChpt %2 && (shift & shift & goto DoShift) || goto End)
if "%1"=="/chapter" (
  call :ChkChpt %2 && (shift & shift & goto DoShift) || goto End
)
if "%1"=="--chapter" (
  call :ChkChpt %2 && (shift & shift & goto DoShift) || goto End
)
if "%1"=="/console" (set "use=Vim" & shift & goto DoShift)
if "%1"=="--console" (set "use=Vim" & shift & goto DoShift)
call :ChkLng %1 && shift || goto End
if not "%1"=="" goto DoShift
goto Use%use%

:UseGui
:: Try making a copy of tutor with gvim.  If gvim cannot be found, try using
:: vim instead.  If vim cannot be found, alert user to check environment and
:: installation.

:: The script tutor.vim tells Vim which file to copy.
start "dummy" /B /W "%~dp0gvim.exe" -u NONE -c "so $VIMRUNTIME/tutor/tutor.vim"
if ERRORLEVEL 1 goto UseVim

:: Start gvim without any .vimrc, set 'nocompatible' and 'showcmd'
start "dummy" /B /W "%~dp0gvim.exe" -u NONE -c "set nocp sc" %TUTORCOPY%

goto End

:UseVim
:: The script tutor.vim tells Vim which file to copy
call "%~dp0vim.exe" -u NONE -c "so $VIMRUNTIME/tutor/tutor.vim"
if ERRORLEVEL 1 goto NoExecutable

:: Start vim without any .vimrc, set 'nocompatible and 'showcmd''
call "%~dp0vim.exe" -u NONE -c "set nocp sc" %TUTORCOPY%

goto End

:NoExecutable
echo:
echo:
echo:No vim or gvim found in current directory or %%PATH%%.
echo:Check your installation or re-run install.exe.

goto End

:ChkChpt
if defined CHAPTER (
  echo:Error. Invalid command line arguments.
  echo:See %~nx0 /? for help.
  exit /B 1
)
for /F %%G in ('echo %1 ^| findstr /R "\<[1-2]\>"') do (
  set "CHAPTER=%%G" & exit /B 0
)
echo:Error. The chapter argument must contain only the digits 1 or 2.
exit /B 1

:ChkLng
if "%1"=="" exit /B 0
if defined xx (
  echo:Error. Invalid command line arguments.
  echo:See %~nx0 /? for help.
  exit /B 1
)
for /F %%G in ('echo %1 ^| findstr /R "[-0-9\._\[\]\$\^\*/!@#&(|)=+\\]"') do (
  echo:Error. The language code must contain only alphabetic characters.
  exit /B 1
)
set "_t=%1"
if ""=="%_t:~1%" (
  echo:Error. The language code must be 2 or 3 characters only.
  exit /B 1
)
if not ""=="%_t:~3%" (
  echo:Error. The language code must be 2 or 3 characters only.
  exit /B 1
)
SetLocal EnableDelayedExpansion
if "!lngs_tutor%CHAPTER%:%1;=!"=="!lngs_tutor%CHAPTER%!" (
  echo:The current installation does not have the %1 language.
  echo:English will be used for the tutorial.
  echo:To view the available languages, use the `%~nx0 /list` command.
  1> nul timeout /T 2
  EndLocal & set "xx=en" & exit /B 0
) else (EndLocal & set "xx=%1" & exit /B 0)

:Usage
echo:
echo:== USAGE =================================================================
echo:
echo:%~nx0 /? ^| ^{/ ^| -^}h ^| ^{/ ^| --^}help
echo:or
echo:%~nx0 ^{/ ^| --^}list
echo:or
echo:%~nx0 ^[^{/ ^| -^}c ^| ^{/ ^| --}chapter NUMBER^] ^[^{/ ^| --^}console^] ^[lng^]
echo:
echo:where:
echo:/? or /h or -h or /help or --help
echo:				Display the quick help and exit.
echo:
echo:/list or --list
echo:				Display the available chapters and languages
echo:				of the tutorial and exit.
echo:
echo:/c or -c or /chapter or --chapter NUMBER
echo:				Specified chapter of the tutorial.
echo:				The NUMBER should be 1 or 2.
echo:				By default, the first chapter.
echo:
echo:/console or --console
echo:				Open the tutorial in the console instead of GUI.
echo:
echo:lng
echo:				Is a 2 or 3 character ISO639 language code
echo:				like "es", "nl" or "bar".
echo:				The default language is English.
echo:
echo:Examples:
echo:	%~nx0 es /c 1 /console
echo:	%~nx0 --chapter 2 de
echo:	%~nx0 fr
echo:

:EndUsage
goto End

:List

:GetLngName
if defined TMP (set "pscult_fl=%TMP%\pscult.tmp") else (
  set "pscult_fl=%TEMP%\pscult.tmp")

powershell.exe -NoLogo -NoProfile -Command ^
[system.globalization.cultureinfo]::GetCultures('AllCultures') ^| ^
Where Name -NotLike "*-*" ^| Where DisplayName -NotLike "Invariant*" ^| ^
%%{$_.Name + \"`t\" + $_.DisplayName + \"`t\" + $_.NativeName} ^| ^
Sort-Object ^| Out-File -FilePath "%pscult_fl%" -Encoding utf8

if defined lngs_tutor1 (set "lngs=%lngs_tutor1%")
if defined lngs_tutor2 if defined lngs (
  for %%G in (%lngs_tutor2%) do (call set "lngs=%%lngs:%%G;=%%")
  set "lngs=%lngs%%lngs_tutor2%"
  ) else (set "lngs=%lngs_tutor2%")

if defined lngs (
  for %%G in (%lngs%) do (
    for /F "tokens=2,* delims=	" %%H in (
      '2^> nul findstr /BR "\<%%G\>" "%pscult_fl%"'
    ) do (set "%%G_name=%%H       %%I")
  )
  set "bar_name=Bavarian       Boarisch"
  set "eo_name=Esperanto       Esperanto"
)
:EndGetLngName

echo:
echo:The following chapters and languages are available in the current
echo:installation tutorial:
echo:
if defined lngs_tutor1 (
  echo:Chapter: 1
  for %%G in (%lngs_tutor1%) do if "en"=="%%G" (
    call echo:%%G	%%%%G_name%%  by default) else (
      call echo:%%G	%%%%G_name%%)
  echo:
)

if defined lngs_tutor2 (
  echo:Chapter: 2
  for %%G in (%lngs_tutor2%) do if "en"=="%%G" (
    call echo:%%G	%%%%G_name%%  by default) else (
      call echo:%%G	%%%%G_name%%)
)
echo:
goto End

:DelTmpCopy
:: remove the copy of the tutor
if exist %TUTORCOPY% del /F /Q %TUTORCOPY%
goto :EOF

:End
:: remove the copy of the tutor and ISO639 file
if exist %TUTORCOPY% del /F /Q %TUTORCOPY%
if exist %pscult_fl% del /F /Q %pscult_fl%
chcp %_sav_chcp% 1> nul
title %ComSpec%
EndLocal

@rem vim:ft=dosbatch:ts=8:sts=2:sw=2:noet:
