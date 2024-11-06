" Vim syntax file
" Language:    Literate Idris 2
" Maintainer:  Idris Hackers (https://github.com/edwinb/idris2-vim)
" Last Change: 2020 May 19
" Version:     0.1
" License:     Vim (see :h license)
" Repository:  https://github.com/ShinKage/idris2-nvim
"
" This is just a minimal adaption of the Literate Haskell syntax file.

" quit when a syntax file was already loaded
if exists("b:current_syntax")
  finish
endif

" Read Idris highlighting.
syntax include @idrisTop syntax/idris2.vim

" Recognize blocks of Bird tracks, highlight as Idris.
syntax region lidrisBirdTrackBlock start="^>" end="\%(^[^>]\)\@=" contains=@idrisTop,lidrisBirdTrack
syntax match  lidrisBirdTrack "^>" contained

hi def link   lidrisBirdTrack Comment

let b:current_syntax = "lidris2"
