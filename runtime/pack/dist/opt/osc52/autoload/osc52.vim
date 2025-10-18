vim9script

export def Available(): string
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

export def Paste(reg: string, type: string): any
  # Some terminals have a confrim prompt when accessing the clipboard, so we
  # only want to request the clipboard if the user explicitly does so.
  if type == "implicit" && !get(g:, 'osc52_always_access', 0)
      return "previous"
  endif

  augroup VimOSC52
    autocmd!
    autocmd TermResponseAll osc ++once call feedkeys("\<F30>", '!')
  augroup END

  # Call without a letter (which indicates the selection type in X11), if there
  # is only one clipboard. Adding a letter also seems to break functionality on
  # Windows terminal.
  #
  # Some terminals like Kitty respect the selection type parameter on both X11
  # and Wayland, from Xterm docs:
  #
  # ```
  # Ps = 5 2  ⇒  Manipulate Selection Data.  These controls may
  # be disabled using the allowWindowOps resource.  The parameter
  #   Pt is parsed as
  #        Pc ; Pd

  # The first, Pc, may contain zero or more characters from the
  # set c , p , q , s , 0 , 1 , 2 , 3 , 4 , 5 , 6 , and 7 .  It is
  # used to construct a list of selection parameters for
  # clipboard, primary, secondary, select, or cut-buffers 0
  # through 7 respectively, in the order given.  If the parameter
  # is empty, xterm uses s 0 , to specify the configurable
  # primary/clipboard selection and cut-buffer 0.

  # The second parameter, Pd, gives the selection data.  Normally
  # this is a string encoded in base64 (RFC-4648).  The data
  # becomes the new selection, which is then available for pasting
  # by other applications.

  # If the second parameter is a ? , xterm replies to the host
  # with the selection data encoded using the same protocol.  It
  # uses the first selection found by asking successively for each
  # item from the list of selection parameters.
  # ```
  if has('clipboard_plus_avail')
    if reg == "+"
      echoraw("\<Esc>]52;c;?\<Esc>\\")
    else
      echoraw("\<Esc>]52;p;?\<Esc>\\")
    endif
  else
      echoraw("\<Esc>]52;;?\<Esc>\\")
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
      echoraw("\<Esc>]52;c;" .. base64_encode(str2blob(lines)) .. "\<Esc>\\")
    else
      echoraw("\<Esc>]52;p;" .. base64_encode(str2blob(lines)) .. "\<Esc>\\")
    endif
  else
      echoraw("\<Esc>]52;;" .. base64_encode(str2blob(lines)) .. "\<Esc>\\")
  endif
enddef

# vim: set sw=2 sts=2 :
