" Tests for regexp in utf8 encoding

func s:equivalence_test()
  let str = "AÀÁÂÃÄÅĀĂĄǍǞǠẢ BḂḆ CÇĆĈĊČ DĎĐḊḎḐ EÈÉÊËĒĔĖĘĚẺẼ FḞ GĜĞĠĢǤǦǴḠ HĤĦḢḦḨ IÌÍÎÏĨĪĬĮİǏỈ JĴ KĶǨḰḴ LĹĻĽĿŁḺ MḾṀ NÑŃŅŇṄṈ OÒÓÔÕÖØŌŎŐƠǑǪǬỎ PṔṖ Q RŔŖŘṘṞ SŚŜŞŠṠ TŢŤŦṪṮ UÙÚÛÜŨŪŬŮŰŲƯǓỦ VṼ WŴẀẂẄẆ XẊẌ YÝŶŸẎỲỶỸ ZŹŻŽƵẐẔ aàáâãäåāăąǎǟǡả bḃḇ cçćĉċč dďđḋḏḑ eèéêëēĕėęěẻẽ fḟ gĝğġģǥǧǵḡ hĥħḣḧḩẖ iìíîïĩīĭįǐỉ jĵǰ kķǩḱḵ lĺļľŀłḻ mḿṁ nñńņňŉṅṉ oòóôõöøōŏőơǒǫǭỏ pṕṗ q rŕŗřṙṟ sśŝşšṡ tţťŧṫṯẗ uùúûüũūŭůűųưǔủ vṽ wŵẁẃẅẇẘ xẋẍ yýÿŷẏẙỳỷỹ zźżžƶẑẕ"
  let groups = split(str)
  for group1 in groups
      for c in split(group1, '\zs')
	" next statement confirms that equivalence class matches every
	" character in group
        call assert_match('^[[=' . c . '=]]*$', group1)
        for group2 in groups
          if group2 != group1
	    " next statement converts that equivalence class doesn't match
	    " character in any other group
            call assert_equal(-1, match(group2, '[[=' . c . '=]]'))
          endif
        endfor
      endfor
  endfor
endfunc

func Test_equivalence_re1()
  set re=1
  call s:equivalence_test()
  set re=0
endfunc

func Test_equivalence_re2()
  set re=2
  call s:equivalence_test()
  set re=0
endfunc

