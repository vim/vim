" Vim syntax file
" Language:    R noweb Files
" Maintainer:  Johannes Ranke <jranke@uni-bremen.de>
" Last Change: 2006 Apr 18
" Version:     0.5 
" SVN:	       $Id$
" Remarks:     - This file is inspired by the proposal of 
"				 Fernando Henrique Ferraz Pereira da Rosa <feferraz@ime.usp.br>
"			     http://www.ime.usp.br/~feferraz/en/sweavevim.html
"			   - It extends some clusters from syntax/tex.vim (version 33,
"			   	 March 06 2006) and therefore depends on some contents 
"			     of this file
"

" Version Clears: {{{1
" For version 5.x: Clear all syntax items
" For version 6.x and 7.x: Quit when a syntax file was already loaded
if version < 600 
  syntax clear
elseif exists("b:current_syntax")
  finish
endif 

syn case match

" Extension of Tex regions {{{1
runtime syntax/tex.vim
unlet b:current_syntax

syn cluster texDocGroup		contains=texPartZone,@texPartGroup,@rnoweb
syn cluster texPartGroup		contains=texChapterZone,texSectionZone,texParaZone,@rnoweb
syn cluster texChapterGroup		contains=texSectionZone,texParaZone,@rnoweb
syn cluster texSectionGroup		contains=texSubSectionZone,texParaZone,@rnoweb
syn cluster texSubSectionGroup		contains=texSubSubSectionZone,texParaZone,@rnoweb
syn cluster texSubSubSectionGroup	contains=texParaZone,@rnoweb
syn cluster texParaGroup		contains=texSubParaZone,@rnoweb

" Highlighting of R code using an existing r.vim syntax file if available {{{1
syn include @rnowebR syntax/r.vim
syn region rnowebChunk matchgroup=rnowebDelimiter start="^<<.*>>=" matchgroup=rnowebDelimiter end="^@" contains=@rnowebR,rnowebChunkReference,rnowebChunk keepend
syn match rnowebChunkReference "^<<.*>>$" contained
syn region rnowebSexpr matchgroup=Delimiter start="\\Sexpr{" matchgroup=Delimiter end="}" contains=@rnowebR

" Sweave options command {{{1
syn region rnowebSweaveopts matchgroup=Delimiter start="\\SweaveOpts{" matchgroup=Delimiter end="}"

" rnoweb Cluster {{{1
syn cluster rnoweb contains=rnowebChunk,rnowebChunkReference,rnowebDelimiter,rnowebSexpr,rnowebSweaveopts

" Highlighting {{{1
hi def link rnowebDelimiter	Delimiter
hi def link rnowebSweaveOpts Statement

let   b:current_syntax = "rnoweb"
" vim: foldmethod=marker:
