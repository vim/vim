" Vim indent file
" Language:	    DocBook Documentation Format
" Maintainer:	    Nikolai Weibull <source@pcppopper.org>
" URL:		    http://www.pcppopper.org/vim/indent/pcp/docbk/
" Latest Revision:  2004-05-22
" arch-tag:	    3d073af7-1d69-42a2-99ad-9a49a21eb28f

" Same as XML indenting for now.
runtime! indent/xml.vim

setlocal indentexpr=XmlIndentGet(v:lnum,0)

" vim: set sts=2 sw=2:
