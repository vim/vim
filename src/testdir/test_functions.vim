" Tests for various functions.

func Test_empty()
  call assert_equal(1, empty(''))
  call assert_equal(0, empty('a'))

  call assert_equal(1, empty(0))
  call assert_equal(1, empty(-0))
  call assert_equal(0, empty(1))
  call assert_equal(0, empty(-1))

  call assert_equal(1, empty(0.0))
  call assert_equal(1, empty(-0.0))
  call assert_equal(0, empty(1.0))
  call assert_equal(0, empty(-1.0))
  call assert_equal(0, empty(1.0/0.0))
  call assert_equal(0, empty(0.0/0.0))

  call assert_equal(1, empty([]))
  call assert_equal(0, empty(['a']))

  call assert_equal(1, empty({}))
  call assert_equal(0, empty({'a':1}))

  call assert_equal(1, empty(v:null))
  call assert_equal(1, empty(v:none))
  call assert_equal(1, empty(v:false))
  call assert_equal(0, empty(v:true))

  call assert_equal(0, empty(function('Test_empty')))
endfunc

func Test_len()
  call assert_equal(1, len(0))
  call assert_equal(2, len(12))

  call assert_equal(0, len(''))
  call assert_equal(2, len('ab'))

  call assert_equal(0, len([]))
  call assert_equal(2, len([2, 1]))

  call assert_equal(0, len({}))
  call assert_equal(2, len({'a': 1, 'b': 2}))

  call assert_fails('call len(v:none)', 'E701:')
  call assert_fails('call len({-> 0})', 'E701:')
endfunc

func Test_max()
  call assert_equal(0, max([]))
  call assert_equal(2, max([2]))
  call assert_equal(2, max([1, 2]))
  call assert_equal(2, max([1, 2, v:null]))

  call assert_equal(0, max({}))
  call assert_equal(2, max({'a':1, 'b':2}))

  call assert_fails('call max(1)', 'E712:')
  call assert_fails('call max(v:none)', 'E712:')
endfunc

func Test_min()
  call assert_equal(0, min([]))
  call assert_equal(2, min([2]))
  call assert_equal(1, min([1, 2]))
  call assert_equal(0, min([1, 2, v:null]))

  call assert_equal(0, min({}))
  call assert_equal(1, min({'a':1, 'b':2}))

  call assert_fails('call min(1)', 'E712:')
  call assert_fails('call min(v:none)', 'E712:')
endfunc

func Test_str2nr()
  call assert_equal(0, str2nr(''))
  call assert_equal(1, str2nr('1'))
  call assert_equal(1, str2nr(' 1 '))

  call assert_equal(1, str2nr('+1'))
  call assert_equal(1, str2nr('+ 1'))
  call assert_equal(1, str2nr(' + 1 '))

  call assert_equal(-1, str2nr('-1'))
  call assert_equal(-1, str2nr('- 1'))
  call assert_equal(-1, str2nr(' - 1 '))

  call assert_equal(123456789, str2nr('123456789'))
  call assert_equal(-123456789, str2nr('-123456789'))

  call assert_equal(5, str2nr('101', 2))
  call assert_equal(5, str2nr('0b101', 2))
  call assert_equal(5, str2nr('0B101', 2))
  call assert_equal(-5, str2nr('-101', 2))
  call assert_equal(-5, str2nr('-0b101', 2))
  call assert_equal(-5, str2nr('-0B101', 2))

  call assert_equal(65, str2nr('101', 8))
  call assert_equal(65, str2nr('0101', 8))
  call assert_equal(-65, str2nr('-101', 8))
  call assert_equal(-65, str2nr('-0101', 8))

  call assert_equal(11259375, str2nr('abcdef', 16))
  call assert_equal(11259375, str2nr('ABCDEF', 16))
  call assert_equal(-11259375, str2nr('-ABCDEF', 16))
  call assert_equal(11259375, str2nr('0xabcdef', 16))
  call assert_equal(11259375, str2nr('0Xabcdef', 16))
  call assert_equal(11259375, str2nr('0XABCDEF', 16))
  call assert_equal(-11259375, str2nr('-0xABCDEF', 16))

  call assert_equal(0, str2nr('0x10'))
  call assert_equal(0, str2nr('0b10'))
  call assert_equal(1, str2nr('12', 2))
  call assert_equal(1, str2nr('18', 8))
  call assert_equal(1, str2nr('1g', 16))

  call assert_equal(0, str2nr(v:null))
  call assert_equal(0, str2nr(v:none))

  call assert_fails('call str2nr([])', 'E730:')
  call assert_fails('call str2nr({->2})', 'E729:')
  call assert_fails('call str2nr(1.2)', 'E806:')
  call assert_fails('call str2nr(10, [])', 'E474:')
