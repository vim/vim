" Vim filetype plugin file
" Language:	Kimwitu++
" Maintainer:	Michael Piefel <piefel@informatik.hu-berlin.de>
" Last Change:	16 August 2001

" Behaves almost like C++
runtime! ftplugin/cpp.vim ftplugin/cpp_*.vim ftplugin/cpp/*.vim

set cpo-=C

" Limit the browser to related files
if has("gui_win32") && !exists("b:browsefilter")
    let b:browsefilter = "Kimwitu/Kimwitu++ Files (*.k)\t*.k\n" .
		\ "Lex/Flex Files (*.l)\t*.l\n" .
		\ "Yacc/Bison Files (*.y)\t*.y\n" .
		\ "All Files (*.*)\t*.*\n"
endif

" Set the errorformat for the Kimwitu++ compiler
set efm+=kc%.%#:\ error\ at\ %f:%l:\ %m
