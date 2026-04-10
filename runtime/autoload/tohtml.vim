vim9script
# Vim autoload file for the tohtml plugin.
# Maintainer: Ben Fritz <fritzophrenic@gmail.com>
# Last Change: 2026 Apr 4
#
# Additional contributors:
#
#         Original by Bram Moolenaar <Bram@vim.org>
#         Diff2HTML() added by Christian Brabandt <cb@256bit.org>
#
#         See Mercurial change logs for more!

# this file uses line continuations (but in vim9script we avoid them)
# Automatically find charsets from all encodings supported natively by Vim. With
# the 8bit- and 2byte- prefixes, Vim can actually support more encodings than
# this. Let the user specify these however since they won't be supported on
# every system.
#
# Note, not all of Vim's supported encodings have a charset to use.
#
# Names in this list are from:
#   http://www.iana.org/assignments/character-sets
# export const encoding_to_charset: {{{
export const encoding_to_charset = {
  'latin1': 'ISO-8859-1',
  'iso-8859-2': 'ISO-8859-2',
  'iso-8859-3': 'ISO-8859-3',
  'iso-8859-4': 'ISO-8859-4',
  'iso-8859-5': 'ISO-8859-5',
  'iso-8859-6': 'ISO-8859-6',
  'iso-8859-7': 'ISO-8859-7',
  'iso-8859-8': 'ISO-8859-8',
  'iso-8859-9': 'ISO-8859-9',
  'iso-8859-10': '',
  'iso-8859-13': 'ISO-8859-13',
  'iso-8859-14': '',
  'iso-8859-15': 'ISO-8859-15',
  'koi8-r': 'KOI8-R',
  'koi8-u': 'KOI8-U',
  'macroman': 'macintosh',
  'cp437': '',
  'cp775': '',
  'cp850': '',
  'cp852': '',
  'cp855': '',
  'cp857': '',
  'cp860': '',
  'cp861': '',
  'cp862': '',
  'cp863': '',
  'cp865': '',
  'cp866': 'IBM866',
  'cp869': '',
  'cp874': '',
  'cp1250': 'windows-1250',
  'cp1251': 'windows-1251',
  'cp1253': 'windows-1253',
  'cp1254': 'windows-1254',
  'cp1255': 'windows-1255',
  'cp1256': 'windows-1256',
  'cp1257': 'windows-1257',
  'cp1258': 'windows-1258',
  'euc-jp': 'EUC-JP',
  'sjis': 'Shift_JIS',
  'cp932': 'Shift_JIS',
  'cp949': '',
  'euc-kr': 'EUC-KR',
  'cp936': 'GBK',
  'euc-cn': 'GB2312',
  'big5': 'Big5',
  'cp950': 'Big5',
  'utf-8': 'UTF-8',
  'ucs-2': 'UTF-8',
  'ucs-2le': 'UTF-8',
  'utf-16': 'UTF-8',
  'utf-16le': 'UTF-8',
  'ucs-4': 'UTF-8',
  'ucs-4le': 'UTF-8',
}

# Notes:
#   1. All UCS/UTF are converted to UTF-8 because it is much better supported
#   2. Any blank spaces are there because Vim supports it but at least one major
#      web browser does not according to http://wiki.whatwg.org/wiki/Web_Encodings.
# }}}

