vim9script

export def Paste(reg: string, type: string): any
  if type == "implicit"
    return "previous"
  endif

  augroup VimOSC52
    autocmd!
    autocmd TermResponseAll osc ++once call feedkeys("\<F30>", '!')
  augroup END

  # Call without a letter (which indicates the selection type in X11), if there
  # is only one clipboard. Adding a letter also seems to break functionality on
  # Windows terminal.
  if has('clipboard_plus_avail')
    if reg == "+"
      call echoraw("\<Esc>]52;c;?\<Esc>\\")
    else
      call echoraw("\<Esc>]52;p;?\<Esc>\\")
    endif
  else
      call echoraw("\<Esc>]52;;?\<Esc>\\")
  endif

  # Wait for response from terminal
  while getchar(-1) != "\<F30>"
  endwhile
  autocmd! VimOSC52

  # Extract the base64 stuff
  var stuff = matchstr(v:termosc, '52;.\+;\zs[A-Za-z0-9+/=]\+')

  return ("", blob2str(base64_decode(stuff)))
enddef

export def Copy(reg: string, type: string, lines: list<string>): void
  if has('clipboard_plus_avail')
    if reg == "+"
      call echoraw("\<Esc>]52;c;" .. base64_encode(str2blob(lines)) .. "\<Esc>\\")
    else
      call echoraw("\<Esc>]52;p;" .. base64_encode(str2blob(lines)) .. "\<Esc>\\")
    endif
  else
      call echoraw("\<Esc>]52;;" .. base64_encode(str2blob(lines)) .. "\<Esc>\\")
  endif
enddef
