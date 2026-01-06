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
  echo "Waiting for OSC52 response... Press CTRL-C to cancel"
  sent_message = true
enddef

export def Paste(reg: string): tuple<string, list<string>>
  # Check if user has indicated that the terminal does not support OSC 52 paste
  # (or has disabled it)
  if get(g:, 'osc52_disable_paste', 0)
    return ("c", [])
  endif

  # Some terminals like Kitty respect the selection type parameter on both X11
  # and Wayland. If the terminal doesn't then the selection type parameter
  # should be ignored (no-op)
  if reg == "+"
    echoraw("\<Esc>]52;c;?\<Esc>\\")
  else
    echoraw("\<Esc>]52;p;?\<Esc>\\")
  endif

  var timerid: number = timer_start(1000, OSCMessage)
  var interrupt: bool = false

  # Wait for response from terminal. If we got interrupted (Ctrl-C), then do a
  # redraw if we already sent the message, and return an empty string.
  try
    while true
      var key: string =  getcharstr(-1, {cursor: "hide"})

      if key == "\<xOSC>" && match(v:termosc, '52;') != -1
        break
      elseif key == "\<C-c>"
        interrupt = true
        break
      endif
    endwhile

  # This doesn't seem to catch Ctrl-C sent via term_sendkeys(), which is used in
  # tests. So also check the result of getcharstr()/getchar().
  catch /^Vim:Interrupt$/
    interrupt = true
  finally
    timer_stop(timerid)
    if sent_message
      sent_message = false
      :redraw!
    endif
  endtry

  if interrupt
    echo "Interrupted while waiting for OSC 52 response"
    return ("c", [""])
  endif

  # Extract the base64 stuff
  var stuff: string = matchstr(v:termosc, '52;.*;\zs[A-Za-z0-9+/=]*')

  if len(stuff) == 0
    return ("c", [])
  endif

  var ret: list<string>

  # "stuff" may be an invalid base64 string, so catch any errors
  try
    ret = blob2str(base64_decode(stuff))
  catch /E\(475\|1515\)/
    echo "Invalid OSC 52 response received"
    return ("c", [""])
  endtry

  return ("", ret)
enddef

export def Copy(reg: string, type: string, lines: list<string>): void
  if reg == "+"
    echoraw("\<Esc>]52;c;" .. base64_encode(str2blob(lines)) .. "\<Esc>\\")
  else
    echoraw("\<Esc>]52;p;" .. base64_encode(str2blob(lines)) .. "\<Esc>\\")
  endif
enddef

# vim: set sw=2 sts=2 :
