vim9script

# Vim plugin for OSC52 clipboard support
#
# Maintainer:	The Vim Project <https://github.com/vim/vim>
# Last Change:	2025 Dec 18

if !has("timers")
  finish
endif

import autoload "../autoload/osc52.vim" as osc

v:clipproviders["osc52"] = {
  "available": osc.Available,
  "paste": {
    "*": osc.Paste,
    "+": osc.Paste
  },
  "copy": {
    "*": osc.Copy,
    "+": osc.Copy
  },
}

def SendDA1(): void
  if !has("gui_running") && !get(g:, 'osc52_force_avail', 0)
      && !get(g:, 'osc52_no_da1', 0)
    echoraw("\<Esc>[c")
  endif
enddef

if v:vim_did_enter
  SendDA1()
endif

augroup VimOSC52Plugin
  autocmd!
  # Query support for OSC 52 using a DA1 query
  autocmd TermResponseAll da1 {
    if match(v:termda1, '?\zs.*52\ze') != -1
      osc.allowed = true
      :silent! clipreset
    else
      osc.allowed = false
      :silent! clipreset
    endif
  }
  autocmd VimEnter * SendDA1()
augroup END

# vim: set sw=2 sts=2 :
