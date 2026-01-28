vim9script

# Vim syntax file.
# Language:    Haredoc (Hare documentation format)
# Maintainer:  Amelia Clarke <selene@perilune.dev>
# Last Change: 2025 Aug 14
# Upstream:    https://git.sr.ht/~sircmpwn/hare.vim

if exists('b:current_syntax')
  finish
endif

# Syntax {{{1
syn case match
syn iskeyword @,48-57,_

# Embedded code samples.
syn region haredocCode start='\t\zs' end='$' contains=@NoSpell display

# References to other declarations and modules.
syn match haredocRef '\v\[\[\h\w*%(::\h\w*)*%(::)?]]' contains=@NoSpell display

# Miscellaneous.
syn keyword haredocTodo FIXME TODO XXX

# Default highlighting {{{1
hi def link haredocCode Comment
hi def link haredocRef Special
hi def link haredocTodo Todo

# Highlight incorrect whitespace by default.
syn match haredocSpaceError '\s\+$' containedin=ALL display
syn match haredocSpaceError '^ \zs \+\ze\t' containedin=ALL display
syn match haredocSpaceError '[^ ]\zs \+\ze\t' containedin=ALL display
if get(g:, 'hare_space_error', 1)
  hi! def link haredocSpaceError Error
else
  hi! def link haredocSpaceError NONE
endif

b:current_syntax = 'haredoc'

# vim: fdm=marker et sts=2 sw=2 ts=8 tw=80
