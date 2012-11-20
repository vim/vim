:: command to build big Vim 64 bit with OLE, Perl, Python, Ruby and Tcl
:: First run: %VCDIR%\vcvarsall.bat x86_amd64
:: Ruby and Tcl are excluded, doesn't seem to work.
SET VCDIR="C:\Program Files (x86)\Microsoft Visual Studio 9.0\VC\"
SET TOOLDIR=E:\
%VCDIR%\bin\nmake -f Make_mvc.mak CPU=AMD64 GUI=yes OLE=yes PERL=E:\perl514 DYNAMIC_PERL=yes PERL_VER=514 PYTHON=%TOOLDIR%python27 DYNAMIC_PYTHON=yes PYTHON_VER=27 PYTHON3=%TOOLDIR%python32 DYNAMIC_PYTHON3=yes PYTHON3_VER=32  %1 IME=yes CSCOPE=yes