# Only automatically find encodings supported natively by Vim, let the user
# specify the encoding if it's not natively supported. This function is only
# used when the user specifies the charset, they better know what they are
# doing!
#
# Names in this list are from:
#   http://www.iana.org/assignments/character-sets
# export const charset_to_encoding: {{{
export const charset_to_encoding = {
  'iso_8859-1:1987': 'latin1',
  'iso-ir-100': 'latin1',
  'iso_8859-1': 'latin1',
  'iso-8859-1': 'latin1',
  'latin1': 'latin1',
  'l1': 'latin1',
  'ibm819': 'latin1',
  'cp819': 'latin1',
  'csisolatin1': 'latin1',
  'iso_8859-2:1987': 'iso-8859-2',
  'iso-ir-101': 'iso-8859-2',
  'iso_8859-2': 'iso-8859-2',
  'iso-8859-2': 'iso-8859-2',
  'latin2': 'iso-8859-2',
  'l2': 'iso-8859-2',
  'csisolatin2': 'iso-8859-2',
  'iso_8859-3:1988': 'iso-8859-3',
  'iso-ir-109': 'iso-8859-3',
  'iso_8859-3': 'iso-8859-3',
  'iso-8859-3': 'iso-8859-3',
  'latin3': 'iso-8859-3',
  'l3': 'iso-8859-3',
  'csisolatin3': 'iso-8859-3',
  'iso_8859-4:1988': 'iso-8859-4',
  'iso-ir-110': 'iso-8859-4',
  'iso_8859-4': 'iso-8859-4',
  'iso-8859-4': 'iso-8859-4',
  'latin4': 'iso-8859-4',
  'l4': 'iso-8859-4',
  'csisolatin4': 'iso-8859-4',
  'iso_8859-5:1988': 'iso-8859-5',
  'iso-ir-144': 'iso-8859-5',
  'iso_8859-5': 'iso-8859-5',
  'iso-8859-5': 'iso-8859-5',
  'cyrillic': 'iso-8859-5',
  'csisolatincyrillic': 'iso-8859-5',
  'iso_8859-6:1987': 'iso-8859-6',
  'iso-ir-127': 'iso-8859-6',
  'iso_8859-6': 'iso-8859-6',
  'iso-8859-6': 'iso-8859-6',
  'ecma-114': 'iso-8859-6',
  'asmo-708': 'iso-8859-6',
  'arabic': 'iso-8859-6',
  'csisolatinarabic': 'iso-8859-6',
  'iso_8859-7:1987': 'iso-8859-7',
  'iso-ir-126': 'iso-8859-7',
  'iso_8859-7': 'iso-8859-7',
  'iso-8859-7': 'iso-8859-7',
  'elot_928': 'iso-8859-7',
  'ecma-118': 'iso-8859-7',
  'greek': 'iso-8859-7',
  'greek8': 'iso-8859-7',
  'csisolatingreek': 'iso-8859-7',
  'iso_8859-8:1988': 'iso-8859-8',
  'iso-ir-138': 'iso-8859-8',
  'iso_8859-8': 'iso-8859-8',
  'iso-8859-8': 'iso-8859-8',
  'hebrew': 'iso-8859-8',
  'csisolatinhebrew': 'iso-8859-8',
  'iso_8859-9:1989': 'iso-8859-9',
  'iso-ir-148': 'iso-8859-9',
  'iso_8859-9': 'iso-8859-9',
  'iso-8859-9': 'iso-8859-9',
  'latin5': 'iso-8859-9',
  'l5': 'iso-8859-9',
  'csisolatin5': 'iso-8859-9',
  'iso-8859-10': 'iso-8859-10',
  'iso-ir-157': 'iso-8859-10',
  'l6': 'iso-8859-10',
  'iso_8859-10:1992': 'iso-8859-10',
  'csisolatin6': 'iso-8859-10',
  'latin6': 'iso-8859-10',
  'iso-8859-13': 'iso-8859-13',
  'iso-8859-14': 'iso-8859-14',
  'iso-ir-199': 'iso-8859-14',
  'iso_8859-14:1998': 'iso-8859-14',
  'iso_8859-14': 'iso-8859-14',
  'latin8': 'iso-8859-14',
  'iso-celtic': 'iso-8859-14',
  'l8': 'iso-8859-14',
  'iso-8859-15': 'iso-8859-15',
  'iso_8859-15': 'iso-8859-15',
  'latin-9': 'iso-8859-15',
  'koi8-r': 'koi8-r',
  'cskoi8r': 'koi8-r',
  'koi8-u': 'koi8-u',
  'macintosh': 'macroman',
  'mac': 'macroman',
  'csmacintosh': 'macroman',
  'ibm437': 'cp437',
  'cp437': 'cp437',
  '437': 'cp437',
  'cspc8codepage437': 'cp437',
  'ibm775': 'cp775',
  'cp775': 'cp775',
  'cspc775baltic': 'cp775',
  'ibm850': 'cp850',
  'cp850': 'cp850',
  '850': 'cp850',
  'cspc850multilingual': 'cp850',
  'ibm852': 'cp852',
  'cp852': 'cp852',
  '852': 'cp852',
  'cspcp852': 'cp852',
  'ibm855': 'cp855',
  'cp855': 'cp855',
  '855': 'cp855',
  'csibm855': 'cp855',
  'ibm857': 'cp857',
  'cp857': 'cp857',
  '857': 'cp857',
  'csibm857': 'cp857',
  'ibm860': 'cp860',
  'cp860': 'cp860',
  '860': 'cp860',
  'csibm860': 'cp860',
  'ibm861': 'cp861',
  'cp861': 'cp861',
  '861': 'cp861',
  'cp-is': 'cp861',
  'csibm861': 'cp861',
  'ibm862': 'cp862',
  'cp862': 'cp862',
  '862': 'cp862',
  'cspc862latinhebrew': 'cp862',
  'ibm863': 'cp863',
  'cp863': 'cp863',
  '863': 'cp863',
  'csibm863': 'cp863',
  'ibm865': 'cp865',
  'cp865': 'cp865',
  '865': 'cp865',
  'csibm865': 'cp865',
  'ibm866': 'cp866',
  'cp866': 'cp866',
  '866': 'cp866',
  'csibm866': 'cp866',
  'ibm869': 'cp869',
  'cp869': 'cp869',
  '869': 'cp869',
  'cp-gr': 'cp869',
  'csibm869': 'cp869',
  'windows-1250': 'cp1250',
  'windows-1251': 'cp1251',
  'windows-1253': 'cp1253',
  'windows-1254': 'cp1254',
  'windows-1255': 'cp1255',
  'windows-1256': 'cp1256',
  'windows-1257': 'cp1257',
  'windows-1258': 'cp1258',
  'extended_unix_code_packed_format_for_japanese': 'euc-jp',
  'cseucpkdfmtjapanese': 'euc-jp',
  'euc-jp': 'euc-jp',
  'shift_jis': 'sjis',
  'ms_kanji': 'sjis',
  'sjis': 'sjis',
  'csshiftjis': 'sjis',
  'ibm-thai': 'cp874',
  'csibmthai': 'cp874',
  'ks_c_5601-1987': 'cp949',
  'iso-ir-149': 'cp949',
  'ks_c_5601-1989': 'cp949',
  'ksc_5601': 'cp949',
  'korean': 'cp949',
  'csksc56011987': 'cp949',
  'euc-kr': 'euc-kr',
  'cseuckr': 'euc-kr',
  'gbk': 'cp936',
  'cp936': 'cp936',
  'ms936': 'cp936',
  'windows-936': 'cp936',
  'gb_2312-80': 'euc-cn',
  'iso-ir-58': 'euc-cn',
  'chinese': 'euc-cn',
  'csiso58gb231280': 'euc-cn',
  'big5': 'big5',
  'csbig5': 'big5',
  'utf-8': 'utf-8',
  'iso-10646-ucs-2': 'ucs-2',
  'csunicode': 'ucs-2',
  'utf-16': 'utf-16',
  'utf-16be': 'utf-16',
  'utf-16le': 'utf-16le',
  'utf-32': 'ucs-4',
  'utf-32be': 'ucs-4',
  'utf-32le': 'ucs-4le',
  'iso-10646-ucs-4': 'ucs-4',
  'csucs4': 'ucs-4'
}
#}}}