endfunc

func Test_strftime()
  if !exists('*strftime')
    return
  endif
  " Format of strftime() depends on system. We assume
  " that basic formats tested here are available and
  " identical on all systems which support strftime().
  call assert_equal('2017-01-18 00:25:12', strftime('%Y-%m-%d %H:%M:%S', 1484695512))
  call assert_equal('\d\d\d\d-\(0\d\|1[012]\)-\([012]\d\|3[01]\) \([01]\d\|2[0-3]\):[0-5]\d:\([0-5]\d\|60\)', strftime('%Y-%m-%d %H:%M:%S'))

  call assert_fails('call strftime([])', 'E730:')
  call assert_fails('call strftime("%Y", [])', 'E745:')
endfunc

func Test_simplify()
  call assert_equal('',            simplify(''))
  call assert_equal('/',           simplify('/'))
  call assert_equal('/',           simplify('/.'))
  call assert_equal('/',           simplify('/..'))
  call assert_equal('/...',        simplify('/...'))
  call assert_equal('./dir/file',  simplify('./dir/file'))
  call assert_equal('./dir/file',  simplify('.///dir//file'))
  call assert_equal('./dir/file',  simplify('./dir/./file'))
  call assert_equal('./file',      simplify('./dir/../file'))
  call assert_equal('../dir/file', simplify('dir/../../dir/file'))
  call assert_equal('./file',      simplify('dir/.././file'))

  call assert_fails('call simplify({->0})', 'E729:')
  call assert_fails('call simplify([])', 'E730:')
  call assert_fails('call simplify({})', 'E731:')
  call assert_fails('call simplify(1.2)', 'E806:')
endfunc

