@echo off
rem To be used on MS-Windows for Visual C++ 2017 Express Edition
rem   aka Microsoft Visual Studio 15
rem See INSTALLpc.txt for information.

rem Fast way: open Folder of VIM/SRC in VC2017
rem use "Developer command prompt" from from Solution Explorer window (RKM - Main folder of project)
rem and in command prompt type: nmake -f Make_mvc.mak
rem after build vim.exe open it in VC2017 (File-Open) for debug if need

rem Member! Close VIM.EXE in VS2017 before rebuild/rewrite .exe in command prompt
@echo on
call "%ProgramFiles%\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvars32.bat"