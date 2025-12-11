vim9script

var sent_message: bool = false

def OSCMessage(id: number)
  echom "Waiting for OSC52 response... Press CTRL-C to quit"
  sent_message = true
enddef

def DA1Message(id: number)
  echom "Waiting for DA1 response... Press CTRL-C to quit"
  sent_message = true
enddef


export def Available(): bool
  if get(g:, 'osc52_force_avail', 0)
    return true
  endif

  g:vimosc52_gotda1 = false

  augroup VimOSC52DA1
    autocmd!
    autocmd TermResponseAll da1 ++once g:vimosc52_gotda1 = v:true
  augroup END

  # Send request and wait for DA1 response from terminal
  call echoraw("\<Esc>[c")

  var timerid: number = timer_start(1000, DA1Message)

  while true
    if getcharstr(-1, {cursor: "hide"}) == "\<xCSI>" && g:vimosc52_gotda1
      break
    endif
  endwhile

  timer_stop(timerid)
  if sent_message
    sent_message = false
    :redraw
  endif

  autocmd! VimOSC52DA1
  unlet g:vimosc52_gotda1

  # If there is a 52 parameter, then the terminal supports OSC 52
  if match(v:termda1, ';\zs52\ze') != -1
    return true
  endif
  return false
enddef

export def Paste(reg: string): any
  # Call without a letter (which indicates the selection type in X11), if there
  # is only one clipboard. Adding a letter also seems to break functionality on
  # Windows terminal.
  #
  # Some terminals like Kitty respect the selection type parameter on both X11
  # and Wayland, from Xterm docs:

  if reg == "+"
    echoraw("\<Esc>]52;c;?\<Esc>\\")
  else
    echoraw("\<Esc>]52;p;?\<Esc>\\")
  endif

  var timerid: number = timer_start(1000, OSCMessage)

  # Wait for response from terminal
  while true
    if getcharstr(-1, {cursor: "hide"}) == "\<xOSC>" && match(v:termosc, '52;') != -1
      break
    endif
  endwhile

  timer_stop(timerid)
  if sent_message
    sent_message = false
    :redraw
  endif

  # Extract the base64 stuff
  var stuff = matchstr(v:termosc, '52;.\+;\zs[A-Za-z0-9+/=]\+')

  return ("", blob2str(base64_decode(stuff)))
enddef

export def Copy(reg: string, type: string, lines: list<string>): void
  if reg == "+"
    echoraw("\<Esc>]52;c;" .. base64_encode(str2blob(lines)) .. "\<Esc>\\")
  else
    echoraw("\<Esc>]52;p;" .. base64_encode(str2blob(lines)) .. "\<Esc>\\")
  endif
enddef

# vim: set sw=2 sts=2 :