var settings: dict<any>

export def Convert2HTML(line1: number, line2: number)
  settings = GetUserSettings()

  if !&diff || settings.diff_one_file #{{{
    if line2 >= line1
      g:html_start_line = line1
      g:html_end_line = line2
    else
      g:html_start_line = line2
      g:html_end_line = line1
    endif
    runtime syntax/2html.vim #}}}
  else #{{{
    var win_list = range(1, winnr('$'))->mapnew((_, w) => winbufnr(w))
    var buf_list: list<number>
    settings.whole_filler = 1
    g:html_diff_win_num = 0
    for window in win_list
      # switch to the next buffer to convert
      win_gotoid(bufwinid(window))

      # figure out whether current charset and encoding will work, if not
      # default to UTF-8
      if !exists('g:html_use_encoding') &&
	  (((&l:fileencoding == '' || (&l:buftype != '' && &l:buftype !=? 'help'))
	  && &encoding !=? settings.vim_encoding)
	  || &l:fileencoding != '' && &l:fileencoding !=? settings.vim_encoding)
	echohl WarningMsg
	echomsg "TOhtml: mismatched file encodings in Diff buffers, using UTF-8"
	echohl None
	settings.vim_encoding = 'utf-8'
	settings.encoding = 'UTF-8'
      endif

      # set up for diff-mode conversion
      g:html_start_line = 1
      g:html_end_line = line('$')
      g:html_diff_win_num += 1

      # convert this file
      runtime syntax/2html.vim

      # remember the HTML buffer for later combination
      add(buf_list, bufnr('%'))
    endfor
    unlet g:html_diff_win_num
    Diff2HTML(win_list, buf_list)
  endif
  unlet g:html_start_line
  unlet g:html_end_line
  settings = null_dict
