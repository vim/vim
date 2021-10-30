@echo off
setlocal enableextensions enabledelayedexpansion
path %SystemRoot%\System32;%SystemRoot%;%SystemRoot%\System32\Wbem

:: Unattended install flag. When set, the script will not require user input.
set unattended=no
if "%1"=="/u" set unattended=yes

:: Make sure this is Windows Vista or later
call :ensure_vista

:: Command line arguments to use when launching gvim from a file association
set gvim_args=

:: Get gvim.exe location
set gvim_path=%~dp0gvim.exe
if not exist "%gvim_path%" call :die "gvim.exe not found"

:: Get vim.ico location
set icon_path=%~dp0vim.ico
if not exist "%icon_path%" call :die "vim.ico not found"

:: Register gvim.exe under the "App Paths" key, so it can be found by
:: ShellExecute, the run command, the start menu, etc.
set app_paths_key=HKCU\SOFTWARE\Microsoft\Windows\CurrentVersion\App Paths\gvim.exe
call :reg add "%app_paths_key%" /d "%gvim_path%" /f
call :reg add "%app_paths_key%" /v "UseUrl" /t REG_DWORD /d 1 /f

:: Register gvim.exe under the "Applications" key to add some default verbs for
:: when gvim is used from the "Open with" menu
set classes_root_key=HKCU\SOFTWARE\Classes
set app_key=%classes_root_key%\Applications\gvim.exe
call :reg add "%app_key%" /v "FriendlyAppName" /d "gvim" /f
call :add_verbs "%app_key%"

:: Add a capabilities key for gvim, which is registered later on for use in the
:: "Default Programs" control panel
set capabilities_key=HKCU\SOFTWARE\Clients\Editor\gvim\Capabilities
call :reg add "%capabilities_key%" /v "ApplicationName" /d "gvim" /f
call :reg add "%capabilities_key%" /v "ApplicationDescription" /d "gvim editor" /f

:: Add file types
set supported_types_key=%app_key%\SupportedTypes
set file_associations_key=%capabilities_key%\FileAssociations

rem :add_type
rem 	set mime_type=%~1
rem 	set perceived_type=%~2
rem 	set friendly_name=%~3
rem 	set extension=%~4

call :add_type "text/plain"                        "text" "Text"                        ".txt"
call :add_type "text/plain"                        "text" "Markdown"                    ".md" ".mkd" ".markdown"
call :add_type "text/plain"                        "text" "Restructured Text"           ".rst"
call :add_type "text/plain"                        "text" "INI Configuration"           ".ini"
call :add_type "text/plain"                        "text" "Information"                 ".info"

:: Register "Default Programs" entry
call :reg add "HKCU\SOFTWARE\RegisteredApplications" /v "gvim" /d "SOFTWARE\Clients\Editor\gvim\Capabilities" /f

echo.
echo Installed successfully^^! You can now configure gvim's file associations in the
echo Default Programs control panel.
echo.
if [%unattended%] == [yes] exit 0
<nul set /p =Press any key to open the Default Programs control panel . . .
pause >nul
control /name Microsoft.DefaultPrograms
exit 0

:die
	if not [%1] == [] echo %~1
	if [%unattended%] == [yes] exit 1
	pause
	exit 1

:ensure_vista
	ver | find "XP" >nul
	if not errorlevel 1 (
		echo This batch script only works on Windows Vista and later. To create file
		echo associations on Windows XP, right click on a text file and use "Open with...".
		call :die
	)
	goto :EOF

:reg
	:: Wrap the reg command to check for errors
	>nul reg %*
	if errorlevel 1 set error=yes
	if [%error%] == [yes] echo Error in command: reg %*
	if [%error%] == [yes] call :die
	goto :EOF

:reg_set_opt
	:: Set a value in the registry if it doesn't already exist
	set key=%~1
	set value=%~2
	set data=%~3

	reg query "%key%" /v "%value%" >nul 2>&1
	if errorlevel 1 call :reg add "%key%" /v "%value%" /d "%data%"
	goto :EOF

:add_verbs
	set key=%~1

	:: Add "open" verb
	call :reg add "%key%\shell\open" /d "&Open" /f
	:: Set open command
	call :reg add "%key%\shell\open\command" /d "\"%gvim_path%\" %gvim_args% -- \"%%%%L" /f
	:: Set the default verb to "open"
	call :reg add "%key%\shell" /d "open" /f

	goto :EOF

:add_progid
	set prog_id=%~1
	set friendly_name=%~2

	:: Add ProgId, edit flags are FTA_OpenIsSafe | FTA_AlwaysUseDirectInvoke
	set prog_id_key=%classes_root_key%\%prog_id%
	call :reg add "%prog_id_key%" /d "%friendly_name%" /f
	call :reg add "%prog_id_key%" /v "EditFlags" /t REG_DWORD /d 4259840 /f
	call :reg add "%prog_id_key%" /v "FriendlyTypeName" /d "%friendly_name%" /f
	call :reg add "%prog_id_key%\DefaultIcon" /d "%icon_path%" /f
	call :add_verbs "%prog_id_key%"

	goto :EOF

:update_extension
	set extension=%~1
	set prog_id=%~2
	set mime_type=%~3
	set perceived_type=%~4

	:: Add information about the file extension, if not already present
	set extension_key=%classes_root_key%\%extension%
	if not [%mime_type%] == [] call :reg_set_opt "%extension_key%" "Content Type" "%mime_type%"
	if not [%perceived_type%] == [] call :reg_set_opt "%extension_key%" "PerceivedType" "%perceived_type%"
	call :reg add "%extension_key%\OpenWithProgIds" /v "%prog_id%" /f

	:: Add type to SupportedTypes
	call :reg add "%supported_types_key%" /v "%extension%" /f

	:: Add type to the Default Programs control panel
	call :reg add "%file_associations_key%" /v "%extension%" /d "%prog_id%" /f

	goto :EOF

:add_type
	set mime_type=%~1
	set perceived_type=%~2
	set friendly_name=%~3
	set extension=%~4

	echo Adding "%extension%" file type

	:: Add ProgId
	set prog_id=gvim%extension%
	call :add_progid "%prog_id%" "%friendly_name%"

	:: Add extensions
	:extension_loop
		call :update_extension "%extension%" "%prog_id%" "%mime_type%" "%perceived_type%"

		:: Trailing parameters are additional extensions
		shift /4
		set extension=%~4
		if not [%extension%] == [] goto extension_loop

	goto :EOF