func s:classes_test()
  set isprint=@,161-255
  call assert_equal('Motörhead', matchstr('Motörhead', '[[:print:]]\+'))

  let alnumchars = ''
  let alphachars = ''
  let backspacechar = ''
  let blankchars = ''
  let cntrlchars = ''
  let digitchars = ''
  let escapechar = ''
  let graphchars = ''
  let lowerchars = ''
  let printchars = ''
  let punctchars = ''
  let returnchar = ''
  let spacechars = ''
  let tabchar = ''
  let upperchars = ''
  let xdigitchars = ''
  let identchars = ''
  let identchars1 = ''
  let kwordchars = ''
  let kwordchars1 = ''
  let fnamechars = ''
  let fnamechars1 = ''
  let i = 1
  while i <= 255
    let c = nr2char(i)
    if c =~ '[[:alpha:]]'
      let alphachars .= c
    endif
    if c =~ '[[:alnum:]]'
      let alnumchars .= c
    endif
    if c =~ '[[:backspace:]]'
      let backspacechar .= c
    endif
    if c =~ '[[:blank:]]'
      let blankchars .= c
    endif
    if c =~ '[[:cntrl:]]'
      let cntrlchars .= c
    endif
    if c =~ '[[:digit:]]'
      let digitchars .= c
    endif
    if c =~ '[[:escape:]]'
      let escapechar .= c
    endif
    if c =~ '[[:graph:]]'
      let graphchars .= c
    endif
    if c =~ '[[:lower:]]'
      let lowerchars .= c
    endif
    if c =~ '[[:print:]]'
      let printchars .= c
    endif
    if c =~ '[[:punct:]]'
      let punctchars .= c
    endif
    if c =~ '[[:return:]]'
      let returnchar .= c
    endif
    if c =~ '[[:space:]]'
      let spacechars .= c
    endif
    if c =~ '[[:tab:]]'
      let tabchar .= c
    endif
    if c =~ '[[:upper:]]'
      let upperchars .= c
    endif
    if c =~ '[[:xdigit:]]'
      let xdigitchars .= c
    endif
    if c =~ '[[:ident:]]'
      let identchars .= c
    endif
    if c =~ '\i'
      let identchars1 .= c
    endif
    if c =~ '[[:keyword:]]'
      let kwordchars .= c
    endif
    if c =~ '\k'
      let kwordchars1 .= c
    endif
    if c =~ '[[:fname:]]'
      let fnamechars .= c
    endif
    if c =~ '\f'
      let fnamechars1 .= c
    endif
    let i += 1
  endwhile

  call assert_equal('ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz', alphachars)
  call assert_equal('0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz', alnumchars)
  call assert_equal("\b", backspacechar)
  call assert_equal("\t ", blankchars)
  call assert_equal("\x01\x02\x03\x04\x05\x06\x07\b\t\n\x0b\f\r\x0e\x0f\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1a\e\x1c\x1d\x1e\x1f\x7f", cntrlchars)
  call assert_equal("0123456789", digitchars)
  call assert_equal("\<Esc>", escapechar)
  call assert_equal('!"#$%&''()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\]^_`abcdefghijklmnopqrstuvwxyz{|}~', graphchars)
  call assert_equal('abcdefghijklmnopqrstuvwxyzµßàáâãäåæçèéêëìíîïðñòóôõöøùúûüýþÿ', lowerchars)
  call assert_equal(' !"#$%&''()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\]^_`abcdefghijklmnopqrstuvwxyz{|}~ ¡¢£¤¥¦§¨©ª«¬­®¯°±²³´µ¶·¸¹º»¼½¾¿ÀÁÂÃÄÅÆÇÈÉÊËÌÍÎÏÐÑÒÓÔÕÖ×ØÙÚÛÜÝÞßàáâãäåæçèéêëìíîïðñòóôõö÷øùúûüýþÿ', printchars)
  call assert_equal('!"#$%&''()*+,-./:;<=>?@[\]^_`{|}~', punctchars)
  call assert_equal('ABCDEFGHIJKLMNOPQRSTUVWXYZÀÁÂÃÄÅÆÇÈÉÊËÌÍÎÏÐÑÒÓÔÕÖØÙÚÛÜÝÞ', upperchars)
  call assert_equal("\r", returnchar)
  call assert_equal("\t\n\x0b\f\r ", spacechars)
  call assert_equal("\t", tabchar)
  call assert_equal('0123456789ABCDEFabcdef', xdigitchars)

  if has('win32')
    let identchars_ok = '0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ_abcdefghijklmnopqrstuvwxyz ¡¢£¤¥¦§µÀÁÂÃÄÅÆÇÈÉÊËÌÍÎÏÐÑÒÓÔÕÖØÙÚÛÜÝÞßàáâãäåæçèéêëìíîïðñòóôõöøùúûüýþÿ'
    let kwordchars_ok = '0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ_abcdefghijklmnopqrstuvwxyzµÀÁÂÃÄÅÆÇÈÉÊËÌÍÎÏÐÑÒÓÔÕÖ×ØÙÚÛÜÝÞßàáâãäåæçèéêëìíîïðñòóôõö÷øùúûüýþÿ'
  elseif has('ebcdic')
    let identchars_ok = 'ABCDEFGHIJKLMNOPQRSTUVWXYZ_abcdefghijklmnopqrstuvwxyz¬®µº¿ÀÁÂÃÄÅÆÇÈÉÊËÌÍÎÏÐÑÒÓÔÕÖØÙÚÛÜÝÞßàáâãäåæçèéêëìíîïðñòóôõö÷øùúûüýþÿ'
    let kwordchars_ok = 'ABCDEFGHIJKLMNOPQRSTUVWXYZ_abcdefghijklmnopqrstuvwxyz¬®µº¿ÀÁÂÃÄÅÆÇÈÉÊËÌÍÎÏÐÑÒÓÔÕÖØÙÚÛÜÝÞßàáâãäåæçèéêëìíîïðñòóôõö÷øùúûüýþÿ'
  else
    let identchars_ok = '0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ_abcdefghijklmnopqrstuvwxyzµÀÁÂÃÄÅÆÇÈÉÊËÌÍÎÏÐÑÒÓÔÕÖ×ØÙÚÛÜÝÞßàáâãäåæçèéêëìíîïðñòóôõö÷øùúûüýþÿ'
    let kwordchars_ok = '0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ_abcdefghijklmnopqrstuvwxyzµÀÁÂÃÄÅÆÇÈÉÊËÌÍÎÏÐÑÒÓÔÕÖ×ØÙÚÛÜÝÞßàáâãäåæçèéêëìíîïðñòóôõö÷øùúûüýþÿ'
  endif

  if has('win32')
    let fnamechars_ok = '!#$%+,-./0123456789:=@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\]_abcdefghijklmnopqrstuvwxyz{}~ ¡¢£¤¥¦§¨©ª«¬­®¯°±²³´µ¶·¸¹º»¼½¾¿ÀÁÂÃÄÅÆÇÈÉÊËÌÍÎÏÐÑÒÓÔÕÖ×ØÙÚÛÜÝÞßàáâãäåæçèéêëìíîïðñòóôõö÷øùúûüýþÿ'
  elseif has('amiga')
    let fnamechars_ok = '$+,-./0123456789:ABCDEFGHIJKLMNOPQRSTUVWXYZ_abcdefghijklmnopqrstuvwxyz~ ¡¢£¤¥¦§¨©ª«¬­®¯°±²³´µ¶·¸¹º»¼½¾¿ÀÁÂÃÄÅÆÇÈÉÊËÌÍÎÏÐÑÒÓÔÕÖ×ØÙÚÛÜÝÞßàáâãäåæçèéêëìíîïðñòóôõö÷øùúûüýþÿ'
  elseif has('vms')
    let fnamechars_ok = '#$%+,-./0123456789:;<>ABCDEFGHIJKLMNOPQRSTUVWXYZ[]_abcdefghijklmnopqrstuvwxyz~ ¡¢£¤¥¦§¨©ª«¬­®¯°±²³´µ¶·¸¹º»¼½¾¿ÀÁÂÃÄÅÆÇÈÉÊËÌÍÎÏÐÑÒÓÔÕÖ×ØÙÚÛÜÝÞßàáâãäåæçèéêëìíîïðñòóôõö÷øùúûüýþÿ'
  elseif has('ebcdic')
    let fnamechars_ok = '#$%+,-./=ABCDEFGHIJKLMNOPQRSTUVWXYZ_abcdefghijklmnopqrstuvwxyz~ ¡¢£¤¥¦§¨©ª«¬­®¯°±²³´µ¶·¸¹º»¼½¾¿ÀÁÂÃÄÅÆÇÈÉÊËÌÍÎÏÐÑÒÓÔÕÖ×ØÙÚÛÜÝÞßàáâãäåæçèéêëìíîïðñòóôõö÷øùúûüýþÿ'
  else
    let fnamechars_ok = '#$%+,-./0123456789=ABCDEFGHIJKLMNOPQRSTUVWXYZ_abcdefghijklmnopqrstuvwxyz~ ¡¢£¤¥¦§¨©ª«¬­®¯°±²³´µ¶·¸¹º»¼½¾¿ÀÁÂÃÄÅÆÇÈÉÊËÌÍÎÏÐÑÒÓÔÕÖ×ØÙÚÛÜÝÞßàáâãäåæçèéêëìíîïðñòóôõö÷øùúûüýþÿ'
  endif

  call assert_equal(identchars_ok, identchars)
  call assert_equal(kwordchars_ok, kwordchars)
  call assert_equal(fnamechars_ok, fnamechars)

  call assert_equal(identchars1, identchars)
  call assert_equal(kwordchars1, kwordchars)
  call assert_equal(fnamechars1, fnamechars)
