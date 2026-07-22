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

set Platform=
if not exist "%VS140COMNTOOLS%..\..\VC\vcvarsall.bat" (
	echo Error: vcvarsall.bat not found.
	exit /b 1
)
call "%VS140COMNTOOLS%..\..\VC\vcvarsall.bat" %*
if defined VisualStudioVersion (
	if defined Platform (
		echo VS 2015 ^(%VisualStudioVersion%^) %Platform%
		title VS 2015 %Platform%
	) else (
		echo VS 2015 ^(%VisualStudioVersion%^) x86
		title VS 2015 x86
	)
)