func Test_tolower()
  call assert_equal("", tolower(""))

  " Test with all printable ASCII characters.
  call assert_equal(' !"#$%&''()*+,-./0123456789:;<=>?@abcdefghijklmnopqrstuvwxyz[\]^_`abcdefghijklmnopqrstuvwxyz{|}~',
          \ tolower(' !"#$%&''()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\]^_`abcdefghijklmnopqrstuvwxyz{|}~'))

  if !has('multi_byte')
    return
  endif

  " Test with a few uppercase diacritics.
  call assert_equal("aàáâãäåāăąǎǟǡả", tolower("AÀÁÂÃÄÅĀĂĄǍǞǠẢ"))
  call assert_equal("bḃḇ", tolower("BḂḆ"))
  call assert_equal("cçćĉċč", tolower("CÇĆĈĊČ"))
  call assert_equal("dďđḋḏḑ", tolower("DĎĐḊḎḐ"))
  call assert_equal("eèéêëēĕėęěẻẽ", tolower("EÈÉÊËĒĔĖĘĚẺẼ"))
  call assert_equal("fḟ ", tolower("FḞ "))
  call assert_equal("gĝğġģǥǧǵḡ", tolower("GĜĞĠĢǤǦǴḠ"))
  call assert_equal("hĥħḣḧḩ", tolower("HĤĦḢḦḨ"))
  call assert_equal("iìíîïĩīĭįiǐỉ", tolower("IÌÍÎÏĨĪĬĮİǏỈ"))
  call assert_equal("jĵ", tolower("JĴ"))
  call assert_equal("kķǩḱḵ", tolower("KĶǨḰḴ"))
  call assert_equal("lĺļľŀłḻ", tolower("LĹĻĽĿŁḺ"))
  call assert_equal("mḿṁ", tolower("MḾṀ"))
  call assert_equal("nñńņňṅṉ", tolower("NÑŃŅŇṄṈ"))
  call assert_equal("oòóôõöøōŏőơǒǫǭỏ", tolower("OÒÓÔÕÖØŌŎŐƠǑǪǬỎ"))
  call assert_equal("pṕṗ", tolower("PṔṖ"))
  call assert_equal("q", tolower("Q"))
  call assert_equal("rŕŗřṙṟ", tolower("RŔŖŘṘṞ"))
  call assert_equal("sśŝşšṡ", tolower("SŚŜŞŠṠ"))
  call assert_equal("tţťŧṫṯ", tolower("TŢŤŦṪṮ"))
  call assert_equal("uùúûüũūŭůűųưǔủ", tolower("UÙÚÛÜŨŪŬŮŰŲƯǓỦ"))
  call assert_equal("vṽ", tolower("VṼ"))
  call assert_equal("wŵẁẃẅẇ", tolower("WŴẀẂẄẆ"))
  call assert_equal("xẋẍ", tolower("XẊẌ"))
  call assert_equal("yýŷÿẏỳỷỹ", tolower("YÝŶŸẎỲỶỸ"))
  call assert_equal("zźżžƶẑẕ", tolower("ZŹŻŽƵẐẔ"))

  " Test with a few lowercase diacritics, which should remain unchanged.
  call assert_equal("aàáâãäåāăąǎǟǡả", tolower("aàáâãäåāăąǎǟǡả"))
  call assert_equal("bḃḇ", tolower("bḃḇ"))
  call assert_equal("cçćĉċč", tolower("cçćĉċč"))
  call assert_equal("dďđḋḏḑ", tolower("dďđḋḏḑ"))
  call assert_equal("eèéêëēĕėęěẻẽ", tolower("eèéêëēĕėęěẻẽ"))
  call assert_equal("fḟ", tolower("fḟ"))
  call assert_equal("gĝğġģǥǧǵḡ", tolower("gĝğġģǥǧǵḡ"))
  call assert_equal("hĥħḣḧḩẖ", tolower("hĥħḣḧḩẖ"))
  call assert_equal("iìíîïĩīĭįǐỉ", tolower("iìíîïĩīĭįǐỉ"))
  call assert_equal("jĵǰ", tolower("jĵǰ"))
  call assert_equal("kķǩḱḵ", tolower("kķǩḱḵ"))
  call assert_equal("lĺļľŀłḻ", tolower("lĺļľŀłḻ"))
  call assert_equal("mḿṁ ", tolower("mḿṁ "))
  call assert_equal("nñńņňŉṅṉ", tolower("nñńņňŉṅṉ"))
  call assert_equal("oòóôõöøōŏőơǒǫǭỏ", tolower("oòóôõöøōŏőơǒǫǭỏ"))
  call assert_equal("pṕṗ", tolower("pṕṗ"))
  call assert_equal("q", tolower("q"))
  call assert_equal("rŕŗřṙṟ", tolower("rŕŗřṙṟ"))
  call assert_equal("sśŝşšṡ", tolower("sśŝşšṡ"))
  call assert_equal("tţťŧṫṯẗ", tolower("tţťŧṫṯẗ"))
  call assert_equal("uùúûüũūŭůűųưǔủ", tolower("uùúûüũūŭůűųưǔủ"))
  call assert_equal("vṽ", tolower("vṽ"))
  call assert_equal("wŵẁẃẅẇẘ", tolower("wŵẁẃẅẇẘ"))
  call assert_equal("ẋẍ", tolower("ẋẍ"))
  call assert_equal("yýÿŷẏẙỳỷỹ", tolower("yýÿŷẏẙỳỷỹ"))
  call assert_equal("zźżžƶẑẕ", tolower("zźżžƶẑẕ"))

  " According to https://twitter.com/jifa/status/625776454479970304
  " Ⱥ (U+023A) and Ⱦ (U+023E) are the *only* code points to increase
  " in length (2 to 3 bytes) when lowercased. So let's test them.
  call assert_equal("ⱥ ⱦ", tolower("Ⱥ Ⱦ"))
endfunc

