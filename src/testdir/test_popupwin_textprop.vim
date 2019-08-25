" Tests for popup windows for text properties

source check.vim
CheckFeature textprop

source screendump.vim
CheckScreendump

func Test_textprop_popup()
  let lines =<< trim END
	call setline(1, range(1, 100))
	call setline(50, 'some text to work with')
	50
	normal zz
	set scrolloff=0
	call prop_type_add('popupMarker', #{highlight: 'DiffAdd'})
	call prop_add(50, 11, #{
		\ length: 7,
		\ type: 'popupMarker',
		\ })
	let winid = popup_create('the text', #{
	      \ pos: 'botleft', 
	      \ textprop: 'popupMarker',
	      \ border: [],
	      \ padding: [0,1,0,1],
	      \ close: 'click',
	      \ })
  END
  call writefile(lines, 'XtestTextpropPopup')
  let buf = RunVimInTerminal('-S XtestTextpropPopup', #{rows: 10})
  call VerifyScreenDump(buf, 'Test_popup_textprop_01', {})

  call term_sendkeys(buf, "zt")
  call VerifyScreenDump(buf, 'Test_popup_textprop_02', {})

  call term_sendkeys(buf, "zzIawe\<Esc>")
  call VerifyScreenDump(buf, 'Test_popup_textprop_03', {})

  call term_sendkeys(buf, "0dw")
  call VerifyScreenDump(buf, 'Test_popup_textprop_04', {})

  call term_sendkeys(buf, "Oinserted\<Esc>")
  call VerifyScreenDump(buf, 'Test_popup_textprop_05', {})

  call term_sendkeys(buf, "k2dd")
  call VerifyScreenDump(buf, 'Test_popup_textprop_06', {})

  " clean up
  call StopVimInTerminal(buf)
  call delete('XtestTextpropPopup')
endfunc


" vim: shiftwidth=2 sts=2
