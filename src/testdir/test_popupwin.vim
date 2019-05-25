" Tests for popup windows

if !has('textprop')
  finish
endif

source screendump.vim

func Test_simple_popup()
  if !CanRunVimInTerminal()
    return
  endif
  call writefile([
	\ "call setline(1, range(1, 100))",
	\ "let winid = popup_create('hello there', {'line': 3, 'col': 11})",
	\ "hi PopupColor ctermbg=lightblue",
	\ "call setwinvar(winid, '&wincolor', 'PopupColor')",
	\ "let winid2 = popup_create(['another one', 'another two', 'another three'], {'line': 3, 'col': 25})",
	\], 'XtestPopup')
  let buf = RunVimInTerminal('-S XtestPopup', {'rows': 10})
  call VerifyScreenDump(buf, 'Test_popupwin_01', {})

  " clean up
  call StopVimInTerminal(buf)
  call delete('XtestPopup')
endfunc
