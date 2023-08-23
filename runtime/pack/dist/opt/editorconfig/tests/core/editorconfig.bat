@echo off
:: editorconfig.bat: First-level invoker for editorconfig-core-vimscript
:: and editorconfig-vim.
:: Just passes the full command line to editorconfig1.vbs, since VBScript
:: applies very simple quoting rules when it parses a command line.
:: Copyright (c) 2018--2019 Chris White.  All rights reserved.
:: Licensed CC-BY-SA, version 3.0 or any later version, at your option.
set here=%~dp0

cscript //Nologo "%here%editorconfig1.vbs" %*
:: %* has the whole command line
