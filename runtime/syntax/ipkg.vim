" Vim syntax file
" Language:    Literate Idris 2
" Maintainer:  Idris Hackers (https://github.com/edwinb/idris2-vim)
" Last Change: 2020 May 19
" Version:     0.1
" Author: ShinKage
" License:             Vim (see :h license)
" Repository:          https://github.com/ShinKage/idris2-nvim
"
" Syntax highlight for Idris 2 Package Descriptors (idris-lang.org)
"

if exists("b:current_syntax")
  finish
endif

syn keyword ipkgKey
    \ package
    \ authors
    \ maintainers
    \ license
    \ brief
    \ readme
    \ homepage
    \ sourceloc
    \ bugtracker
    \ options
    \ opts
    \ sourcedir
    \ builddir
    \ outputdir
    \ prebuild
    \ postbuild
    \ preinstall
    \ postinstall
    \ preclean
    \ postclean
    \ version
    \ langversion
    \ modules
    \ main
    \ executable
    \ depends

syn region ipkgString start=+"+ skip=+\\\\\|\\"+ end=+"+ contains=@Spell
syn match ipkgVersion "[0-9]*\([.][0-9]*\)*"
syn match ipkgName "[a-zA-Z][a-zA-z0-9_']*\([.][a-zA-Z][a-zA-z0-9_']*\)*" contained
syn match ipkgOperator "\(,\|&&\|<\|<=\|==\|>=\|>\)"
syn match ipkgComment "---*\([^-!#$%&\*\+./<=>\?@\\^|~].*\)\?$" contains=@Spell

highlight def link ipkgKey Statement
highlight def link ipkgString String
highlight def link ipkgVersion Number
highlight def link ipkgName Identifier
highlight def link ipkgOperator Operator
highlight def link ipkgComment Comment

let b:current_syntax = "ipkg"
