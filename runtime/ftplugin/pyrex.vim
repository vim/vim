" Vim filetype plugin file
" Language:	Pyrex
" Maintainer:	Marco Barisione <marco.bari@people.it>
" URL:		http://marcobari.altervista.org/pyrex_vim.html
" Last Change:	2004 May 16

" Only do this when not done yet for this buffer
if exists("b:did_ftplugin")
  finish
endif

" Behaves just like Python
runtime! ftplugin/python.vim ftplugin/python_*.vim ftplugin/python/*.vim

if has("gui_win32") && exists("b:browsefilter")
    let  b:browsefilter = "Pyrex files (*.pyx,*.pxd)\t*.pyx;*.pxd\n" .
			\ "Python Files (*.py)\t*.py\n" .
			\ "C Source Files (*.c)\t*.c\n" .
			\ "C Header Files (*.h)\t*.h\n" .
			\ "C++ Source Files (*.cpp *.c++)\t*.cpp;*.c++\n" .
			\ "All Files (*.*)\t*.*\n"
endif
