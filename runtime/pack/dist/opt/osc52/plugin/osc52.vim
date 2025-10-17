vim9script

# Vim plugin for OSC52 clipboard support
#
# Maintainer:	The Vim Project <https://github.com/vim/vim>
# Last Change:	2025 October 14

import autoload "../autoload/osc52.vim" as osc

def Available(): string
  if get(g:, 'osc52_force_avail', 0)
    return "+*"
  endif

  # Send DA1 request
  augroup VimOSC52DA1
    autocmd!
    autocmd TermResponseAll da1 ++once call feedkeys("\<F30>", '!')
  augroup END

  call echoraw("\<Esc>[c")

  # Wait for response from terminal
  while getchar(-1) != "\<F30>"
  endwhile
  autocmd! VimOSC52DA1

  # If there is a 52 parameter, then the terminal supports OSC 52
  if match(v:termda1, ';\zs52\ze') != -1
    return "+*"
  endif
  return ""
enddef

v:clipproviders["osc52"] = {
  "available": function("Available"),
  "paste": {
    "*": osc.Paste,
    "+": osc.Paste
  },
  "copy": {
    "*": osc.Copy,
    "+": osc.Copy
  },
}

# vim: set sw=2 sts=2 :
