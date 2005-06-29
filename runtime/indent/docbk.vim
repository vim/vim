" Vim indent file
" Language:         DocBook Documentation Format
" Maintainer:       Nikolai Weibull <nikolai+work.vim@bitwi.se>
" Latest Revision:  2005-06-29

if exists("b:did_indent")
  finish
endif

" Same as XML indenting for now.
runtime! indent/xml.vim

if exists('*XmlIndentGet')
  setlocal indentexpr=XmlIndentGet(v:lnum,0)
endif
