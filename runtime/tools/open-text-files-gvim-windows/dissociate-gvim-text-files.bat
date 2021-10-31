@echo off
setlocal enableextensions enabledelayedexpansion
path %SystemRoot%\System32;%SystemRoot%;%SystemRoot%\System32\Wbem

:: Unattended install flag. When set, the script will not require user input.
set unattended=no
if "%1"=="/u" set unattended=yes

:: Make sure the script is running as admin
call :ensure_admin

:: Delete "App Paths" entry
reg delete "HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\App Paths\gvim.exe" /f >nul

:: Delete HKCR subkeys
set classes_root_key=HKCU\SOFTWARE\Classes
reg delete "%classes_root_key%\Applications\gvim.exe" /f >nul

:: Delete "Default Programs" entry
reg delete "HKLM\SOFTWARE\RegisteredApplications" /v "gvim" /f >nul
reg delete "HKLM\SOFTWARE\Clients\Editor\gvim\Capabilities" /f >nul

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

:ensure_admin
:: 'openfiles' is just a commmand that is present on all supported Windows
:: versions, requires admin privileges and has no side effects, see:
:: https://stackoverflow.com/questions/4051883/batch-script-how-to-check-for-admin-rights
openfiles >nul 2>&1
if errorlevel 1 (
	echo This batch script requires administrator privileges. Right-click on
	echo mpv-install.bat and select "Run as administrator".
	call :die
)
goto :EOF

:die
if not [%1] == [] echo %~1
if [%unattended%] == [yes] exit 1
pause
exit 1
