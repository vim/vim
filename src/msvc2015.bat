@echo off
rem To be used on MS-Windows for Visual C++ 2015 (either Express or Community)
rem See INSTALLpc.txt for information.
rem
rem Usage:
rem   For x86 builds run this without options:
rem     msvc2015
rem   For x64 builds run this with "x86_amd64" option:
rem     msvc2015 x86_amd64
rem   This works on any editions including Express edition.
rem   If you use Community (or Professional) edition, you can also use "x64"
rem   option:
rem     msvc2015 x64
@echo on

call "%VS140COMNTOOLS%..\..\VC\vcvarsall.bat" %*

rem Use Windows SDK 7.1A for targeting Windows XP.
if "%ProgramFiles(x86)%"=="" (
	set "WinSdk71=%ProgramFiles%\Microsoft SDKs\Windows\v7.1A"
) else (
	set "WinSdk71=%ProgramFiles(x86)%\Microsoft SDKs\Windows\v7.1A"
)
if not exist "%WinSdk71%" (
	echo Windows SDK 7.1A is not found.  Targeting Windows Vista and later.
	goto :eof
)

set INCLUDE=%WinSdk71%\Include;%INCLUDE%
if /i "%Platform%"=="x64" (
	set "LIB=%WinSdk71%\Lib\x64;%LIB%"
	set SUBSYSTEM_VER=5.02
) else (
	set "LIB=%WinSdk71%\Lib;%LIB%"
	set SUBSYSTEM_VER=5.01
)
set CL=/D_USING_V110_SDK71_
