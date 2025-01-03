@ 2>/dev/null # 2>nul & goto:win32
#!/bin/sh
if test -z "$1" ; then src=../src ; else src=$1 ; fi
if test -z "$2" ; then dst=${src} ; else dst=$2 ; fi
if test -f "${src}/vim.exe" ;
 then mv -f "${src}/vim.exe" "${dst}/vimw32.exe" ;
fi
if test -f "${src}/vim.pdb" ;
 then mv -f "${src}/vim.pdb" "${dst}/vimw32.pdb" ;
fi
if test -f "${src}/gvim.exe" ;
 then mv -f "${src}/gvim.exe" "${dst}/gvim_ole.exe" ;
fi
if test -f "${src}/gvim.pdb" ;
 then mv -f "${src}/gvim.pdb" "${dst}/gvim_ole.pdb" ;
fi
if test -f "${src}/install.exe" ;
 then mv "${src}/install.exe" "${dst}/installw32.exe" ;
fi
if test -f "${src}/uninstall.exe" ;
 then mv -f "${src}/uninstall.exe" "${dst}/uninstallw32.exe" ;
fi
if test -f "${src}/tee/tee.exe" ;
 then mv -f "${src}/tee/tee.exe" "${dst}/teew32.exe" ; 
elif test -f "${src}/tee.exe" ;
 then mv -f "${src}/tee.exe" "${dst}/teew32.exe" ; 
fi
if test -f "${src}/xxd/xxd.exe" ;
 then mv -f "${src}/xxd/xxd.exe" "${dst}/xxdw32.exe" ; 
elif  test -f "${src}/xxd.exe" ;
 then mv -f "${src}/xxd.exe" "${dst}/xxdw32.exe" ; 
fi
# Uncomment return if the file is run through the command "source"
#return
exit

:win32
SetLocal
if exist mv.exe (set "mv=mv.exe -f") else (set "mv=move /Y")
if ""=="%~1" (set "src=..\src") else (set "src=%~1")
if ""=="%~2" (set "dst=%src%") else (set "dst=%~2")
if exist "%src%\vim.exe" %mv% "%src%\vim.exe" "%dst%\vimw32.exe"
if exist "%src%\vim.pdb" %mv% "%src%\vim.pdb" "%dst%\vimw32.pdb"
if exist "%src%\gvim.exe" %mv% "%src%\gvim.exe" "%dst%\gvim_ole.exe"
if exist "%src%\gvim.pdb" %mv% "%src%\gvim.pdb" "%dst%\gvim_ole.pdb"
if exist "%src%\install.exe" %mv% "%src%\install.exe" "%dst%\installw32.exe"
if exist "%src%\uninstall.exe" (
    %mv% "%src%\uninstall.exe" "%dst%\uninstallw32.exe"
)
if exist "%src%\tee\tee.exe" (%mv% "%src%\tee\tee.exe" "%dst%\teew32.exe"
    ) else (if exist "%src%\tee.exe" %mv% "%src%\tee.exe" "%dst%\teew32.exe"
)
if exist "%src%\xxd\xxd.exe" (%mv% "%src%\xxd\xxd.exe" "%dst%\xxdw32.exe"
    ) else (if exist "%src%\xxd.exe" %mv% "%src%\xxd.exe" "%dst%\xxdw32.exe"
)
EndLocal
goto:EOF
