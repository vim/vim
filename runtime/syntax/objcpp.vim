" Vim syntax file
" Language:     ObjC++
" Maintainer:   Anthony Hodsdon <ahodsdon@fastmail.fm>
" Last change:  2003 Apr 25

" For version 5.x: Clear all syntax items
" For version 6.x: Quit when a syntax file was already loaded
if version < 600
   syntax clear
elseif exists("b:current_syntax")
   finish
endif

" Read in C++ and ObjC syntax files
if version < 600
   so <sfile>:p:h/cpp.vim
   so <sflie>:p:h/objc.vim
else
   runtime! syntax/cpp.vim
   unlet b:current_syntax
   runtime! syntax/objc.vim
endif

" Note that we already have a region for method calls ( [objc_class method] )
" by way of cBracket.
syn region objCFunc start="^\s*[-+]"  end="$"  contains=ALLBUT,cErrInParen,cErrInBracket

syn keyword objCppNonStructure    class template namespace transparent contained
syn keyword objCppNonStatement    new delete friend using transparent contained

let b:current_syntax = "objcpp"
