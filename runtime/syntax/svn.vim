" Vim syntax file
" Language:     Subversion (svn) commit file
" Maintainer:   Dmitry Vasiliev <dima at hlabs dot org>
" URL:          https://github.com/hdima/vim-scripts/blob/master/syntax/svn.vim
" Last Change:  2012-07-21
" Filenames:    svn-commit*.tmp
" Version:      1.9

" Contributors:
"   Stefano Zacchiroli
"   A. S. Budden
"   Myk Taylor
"   Ingo Karkat

" For version 5.x: Clear all syntax items.
" For version 6.x: Quit when a syntax file was already loaded.
if version < 600
  syntax clear
elseif exists("b:current_syntax")
  finish
endif

syn region svnText      start="\%^" end="^--.*--$"me=s-1 contains=@Spell

syn region svnRegion    start="^--.*--$" end="\%$" contains=ALL
syn match svnRemoved    "^D    .*$" contained
syn match svnRenamed    "^R[ M][ U][ +] .*$" contained
syn match svnAdded      "^A[ M][ U][ +] .*$" contained
syn match svnModified   "^M[ M][ U]  .*$" contained
syn match svnProperty   "^_M[ U]  .*$" contained

" Synchronization.
syn sync clear
syn sync match svnSync  grouphere svnRegion "^--.*--$"me=s-1

" Define the default highlighting.
" For version 5.7 and earlier: only when not done already.
" For version 5.8 and later: only when an item doesn't have highlighting yet.
if version >= 508 || !exists("did_svn_syn_inits")
  if version <= 508
    let did_svn_syn_inits = 1
    command -nargs=+ HiLink hi link <args>
  else
    command -nargs=+ HiLink hi def link <args>
  endif

  HiLink svnRegion      Comment
  HiLink svnRemoved     Constant
  HiLink svnAdded       Identifier
  HiLink svnModified    Special
  HiLink svnProperty    Special
  HiLink svnRenamed     Special

  delcommand HiLink
endif

let b:current_syntax = "svn"
