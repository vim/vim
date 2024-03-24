@ 2>/dev/null # 2>nul & goto:win32
#!/bin/sh
if test -e ../src/vim.exe ; then mv ../src/vim.exe ../src/vimw32.exe ; fi
if test -e ../src/vim.pdb ; then mv ../src/vim.pdb ../src/vimw32.pdb ; fi
if test -e ../src/gvim.exe ; then mv ../src/gvim.exe ../src/gvim_ole.exe ; fi
if test -e ../src/gvim.pdb ; then mv ../src/gvim.pdb ../src/gvim_ole.pdb ; fi
if test -e ../src/install.exe ;
 then
 mv ../src/install.exe ../src/installw32.exe ;
fi
if test -e ../src/uninstall.exe ;
 then
 mv ../src/uninstall.exe ../src/uninstallw32.exe ;
fi
if test -e ../src/tee/tee.exe ;
 then
 mv ../src/tee/tee.exe ../src/teew32.exe ; 
fi
if test -e ../src/xxd/xxd.exe ;
 then
 mv ../src/xxd/xxd.exe ../src/xxdw32.exe ; 
fi
# Uncomment return if the file is run through the command "source"
#return
exit

:win32
if exist mv.exe (set "mv=mv.exe -f") else (set "mv=move /y")
if exist ..\src\vim.exe %mv% ..\src\vim.exe ..\src\vimw32.exe
if exist ..\src\vim.pdb %mv% ..\src\vim.pdb ..\src\vimw32.pdb
if exist ..\src\gvim.exe %mv% ..\src\gvim.exe ..\src\gvim_ole.exe
if exist ..\src\gvim.pdb %mv% ..\src\gvim.pdb ..\src\gvim_ole.pdb
if exist ..\src\install.exe %mv% ..\src\install.exe ..\src\installw32.exe
if exist ..\src\uninstall.exe %mv% ..\src\uninstall.exe ..\src\uninstallw32.exe
if exist ..\src\tee\tee.exe %mv% ..\src\tee\tee.exe ..\src\teew32.exe
if exist ..\src\xxd\xxd.exe %mv% ..\src\xxd\xxd.exe ..\src\xxdw32.exe
set "mv="
goto:EOF
