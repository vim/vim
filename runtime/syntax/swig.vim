" Vim syntax file
" Language:	SWIG
" Maintainer:	Roman Stanchak (rstanchak@yahoo.com)
" Last Change:	2023 April 27

if exists("b:current_syntax")
	finish
endif

" Read the C++ syntax to start with
runtime! syntax/cpp.vim
unlet b:current_syntax

" SWIG extentions
syn keyword swigDirective %typemap %define %apply %fragment %include %enddef %extend %newobject %name
syn keyword swigDirective %rename %ignore %keyword %typemap %define %apply %fragment %include
syn keyword swigDirective %enddef %extend %newobject %name %rename %ignore %template %module %constant
syn match swigDirective "%\({\|}\)"
syn match swigUserDef "%[-_a-zA-Z0-9]\+"

" Default highlighting
command -nargs=+ HiLink hi def link <args>
HiLink swigDirective	Exception
HiLink swigUserDef 		PreProc
delcommand HiLink

let b:current_syntax = "swig"

" vim: ts=8