endfunc

func Test_classes_re1()
  set re=1
  call s:classes_test()
  set re=0
endfunc

func Test_classes_re2()
  set re=2
  call s:classes_test()
  set re=0
endfunc

func Test_reversed_range()
  for re in range(0, 2)
    exe 'set re=' . re
    call assert_fails('call match("abc def", "[c-a]")', 'E944:')
  endfor
  set re=0
endfunc

func Test_large_class()
  set re=1
  call assert_fails('call match("abc def", "[\u3000-\u4000]")', 'E945:')
  set re=2
  call assert_equal(0, 'abc def' =~# '[\u3000-\u4000]')
  call assert_equal(1, "\u3042" =~# '[\u3000-\u4000]')
  set re=0
endfunc

func Test_optmatch_toolong()
  set re=1
  " Can only handle about 8000 characters.
  let pat = '\\%[' .. repeat('x', 9000) .. ']'
  call assert_fails('call match("abc def", "' .. pat .. '")', 'E339:')
  set re=0
endfunc

" Test for regexp patterns with multi-byte support, using utf-8.
func Test_multibyte_chars()
  " tl is a List of Lists with:
  "    2: test auto/old/new  0: test auto/old  1: test auto/new
  "    regexp pattern
  "    text to test the pattern on
  "    expected match (optional)
  "    expected submatch 1 (optional)
  "    expected submatch 2 (optional)
  "    etc.
  "  When there is no match use only the first two items.
  let tl = []

  " Multi-byte character tests. These will fail unless vim is compiled
  " with Multibyte (FEAT_MBYTE) or BIG/HUGE features.
  call add(tl, [2, '[[:alpha:][=a=]]\+', '879 aiaãâaiuvna ', 'aiaãâaiuvna'])
  call add(tl, [2, '[[=a=]]\+', 'ddaãâbcd', 'aãâ'])								" equivalence classes
  call add(tl, [2, '[^ม ]\+', 'มม oijasoifjos ifjoisj f osij j มมมมม abcd', 'oijasoifjos'])
  call add(tl, [2, ' [^ ]\+', 'start มabcdม ', ' มabcdม'])
  call add(tl, [2, '[ม[:alpha:][=a=]]\+', '879 aiaãมâมaiuvna ', 'aiaãมâมaiuvna'])

  " this is not a normal "i" but 0xec
  call add(tl, [2, '\p\+', 'ìa', 'ìa'])
  call add(tl, [2, '\p*', 'aあ', 'aあ'])

  " Test recognition of some character classes
  call add(tl, [2, '\i\+', '&*¨xx ', 'xx'])
  call add(tl, [2, '\f\+', '&*fname ', 'fname'])

  " Test composing character matching
  call add(tl, [2, '.ม', 'xม่x yมy', 'yม'])
  call add(tl, [2, '.ม่', 'xม่x yมy', 'xม่'])
  call add(tl, [2, "\u05b9", " x\u05b9 ", "x\u05b9"])
  call add(tl, [2, ".\u05b9", " x\u05b9 ", "x\u05b9"])
  call add(tl, [2, "\u05b9\u05bb", " x\u05b9\u05bb ", "x\u05b9\u05bb"])
  call add(tl, [2, ".\u05b9\u05bb", " x\u05b9\u05bb ", "x\u05b9\u05bb"])
  call add(tl, [2, "\u05bb\u05b9", " x\u05b9\u05bb ", "x\u05b9\u05bb"])
  call add(tl, [2, ".\u05bb\u05b9", " x\u05b9\u05bb ", "x\u05b9\u05bb"])
  call add(tl, [2, "\u05b9", " y\u05bb x\u05b9 ", "x\u05b9"])
  call add(tl, [2, ".\u05b9", " y\u05bb x\u05b9 ", "x\u05b9"])
  call add(tl, [2, "\u05b9", " y\u05bb\u05b9 x\u05b9 ", "y\u05bb\u05b9"])
  call add(tl, [2, ".\u05b9", " y\u05bb\u05b9 x\u05b9 ", "y\u05bb\u05b9"])
  call add(tl, [1, "\u05b9\u05bb", " y\u05b9 x\u05b9\u05bb ", "x\u05b9\u05bb"])
  call add(tl, [2, ".\u05b9\u05bb", " y\u05bb x\u05b9\u05bb ", "x\u05b9\u05bb"])
  call add(tl, [2, "a", "ca\u0300t"])
  call add(tl, [2, "ca", "ca\u0300t"])
  call add(tl, [2, "a\u0300", "ca\u0300t", "a\u0300"])
  call add(tl, [2, 'a\%C', "ca\u0300t", "a\u0300"])
  call add(tl, [2, 'ca\%C', "ca\u0300t", "ca\u0300"])
  call add(tl, [2, 'ca\%Ct', "ca\u0300t", "ca\u0300t"])

  " Test \Z
  call add(tl, [2, 'ú\Z', 'x'])
  call add(tl, [2, 'יהוה\Z', 'יהוה', 'יהוה'])
  call add(tl, [2, 'יְהוָה\Z', 'יהוה', 'יהוה'])
  call add(tl, [2, 'יהוה\Z', 'יְהוָה', 'יְהוָה'])
  call add(tl, [2, 'יְהוָה\Z', 'יְהוָה', 'יְהוָה'])
  call add(tl, [2, 'יְ\Z', 'וְיַ', 'יַ'])
  call add(tl, [2, "ק\u200d\u05b9x\\Z", "xק\u200d\u05b9xy", "ק\u200d\u05b9x"])
  call add(tl, [2, "ק\u200d\u05b9x\\Z", "xק\u200dxy", "ק\u200dx"])
  call add(tl, [2, "ק\u200dx\\Z", "xק\u200d\u05b9xy", "ק\u200d\u05b9x"])
  call add(tl, [2, "ק\u200dx\\Z", "xק\u200dxy", "ק\u200dx"])
  call add(tl, [2, "\u05b9\\Z", "xyz"])
  call add(tl, [2, "\\Z\u05b9", "xyz"])
  call add(tl, [2, "\u05b9\\Z", "xy\u05b9z", "y\u05b9"])
  call add(tl, [2, "\\Z\u05b9", "xy\u05b9z", "y\u05b9"])
  call add(tl, [1, "\u05b9\\+\\Z", "xy\u05b9z\u05b9 ", "y\u05b9z\u05b9"])
  call add(tl, [1, "\\Z\u05b9\\+", "xy\u05b9z\u05b9 ", "y\u05b9z\u05b9"])

  " Combining different tests and features
  call add(tl, [2, '[^[=a=]]\+', 'ddaãâbcd', 'dd'])

  " Run the tests
  for t in tl
    let re = t[0]
    let pat = t[1]
    let text = t[2]
    let matchidx = 3
    for engine in [0, 1, 2]
      if engine == 2 && re == 0 || engine == 1 && re == 1
        continue
      endif
      let &regexpengine = engine
      try
        let l = matchlist(text, pat)
      catch
        call assert_report('Error ' . engine . ': pat: \"' . pat .
		    \ '\", text: \"' . text .
		    \ '\", caused an exception: \"' . v:exception . '\"')
      endtry
      " check the match itself
      if len(l) == 0 && len(t) > matchidx
        call assert_report('Error ' . engine . ': pat: \"' . pat .
		    \ '\", text: \"' . text .
		    \ '\", did not match, expected: \"' . t[matchidx] . '\"')
      elseif len(l) > 0 && len(t) == matchidx
        call assert_report('Error ' . engine . ': pat: \"' . pat .
		    \ '\", text: \"' . text . '\", match: \"' . l[0] .
		    \ '\", expected no match')
      elseif len(t) > matchidx && l[0] != t[matchidx]
        call assert_report('Error ' . engine . ': pat: \"' . pat .
		    \ '\", text: \"' . text . '\", match: \"' . l[0] .
		    \ '\", expected: \"' . t[matchidx] . '\"')
      else
        " Test passed
      endif
      if len(l) > 0
        " check all the nine submatches
        for i in range(1, 9)
          if len(t) <= matchidx + i
            let e = ''
          else
            let e = t[matchidx + i]
          endif
          if l[i] != e
            call assert_report('Error ' . engine . ': pat: \"' . pat .
                  \ '\", text: \"' . text . '\", submatch ' . i .
                  \ ': \"' . l[i] . '\", expected: \"' . e . '\"')
          endif
        endfor
        unlet i
      endif
    endfor
  endfor
  set regexpengine&
endfunc

" check that 'ambiwidth' does not change the meaning of \p
func Test_ambiwidth()
  set regexpengine=1 ambiwidth=single
  call assert_equal(0, match("\u00EC", '\p'))
  set regexpengine=1 ambiwidth=double
  call assert_equal(0, match("\u00EC", '\p'))
  set regexpengine=2 ambiwidth=single
  call assert_equal(0, match("\u00EC", '\p'))
  set regexpengine=2 ambiwidth=double
  call assert_equal(0, match("\u00EC", '\p'))
  set regexpengine& ambiwidth&
endfunc

" vim: shiftwidth=2 sts=2 expandtab