enddef

export def Diff2HTML(win_list: list<number>, buf_list: list<number>)
  var xml_line = ''
  var tag_close = '>'

  var old_paste = &paste
  set paste
  var old_magic = &magic
  set magic

  var html = []
  var style: list<string>
  var body_line: string
  var body_end_line: string
  var body_line_num: number
  var html5 = 0
  if !settings.no_doc
    if settings.use_xhtml
      if settings.encoding != ''
	xml_line = "<?xml version=\"1.0\" encoding=\"" .. settings.encoding .. "\"?>"
      else
	xml_line = "<?xml version=\"1.0\"?>"
      endif
      tag_close = ' />'
    endif

    style = [settings.use_xhtml ? '' : '-->']

    if settings.use_xhtml
      add(html, xml_line)
    endif
    if settings.use_xhtml
      add(html, "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">")
      add(html, '<html xmlns="http://www.w3.org/1999/xhtml">')
    elseif settings.use_css && !settings.no_pre
      add(html, "<!DOCTYPE html>")
      add(html, '<html>')
      html5 = 1
    else
      add(html, '<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN"')
      add(html, '  "http://www.w3.org/TR/html4/loose.dtd">')
      add(html, '<html>')
    endif
    add(html, '<head>')

    # include encoding as close to the top as possible, but only if not already
    # contained in XML information
    if settings.encoding != '' && !settings.use_xhtml
      if html5
	add(html, '<meta charset="' .. settings.encoding .. '"' .. tag_close)
      else
	add(html, "<meta http-equiv=\"content-type\" content=\"text/html; charset=" .. settings.encoding .. '"' .. tag_close)
      endif
    endif

    add(html, '<title>diff</title>')
    add(html, $'<meta name="Generator" content="Vim/{v:version / 100}.{v:version % 100}"{tag_close}')
    add(html, $'<meta name="plugin-version" content="{g:loaded_2html_plugin}"{tag_close}')
    add(html, '<meta name="settings" content="' ..
      join(filter(keys(settings), (_, k) => {
	var v = settings[k]
	return type(v) == v:t_number ? v == 1 : false
      }), ',') ..
      ',prevent_copy=' .. settings.prevent_copy ..
      ',use_input_for_pc=' .. settings.use_input_for_pc ..
      '"' .. tag_close)
    add(html, '<meta name="colorscheme" content="' ..
      (exists('g:colors_name')
      ? g:colors_name
      : 'none') .. '"' .. tag_close)

    add(html, '</head>')
    body_line_num = len(html)
    add(html, '<body' .. (settings.line_ids ? ' onload="JumpToLine();"' : '') .. '>')
  endif
  add(html, "<table " .. (settings.use_css ? "" : "border='1' width='100%' ") .. "id='vimCodeElement" .. settings.id_suffix .. "'>")

  add(html, '<tr>')
  for buf in win_list
    add(html, '<th>' .. bufname(buf) .. '</th>')
  endfor
  add(html, '</tr><tr>')

  var diff_style_start = 0
  var insert_index = 0

  for buf in buf_list
    var temp = []
    win_gotoid(bufwinid(buf))

    # If text is folded because of user foldmethod settings, etc. we don't want
    # to act on everything in a fold by mistake.
    setlocal nofoldenable

    # When not using CSS or when using xhtml, the <body> line can be important.
    # Assume it will be the same for all buffers and grab it from the first
    # buffer. Similarly, need to grab the body end line as well.
    if !settings.no_doc
      if body_line == ''
	:1
	search('<body')
	body_line = getline('.')
	:$
	search('</body>', 'b')
	body_end_line = getline('.')
      endif

      # Grab the style information. Some of this will be duplicated so only insert
      # it if it's not already there. {{{
      :1
      var style_start = search('^<style\( type="text/css"\)\?>')
      :1
      var style_end = search('^</style>')
      if style_start > 0 && style_end > 0
	var buf_styles = getline(style_start + 1, style_end - 1)
	for a_style in buf_styles
	  if index(style, a_style) == -1
	    if diff_style_start == 0
	      if a_style =~ '\<Diff\(Change\|Text\|Add\|Delete\)'
		diff_style_start = len(style) - 1
	      endif
	    endif
	    insert(style, a_style, insert_index)
	    insert_index += 1
	  endif
	endfor
      endif # }}}

      # everything new will get added before the diff styles so diff highlight
      # properly overrides normal highlight
      if diff_style_start != 0
	insert_index = diff_style_start
      endif

      # Delete those parts that are not needed so we can include the rest into the
      # resulting table.
      :1,/^<body.*\%(\n<!--.*-->\_s\+.*id='oneCharWidth'.*\_s\+.*id='oneInputWidth'.*\_s\+.*id='oneEmWidth'\)\?\zs/d _
      :$
      :?</body>?,$d _
    elseif !settings.no_modeline
      # remove modeline from source files if it is included and we haven't deleted
      # due to removing html footer already
      :$d
    endif
    temp = getline(1, '$')
    # clean out id on the main content container because we already set it on
    # the table
    temp[0] = substitute(temp[0], " id='vimCodeElement[^']*'", "", "")
    # undo deletion of start and end part
    # so we can later save the file as valid html
    # TODO: restore using grabbed lines if undolevel is 1?
    if !settings.no_doc
      normal! 2u
    elseif !settings.no_modeline
      normal! u
    endif
    if settings.use_css
      add(html, '<td><div>')
    elseif settings.use_xhtml
      add(html, '<td nowrap="nowrap" valign="top"><div>')
    else
      add(html, '<td nowrap valign="top"><div>')
    endif
    html += temp
    add(html, '</div></td>')

    # Close this buffer
    # TODO: the comment above says we're going to allow saving the file
    # later .. .but here we discard it?
    quit!
  endfor

  if !settings.no_doc
    html[body_line_num] = body_line
  endif

  add(html, '</tr>')
  add(html, '</table>')
  if !settings.no_doc
    add(html, body_end_line)
    add(html, '</html>')
  endif

  # The generated HTML is admittedly ugly and takes a LONG time to fold.
  # Make sure the user doesn't do syntax folding when loading a generated file,
  # using a modeline.
  if !settings.no_modeline
    add(html, '<!-- vim: set foldmethod=manual : -->')
  endif

  var i = 1
  var name = "Diff" .. (settings.use_xhtml ? ".xhtml" : ".html")
  # Find an unused file name if current file name is already in use
  while filereadable(name)
    name = substitute(name, '\d*\.x\?html$', '', '') .. i .. '.' .. fnamemodify(copy(name), ":t:e")
    i += 1
  endwhile

  var ei_sav = &eventignore
  set eventignore+=FileType
  execute "topleft new " .. name
  &eventignore = ei_sav

  setlocal modifiable

  # just in case some user autocmd creates content in the new buffer, make sure
  # it is empty before proceeding
  :%d

  # set the fileencoding to match the charset we'll be using
  &fileencoding = settings.vim_encoding

  # According to http://www.w3.org/TR/html4/charset.html#doc-char-set, the byte
  # order mark is highly recommend on the web when using multibyte encodings. But,
  # it is not a good idea to include it on UTF-8 files. Otherwise, let Vim
  # determine when it is actually inserted.
  if settings.vim_encoding == 'utf-8'
    setlocal nobomb
  else
    setlocal bomb
  endif

  append(0, html)

  if !settings.no_doc
    if len(style) > 0
      :1
      var style_start = search('^</head>') - 1

      # add required javascript in reverse order so we can just call append again
      # and again without adjusting #{{{

      var uses_script = settings.dynamic_folds || settings.line_ids

      # insert script closing tag if needed
      if uses_script
	append(style_start, [
	  '',
	  settings.use_xhtml ? '//]]>' : '-->',
	  "</script>"
	])
      endif

      # insert javascript to get IDs from line numbers, and to open a fold before
      # jumping to any lines contained therein
      if settings.line_ids
	append(style_start, [
	  "  /* Always jump to new location even if the line was hidden inside a fold, or",
	  "   * we corrected the raw number to a line ID.",
	  "   */",
	  "  if (lineElem) {",
	  "    lineElem.scrollIntoView(true);",
	  "  }",
	  "  return true;",
	  "}",
	  "if ('onhashchange' in window) {",
	  "  window.onhashchange = JumpToLine;",
	  "}"
	])

	if settings.dynamic_folds
	  append(style_start, [
	    "",
	  "  /* navigate upwards in the DOM tree to open all folds containing the line */",
	  "  var node = lineElem;",
	  "  while (node && node.id != 'vimCodeElement" .. settings.id_suffix .. "')",
	  "  {",
	  "    if (node.className == 'closed-fold')",
	  "    {",
	  "      /* toggle open the fold ID (remove window ID) */",
	  "      toggleFold(node.id.substr(4));",
	  "    }",
	  "    node = node.parentNode;",
	  "  }",
	  ])
	endif
      endif

      if settings.line_ids
	append(style_start, [
	  "",
	  "/* function to open any folds containing a jumped-to line before jumping to it */",
	  "function JumpToLine()",
	  "{",
	  "  var lineNum;",
	  "  lineNum = window.location.hash;",
	  "  lineNum = lineNum.substr(1); /* strip off '#' */",
	  "",
	"  if (lineNum.indexOf('L') == -1) {",
	"    lineNum = 'L'+lineNum;",
	"  }",
	"  if (lineNum.indexOf('W') == -1) {",
	"    lineNum = 'W1'+lineNum;",
	"  }",
	"  var lineElem = document.getElementById(lineNum);"
	])
      endif

      # Insert javascript to toggle matching folds open and closed in all windows,
      # if dynamic folding is active.
      if settings.dynamic_folds
	append(style_start, [
	"  function toggleFold(objID)",
	"  {",
	"    for (win_num = 1; win_num <= " .. len(buf_list) .. "; win_num++)",
	"    {",
	"      var fold;",
	"      fold = document.getElementById(\"win\"+win_num+objID);",
	"      if(fold.className == 'closed-fold')",
	"      {",
	"        fold.className = 'open-fold';",
	"      }",
	"      else if (fold.className == 'open-fold')",
	"      {",
	"        fold.className = 'closed-fold';",
	"      }",
	"    }",
	"  }",
	])
      endif

      if uses_script
	# insert script tag if needed
	append(style_start, [
	  "<script" .. (html5 ? "" : " type='text/javascript'") .. ">",
	  settings.use_xhtml ? '//<![CDATA[' : "<!--"])
      endif

      # Insert styles from all the generated html documents and additional styles
      # for the table-based layout of the side-by-side diff. The diff should take
      # up the full browser window (but not more), and be static in size,
      # horizontally scrollable when the lines are too long. Otherwise, the diff
      if settings.use_css
	append(style_start,
	  ['<style' .. (html5 ? '' : 'type="text/css"') .. '>'] +
	  style +
	  [ settings.use_xhtml ? '' : '<!--',
	    'table { table-layout: fixed; }',
	    'html, body, table, tbody { width: 100%; margin: 0; padding: 0; }',
	    'table, td, th { border: 1px solid; }',
	    'td { vertical-align: top; }',
	    'th, td { width: ' .. printf("%.1f", 100.0 / len(win_list)) .. '%; }',
	    'td div { overflow: auto; }',
	    settings.use_xhtml ? '' : '-->',
	    '</style>'
	  ])
      endif #}}}
    endif
  endif

  &paste = old_paste
  &magic = old_magic
enddef

# Gets a single user option and sets it in the passed-in Dict, or gives it the
# default value if the option doesn't actually exist.
export def GetOption(_settings: dict<any>, option: string, default: any)
  _settings[option] = get(g:, $'html_{option}', default)
enddef

# returns a Dict containing the values of all user options for 2html, including
# default values for those not given an explicit value by the user. Discards the
# html_ prefix of the option for nicer looking code.
export def GetUserSettings(): dict<any>
  if !empty(settings)
    # just restore the known options if we've already retrieved them
    return settings
  else
    # otherwise figure out which options are set
    var user_settings = {}

    # Define the correct option if the old option name exists and we haven't
    # already defined the correct one.
    if exists('g:use_xhtml') && !exists("g:html_use_xhtml")
      echohl WarningMsg
      echomsg "Warning: g:use_xhtml is deprecated, use g:html_use_xhtml"
      echohl None
      g:html_use_xhtml = g:use_xhtml
    endif

    # get current option settings with appropriate defaults {{{
    GetOption(user_settings,       'no_progress', !has("statusline") )
    GetOption(user_settings,     'diff_one_file', 0 )
    GetOption(user_settings,      'number_lines', &number )
    GetOption(user_settings,          'pre_wrap', &wrap )
    GetOption(user_settings,           'use_css', 1 )
    GetOption(user_settings,    'ignore_conceal', 0 )
    GetOption(user_settings,    'ignore_folding', 0 )
    GetOption(user_settings,     'dynamic_folds', 0 )
    GetOption(user_settings,     'no_foldcolumn', user_settings.ignore_folding)
    GetOption(user_settings,      'hover_unfold', 0 )
    GetOption(user_settings,            'no_pre', 0 )
    GetOption(user_settings,            'no_doc', 0 )
    GetOption(user_settings,          'no_links', 0 )
    GetOption(user_settings,       'no_modeline', 0 )
    GetOption(user_settings,        'no_invalid', 0 )
    GetOption(user_settings,      'whole_filler', 0 )
    GetOption(user_settings,         'use_xhtml', 0 )
    GetOption(user_settings,          'line_ids', user_settings.number_lines )
    GetOption(user_settings, 'use_input_for_pc', 'none')
    # }}}

    # override those settings that need it {{{

    # hover opening implies dynamic folding
    if user_settings.hover_unfold
      user_settings.dynamic_folds = 1
    endif

    # ignore folding overrides dynamic folding
    if user_settings.ignore_folding && user_settings.dynamic_folds
      user_settings.dynamic_folds = 0
      user_settings.hover_unfold = 0
    endif

    # dynamic folding with no foldcolumn implies hover opens
    if user_settings.dynamic_folds && user_settings.no_foldcolumn
      user_settings.hover_unfold = 1
    endif

    # dynamic folding implies css
    if user_settings.dynamic_folds
      user_settings.use_css = 1
    else
      user_settings.no_foldcolumn = 1 # won't do anything but for consistency and for the test suite
    endif

    # if we're not using CSS we cannot use a pre section because <font> tags
    # aren't allowed inside a <pre> block
    if !user_settings.use_css
      user_settings.no_pre = 1
    endif

    # pre_wrap doesn't do anything if not using pre or not using CSS
    if user_settings.no_pre || !user_settings.use_css
      user_settings.pre_wrap = 0
    endif
    #}}}

    # set up expand_tabs option after all the overrides so we know the
    # appropriate defaults #{{{
    if user_settings.no_pre == 0
      GetOption(user_settings,
	'expand_tabs',
	&expandtab || &ts != 8 || &vts != '' || user_settings.number_lines ||
	(user_settings.dynamic_folds && !user_settings.no_foldcolumn))
    else
      user_settings.expand_tabs = 1
    endif
    # }}}

    # textual options
    if exists("g:html_use_encoding") #{{{
      # user specified the desired MIME charset, figure out proper
      # 'fileencoding' from it or warn the user if we cannot
      user_settings.encoding = g:html_use_encoding
      user_settings.vim_encoding = EncodingFromCharset(g:html_use_encoding)
      if user_settings.vim_encoding == ''
	echohl WarningMsg
	echomsg "TOhtml: file encoding for" g:html_use_encoding "unknown, please set 'fileencoding'"
	echohl None
      endif
    else
      # Figure out proper MIME charset from 'fileencoding' if possible
      if &l:fileencoding != ''
	# If the buffer is not a "normal" type, the 'fileencoding' value may not
	# be trusted; since the buffer should not be written the fileencoding is
	# not intended to be used.
	if &l:buftype == '' || &l:buftype ==? 'help'
	  user_settings.vim_encoding = &l:fileencoding
	  CharsetFromEncoding(user_settings)
	else
	  user_settings.encoding = '' # trigger detection using &encoding
	endif
      endif

      # else from 'encoding' if possible
      if &l:fileencoding == '' || user_settings.encoding == ''
	user_settings.vim_encoding = &encoding
	CharsetFromEncoding(user_settings)
      endif

      # else default to UTF-8 and warn user
      if user_settings.encoding == ''
	user_settings.vim_encoding = 'utf-8'
	user_settings.encoding = 'UTF-8'
	echohl WarningMsg
	echomsg "TOhtml: couldn't determine MIME charset, using UTF-8"
	echohl None
      endif
    endif #}}}

    # Default to making nothing uncopyable, because we default to
    # not-standards way of doing things, and also because Microsoft Word and
    # others paste the <input> elements anyway.
    #
    # html_prevent_copy only has an effect when using CSS.
    #
    # All options:
    #	  f - fold column
    #	  n - line numbers (also within fold text)
    #	  t - fold text
    #	  d - diff filler
    #	  c - concealed text (reserved future)
    #	  l - listchars (reserved possible future)
    #	  s - signs (reserved possible future)
    #
    # Normal text is always selectable.
    user_settings.prevent_copy = ""
    if user_settings.use_css
      if exists("g:html_prevent_copy")
	if user_settings.dynamic_folds && !user_settings.no_foldcolumn && g:html_prevent_copy =~# 'f'
	  user_settings.prevent_copy ..= 'f'
	endif
	if user_settings.number_lines && g:html_prevent_copy =~# 'n'
	  user_settings.prevent_copy ..= 'n'
	endif
	if &diff && g:html_prevent_copy =~# 'd'
	  user_settings.prevent_copy ..= 'd'
	endif
	if !user_settings.ignore_folding && g:html_prevent_copy =~# 't'
	  user_settings.prevent_copy ..= 't'
	endif
      else
	user_settings.prevent_copy = ""
      endif
    endif
    if empty(user_settings.prevent_copy)
      user_settings.no_invalid = 0
    endif

    # enforce valid values for use_input_for_pc
    if user_settings.use_input_for_pc !~# 'fallback\|none\|all'
      user_settings.use_input_for_pc = 'none'
      echohl WarningMsg
      echomsg '2html: "' .. g:html_use_input_for_pc .. '" is not valid for g:html_use_input_for_pc'
      echomsg '2html: defaulting to "' .. user_settings.use_input_for_pc .. '"'
      echohl None
      sleep 3
    endif

    if exists('g:html_id_expr')
      user_settings.id_suffix = eval(g:html_id_expr)
      if user_settings.id_suffix !~ '^[-_:.A-Za-z0-9]*$'
	echohl WarningMsg
	echomsg '2html: g:html_id_expr evaluated to invalid string for HTML id attributes'
	echomsg '2html: Omitting user-specified suffix'
	echohl None
	sleep 3
	user_settings.id_suffix = ""
      endif
    else
      user_settings.id_suffix = ""
    endif

    # TODO: font

    return user_settings
  endif
enddef

# get the proper HTML charset name from a Vim encoding option.
export def CharsetFromEncoding(_settings: dict<any>) #{{{
  var vim_encoding = _settings.vim_encoding
  if exists('g:html_charset_override') && has_key(g:html_charset_override, vim_encoding)
    _settings.encoding = g:html_charset_override[vim_encoding]
  else
    if vim_encoding =~ '^8bit\|^2byte'
      # 8bit- and 2byte- prefixes are to indicate encodings available on the
      # system that Vim will convert with iconv(), look up just the encoding name,
      # not Vim's prefix.
      vim_encoding = substitute(vim_encoding, '^8bit-\|^2byte-', '', '')
    endif
    if has_key(encoding_to_charset, vim_encoding)
      _settings.encoding = encoding_to_charset[vim_encoding]
    else
      _settings.encoding = ""
    endif
  endif
  if _settings.encoding != ""
    var vim_encoding2 = EncodingFromCharset(_settings.encoding)
    if vim_encoding2 != ""
      # if the Vim encoding to HTML encoding conversion is set up (by default or
      # by the user) to convert to a different encoding, we need to also change
      # the Vim encoding of the new buffer
      _settings.vim_encoding = vim_encoding2
    endif
  endif
enddef #}}}

# Get the proper Vim encoding option setting from an HTML charset name.
export def EncodingFromCharset(encoding: string): string #{{{
  if exists('g:html_encoding_override') && has_key(g:html_encoding_override, encoding)
    return g:html_encoding_override[encoding]
  elseif has_key(charset_to_encoding, tolower(encoding))
    return charset_to_encoding[tolower(encoding)]
  else
    return ""
  endif
enddef #}}}

# Make sure any patches will probably use consistent indent
#   vim: ts=8 sw=2 sts=2 noet fdm=marker