func Test_toupper()
  call assert_equal("", toupper(""))

  " Test with all printable ASCII characters.
  call assert_equal(' !"#$%&''()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\]^_`ABCDEFGHIJKLMNOPQRSTUVWXYZ{|}~',
          \ toupper(' !"#$%&''()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\]^_`abcdefghijklmnopqrstuvwxyz{|}~'))

  if !has('multi_byte')
    return
  endif

  " Test with a few lowercase diacritics.
  call assert_equal("AÀÁÂÃÄÅĀĂĄǍǞǠẢ", toupper("aàáâãäåāăąǎǟǡả"))
  call assert_equal("BḂḆ", toupper("bḃḇ"))
  call assert_equal("CÇĆĈĊČ", toupper("cçćĉċč"))
  call assert_equal("DĎĐḊḎḐ", toupper("dďđḋḏḑ"))
  call assert_equal("EÈÉÊËĒĔĖĘĚẺẼ", toupper("eèéêëēĕėęěẻẽ"))
  call assert_equal("FḞ", toupper("fḟ"))
  call assert_equal("GĜĞĠĢǤǦǴḠ", toupper("gĝğġģǥǧǵḡ"))
  call assert_equal("HĤĦḢḦḨẖ", toupper("hĥħḣḧḩẖ"))
  call assert_equal("IÌÍÎÏĨĪĬĮǏỈ", toupper("iìíîïĩīĭįǐỉ"))
  call assert_equal("JĴǰ", toupper("jĵǰ"))
  call assert_equal("KĶǨḰḴ", toupper("kķǩḱḵ"))
  call assert_equal("LĹĻĽĿŁḺ", toupper("lĺļľŀłḻ"))
  call assert_equal("MḾṀ ", toupper("mḿṁ "))
  call assert_equal("NÑŃŅŇŉṄṈ", toupper("nñńņňŉṅṉ"))
  call assert_equal("OÒÓÔÕÖØŌŎŐƠǑǪǬỎ", toupper("oòóôõöøōŏőơǒǫǭỏ"))
  call assert_equal("PṔṖ", toupper("pṕṗ"))
  call assert_equal("Q", toupper("q"))
  call assert_equal("RŔŖŘṘṞ", toupper("rŕŗřṙṟ"))
  call assert_equal("SŚŜŞŠṠ", toupper("sśŝşšṡ"))
  call assert_equal("TŢŤŦṪṮẗ", toupper("tţťŧṫṯẗ"))
  call assert_equal("UÙÚÛÜŨŪŬŮŰŲƯǓỦ", toupper("uùúûüũūŭůűųưǔủ"))
  call assert_equal("VṼ", toupper("vṽ"))
  call assert_equal("WŴẀẂẄẆẘ", toupper("wŵẁẃẅẇẘ"))
  call assert_equal("ẊẌ", toupper("ẋẍ"))
  call assert_equal("YÝŸŶẎẙỲỶỸ", toupper("yýÿŷẏẙỳỷỹ"))
  call assert_equal("ZŹŻŽƵẐẔ", toupper("zźżžƶẑẕ"))

  " Test that uppercase diacritics, which should remain unchanged.
  call assert_equal("AÀÁÂÃÄÅĀĂĄǍǞǠẢ", toupper("AÀÁÂÃÄÅĀĂĄǍǞǠẢ"))
  call assert_equal("BḂḆ", toupper("BḂḆ"))
  call assert_equal("CÇĆĈĊČ", toupper("CÇĆĈĊČ"))
  call assert_equal("DĎĐḊḎḐ", toupper("DĎĐḊḎḐ"))
  call assert_equal("EÈÉÊËĒĔĖĘĚẺẼ", toupper("EÈÉÊËĒĔĖĘĚẺẼ"))
  call assert_equal("FḞ ", toupper("FḞ "))
  call assert_equal("GĜĞĠĢǤǦǴḠ", toupper("GĜĞĠĢǤǦǴḠ"))
  call assert_equal("HĤĦḢḦḨ", toupper("HĤĦḢḦḨ"))
  call assert_equal("IÌÍÎÏĨĪĬĮİǏỈ", toupper("IÌÍÎÏĨĪĬĮİǏỈ"))
  call assert_equal("JĴ", toupper("JĴ"))
  call assert_equal("KĶǨḰḴ", toupper("KĶǨḰḴ"))
  call assert_equal("LĹĻĽĿŁḺ", toupper("LĹĻĽĿŁḺ"))
  call assert_equal("MḾṀ", toupper("MḾṀ"))
  call assert_equal("NÑŃŅŇṄṈ", toupper("NÑŃŅŇṄṈ"))
  call assert_equal("OÒÓÔÕÖØŌŎŐƠǑǪǬỎ", toupper("OÒÓÔÕÖØŌŎŐƠǑǪǬỎ"))
  call assert_equal("PṔṖ", toupper("PṔṖ"))
  call assert_equal("Q", toupper("Q"))
  call assert_equal("RŔŖŘṘṞ", toupper("RŔŖŘṘṞ"))
  call assert_equal("SŚŜŞŠṠ", toupper("SŚŜŞŠṠ"))
  call assert_equal("TŢŤŦṪṮ", toupper("TŢŤŦṪṮ"))
  call assert_equal("UÙÚÛÜŨŪŬŮŰŲƯǓỦ", toupper("UÙÚÛÜŨŪŬŮŰŲƯǓỦ"))
  call assert_equal("VṼ", toupper("VṼ"))
  call assert_equal("WŴẀẂẄẆ", toupper("WŴẀẂẄẆ"))
  call assert_equal("XẊẌ", toupper("XẊẌ"))
  call assert_equal("YÝŶŸẎỲỶỸ", toupper("YÝŶŸẎỲỶỸ"))
  call assert_equal("ZŹŻŽƵẐẔ", toupper("ZŹŻŽƵẐẔ"))

  call assert_equal("Ⱥ Ⱦ", toupper("ⱥ ⱦ"))
endfunc


