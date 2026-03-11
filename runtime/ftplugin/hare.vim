vim9script

# Vim filetype plugin.
# Language:    Hare
# Maintainer:  Amelia Clarke <selene@perilune.dev>
# Last Change: 2026 Jan 24
# Upstream:    https://git.sr.ht/~sircmpwn/hare.vim

if exists('b:did_ftplugin')
  finish
endif
b:did_ftplugin = 1

# Use the Hare compiler.
compiler hare
b:undo_ftplugin = 'compiler make'

# Formatting settings.
setlocal comments=://
setlocal commentstring=//\ %s
setlocal formatlistpat=^\\s*-\\s\\+
setlocal formatoptions+=croqnlj/ formatoptions-=t
b:undo_ftplugin ..= ' | setl cms< com< flp< fo<'

# Locate Hare modules.
&l:include = '\v^\s*use\s+%(\h\w*\s*\=)?'
&l:includeexpr = 'trim(v:fname, ":", 2)->substitute("::", "/", "g")'
setlocal isfname+=:
&l:path = ',,' .. hare#GetPath()
b:undo_ftplugin ..= ' | setl inc< inex< isf< pa<'

# Follow the official style guide by default.
if get(g:, 'hare_recommended_style', 1)
  setlocal noexpandtab
  setlocal shiftwidth=8
  setlocal softtabstop=0
  setlocal tabstop=8
  setlocal textwidth=80
  b:undo_ftplugin ..= ' | setl et< sts< sw< ts< tw<'
endif

# Highlight incorrect whitespace outside of insert mode.
if get(g:, 'hare_space_error', 1)
  augroup HareSpaceError
    autocmd!
    autocmd InsertEnter * hi link hareSpaceError NONE
    autocmd InsertLeave * hi link hareSpaceError Error
  augroup END
endif

# vim: et sts=2 sw=2 ts=8 tw=80
