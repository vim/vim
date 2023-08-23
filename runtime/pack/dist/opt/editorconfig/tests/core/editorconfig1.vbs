' editorconfig1.vbs: run by editorconfig.bat
' runs editorconfig2.ps1
' Part of editorconfig-core-vimscript and editorconfig-vim.
'
' Copyright (c) 2018--2019 Chris White.  All rights reserved.
' Licensed CC-BY-SA, version 3.0 or any later version, at your option.
'
' Modified from
' https://stackoverflow.com/a/2470557/2877364 by
' https://stackoverflow.com/users/2441/aphoria

' Thanks to https://www.geekshangout.com/vbs-script-to-get-the-location-of-the-current-script/
currentScriptPath = Replace(WScript.ScriptFullName, WScript.ScriptName, "")

' Load our common library.  Thanks to https://stackoverflow.com/a/316169/2877364
With CreateObject("Scripting.FileSystemObject")
   executeGlobal .openTextFile(currentScriptPath & "ecvbslib.vbs").readAll()
End With

' === MAIN ==================================================================

' Encode all the arguments as modified base64 so there will be no quoting
' issues when we invoke powershell.
b64args = MakeY64Args(Wscript.Arguments)

' Quote script name just in case
ps1name = QuoteForShell(currentScriptPath & "editorconfig2.ps1")
'Wscript.Echo "Script is in " & ps1name

if True then
    retval = RunCommandAndEcho( "powershell.exe" & _
        " -executionpolicy bypass -file " & ps1name & " " & join(b64args) _
    )
        ' add -noexit to leave window open so you can see error messages

    WScript.Quit retval
end if

' vi: set ts=4 sts=4 sw=4 et ai:
