vim9script

# Vim plugin for OSC52 clipboard support
#
# Maintainer:	The Vim Project <https://github.com/vim/vim>
# Last Change:	2025 October 14

if exists("loaded_osc52")
  finish
endif
g:loaded_osc52 = 1

def Available(): string
  return "+*"
enddef

def Paste(reg: string, type: string): any
  if type == "implicit"
    return "previous"
  endif

  augroup VimOSC52
    autocmd!
    autocmd TermResponseAll osc ++once call feedkeys("\<F30>", '!')
  augroup END

  if has('win32')
      call echoraw("\<Esc>]52;;?\<Esc>\\")
  else
    if reg == "+" || !has('clipboard_plus_avail')
      call echoraw("\<Esc>]52;c;?\<Esc>\\")
    else
      call echoraw("\<Esc>]52;p;?\<Esc>\\")
    endif
  endif

  while getchar(-1) != "\<F30>"
  endwhile
  autocmd! VimOSC52

  var stuff = matchstr(v:termosc, '52;.\+;\zs[A-Za-z0-9+/=]\+')

  return ("", blob2str(base64_decode(stuff)))
enddef

def Copy(reg: string, type: string, lines: list<string>): void
  if has('win32')
      call echoraw("\<Esc>]52;;" .. base64_encode(str2blob(lines)) .. "\<Esc>\\")
  else
    if reg == "+" || !has('clipboard_plus_avail')
      call echoraw("\<Esc>]52;c;" .. base64_encode(str2blob(lines)) .. "\<Esc>\\")
    else
      call echoraw("\<Esc>]52;p;" .. base64_encode(str2blob(lines)) .. "\<Esc>\\")
    endif
  endif
enddef

v:clipproviders["osc52"] = {
  "available": function("Available"),
  "paste": {
    "*": function("Paste"),
    "+": function("Paste")
  },
  "copy": {
    "*": function("Copy"),
    "+": function("Copy")
  },
}

# vim: set sw=2 sts=2 :
