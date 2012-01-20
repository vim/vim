" zipPlugin.vim: Handles browsing zipfiles
"            PLUGIN PORTION
" Date:			Aug 15, 2011
" Maintainer:	Charles E Campbell, Jr <NdrOchip@ScampbellPfamily.AbizM-NOSPAM>
" License:		Vim License  (see vim's :help license)
" Copyright:    Copyright (C) 2005-2011 Charles E. Campbell, Jr. {{{1
"               Permission is hereby granted to use and distribute this code,
"               with or without modifications, provided that this copyright
"               notice is copied with it. Like anything else that's free,
"               zipPlugin.vim is provided *as is* and comes with no warranty
"               of any kind, either expressed or implied. By using this
"               plugin, you agree that in no event will the copyright
"               holder be liable for any damages resulting from the use
"               of this software.
"
" (James 4:8 WEB) Draw near to God, and he will draw near to you.
" Cleanse your hands, you sinners; and purify your hearts, you double-minded.
" ---------------------------------------------------------------------
" Load Once: {{{1
if &cp || exists("g:loaded_zipPlugin")
 finish
endif
let g:loaded_zipPlugin = "v25"
let s:keepcpo          = &cpo
set cpo&vim

" ---------------------------------------------------------------------
" Public Interface: {{{1
augroup zip
 au!
 au BufReadCmd   zipfile:*	call zip#Read(expand("<amatch>"), 1)
 au FileReadCmd  zipfile:*	call zip#Read(expand("<amatch>"), 0)
 au BufWriteCmd  zipfile:*	call zip#Write(expand("<amatch>"))
 au FileWriteCmd zipfile:*	call zip#Write(expand("<amatch>"))

 if has("unix")
  au BufReadCmd   zipfile:*/*	call zip#Read(expand("<amatch>"), 1)
  au FileReadCmd  zipfile:*/*	call zip#Read(expand("<amatch>"), 0)
  au BufWriteCmd  zipfile:*/*	call zip#Write(expand("<amatch>"))
  au FileWriteCmd zipfile:*/*	call zip#Write(expand("<amatch>"))
 endif

 au BufReadCmd   *.zip,*.jar,*.xpi,*.war,*.ear,*.celzip,*.oxt		call zip#Browse(expand("<amatch>"))
augroup END

" ---------------------------------------------------------------------
"  Restoration And Modelines: {{{1
"  vim: fdm=marker
let &cpo= s:keepcpo
unlet s:keepcpo
