@echo off
setlocal enableextensions enabledelayedexpansion
path %SystemRoot%\System32;%SystemRoot%;%SystemRoot%\System32\Wbem

:: Unattended install flag. When set, the script will not require user input.
set unattended=no
if "%1"=="/u" set unattended=yes

:: Delete "App Paths" entry
reg delete "HKCU\SOFTWARE\Microsoft\Windows\CurrentVersion\App Paths\gvim.exe" /f >nul

:: Delete HKCR subkeys
set classes_root_key=HKCU\SOFTWARE\Classes
reg delete "%classes_root_key%\Applications\gvim.exe" /f >nul

:: Delete "Default Programs" entry
reg delete "HKCU\SOFTWARE\RegisteredApplications" /v "gvim" /f >nul
reg delete "HKCU\SOFTWARE\Clients\Editor\gvim\Capabilities" /f >nul

:: Delete all OpenWithProgIds referencing ProgIds that start with gvim.
for /f "usebackq eol= delims=" %%k in (`reg query "%classes_root_key%" /f "gvim.*" /s /v /c`) do (
	set "key=%%k"
	echo !key!| findstr /r /i "^HKEY_CURRENT_USER\\SOFTWARE\\Classes\\\.[^\\][^\\]*\\OpenWithProgIds$" >nul
	if not errorlevel 1 (
		for /f "usebackq eol= tokens=1" %%v in (`reg query "!key!" /f "gvim.*" /v /c`) do (
			set "value=%%v"
			echo !value!| findstr /r /i "^gvim\.[^\\][^\\]*$" >nul
			if not errorlevel 1 (
				echo Deleting !key!\!value!
				reg delete "!key!" /v "!value!" /f >nul
			)
		)
	)
)

:: Delete all ProgIds starting with gvim.
for /f "usebackq eol= delims=" %%k in (`reg query "%classes_root_key%" /f "gvim.*" /k /c`) do (
	set "key=%%k"
	echo !key!| findstr /r /i "^HKEY_CURRENT_USER\\SOFTWARE\\Classes\\gvim\.[^\\][^\\]*$" >nul
	if not errorlevel 1 (
		echo Deleting !key!
		reg delete "!key!" /f >nul
	)
)

echo Uninstalled successfully
if [%unattended%] == [yes] exit 0
pause
exit 0

:die
	if not [%1] == [] echo %~1
	if [%unattended%] == [yes] exit 1
	pause
	exit 1
