vim9script

export var allowed: bool = false

export def Available(): bool
  if get(g:, 'osc52_force_avail', 0)
    return true
  endif
  return allowed
enddef

var sent_message: bool = false

def OSCMessage(id: number)
  echom "Waiting for OSC52 response... Press CTRL-C to quit"
  sent_message = true
enddef

export def Paste(reg: string): tuple<string, list<string>>
  # Some terminals like Kitty respect the selection type parameter on both X11
  # and Wayland. If the terminal doesn't then the selection type parameter
  # should be ignored (no-op)
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
