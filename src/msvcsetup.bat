rem To be used on MS-Windows when using the Visual C++ Toolkit 2003
rem See INSTALLpc.txt for information.

set PATH=%SystemRoot%\Microsoft.NET\Framework\v1.1.4322;%PATH%
call "%VCToolkitInstallDir%vcvars32.bat"
set MSVCVer=7.1

rem The platform SDK can be installed elsewhere, adjust the path.
call "%ProgramFiles%\Microsoft Platform SDK\SetEnv.Cmd"
rem call "e:\Microsoft Platform SDK\SetEnv.Cmd"

set LIB=%ProgramFiles%\Microsoft Visual Studio .NET 2003\Vc7\lib;%LIB%
