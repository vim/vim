" Vim syntax support file
" Maintainer: Ben Fritz <fritzophrenic@gmail.com>
" Last Change: 2011 Jan 06
"
" Additional contributors:
"
"             Original by Bram Moolenaar <Bram@vim.org>
"             Modified by David Ne\v{c}as (Yeti) <yeti@physics.muni.cz>
"             XHTML support by Panagiotis Issaris <takis@lumumba.luc.ac.be>
"             Made w3 compliant by Edd Barrett <vext01@gmail.com>
"             Added html_font. Edd Barrett <vext01@gmail.com>
"             Progress bar based off code from "progressbar widget" plugin by
"               Andreas Politz, heavily modified:
"               http://www.vim.org/scripts/script.php?script_id=2006
"
"             See Mercurial change logs for more!

" Transform a file into HTML, using the current syntax highlighting.

" this file uses line continuations
let s:cpo_sav = &cpo
let s:ls  = &ls
set cpo-=C

let s:end=line('$')

" Font
if exists("g:html_font")
  let s:htmlfont = "'". g:html_font . "', monospace"
else
  let s:htmlfont = "monospace"
endif

let s:settings = tohtml#GetUserSettings()

" When not in gui we can only guess the colors.
if has("gui_running")
  let s:whatterm = "gui"
else
  let s:whatterm = "cterm"
  if &t_Co == 8
    let s:cterm_color = {0: "#808080", 1: "#ff6060", 2: "#00ff00", 3: "#ffff00", 4: "#8080ff", 5: "#ff40ff", 6: "#00ffff", 7: "#ffffff"}
  else
    let s:cterm_color = {0: "#000000", 1: "#c00000", 2: "#008000", 3: "#804000", 4: "#0000c0", 5: "#c000c0", 6: "#008080", 7: "#c0c0c0", 8: "#808080", 9: "#ff6060", 10: "#00ff00", 11: "#ffff00", 12: "#8080ff", 13: "#ff40ff", 14: "#00ffff", 15: "#ffffff"}

    " Colors for 88 and 256 come from xterm.
    if &t_Co == 88
      call extend(s:cterm_color, {16: "#000000", 17: "#00008b", 18: "#0000cd", 19: "#0000ff", 20: "#008b00", 21: "#008b8b", 22: "#008bcd", 23: "#008bff", 24: "#00cd00", 25: "#00cd8b", 26: "#00cdcd", 27: "#00cdff", 28: "#00ff00", 29: "#00ff8b", 30: "#00ffcd", 31: "#00ffff", 32: "#8b0000", 33: "#8b008b", 34: "#8b00cd", 35: "#8b00ff", 36: "#8b8b00", 37: "#8b8b8b", 38: "#8b8bcd", 39: "#8b8bff", 40: "#8bcd00", 41: "#8bcd8b", 42: "#8bcdcd", 43: "#8bcdff", 44: "#8bff00", 45: "#8bff8b", 46: "#8bffcd", 47: "#8bffff", 48: "#cd0000", 49: "#cd008b", 50: "#cd00cd", 51: "#cd00ff", 52: "#cd8b00", 53: "#cd8b8b", 54: "#cd8bcd", 55: "#cd8bff", 56: "#cdcd00", 57: "#cdcd8b", 58: "#cdcdcd", 59: "#cdcdff", 60: "#cdff00", 61: "#cdff8b", 62: "#cdffcd", 63: "#cdffff", 64: "#ff0000"})
      call extend(s:cterm_color, {65: "#ff008b", 66: "#ff00cd", 67: "#ff00ff", 68: "#ff8b00", 69: "#ff8b8b", 70: "#ff8bcd", 71: "#ff8bff", 72: "#ffcd00", 73: "#ffcd8b", 74: "#ffcdcd", 75: "#ffcdff", 76: "#ffff00", 77: "#ffff8b", 78: "#ffffcd", 79: "#ffffff", 80: "#2e2e2e", 81: "#5c5c5c", 82: "#737373", 83: "#8b8b8b", 84: "#a2a2a2", 85: "#b9b9b9", 86: "#d0d0d0", 87: "#e7e7e7"})
    elseif &t_Co == 256
      call extend(s:cterm_color, {16: "#000000", 17: "#00005f", 18: "#000087", 19: "#0000af", 20: "#0000d7", 21: "#0000ff", 22: "#005f00", 23: "#005f5f", 24: "#005f87", 25: "#005faf", 26: "#005fd7", 27: "#005fff", 28: "#008700", 29: "#00875f", 30: "#008787", 31: "#0087af", 32: "#0087d7", 33: "#0087ff", 34: "#00af00", 35: "#00af5f", 36: "#00af87", 37: "#00afaf", 38: "#00afd7", 39: "#00afff", 40: "#00d700", 41: "#00d75f", 42: "#00d787", 43: "#00d7af", 44: "#00d7d7", 45: "#00d7ff", 46: "#00ff00", 47: "#00ff5f", 48: "#00ff87", 49: "#00ffaf", 50: "#00ffd7", 51: "#00ffff", 52: "#5f0000", 53: "#5f005f", 54: "#5f0087", 55: "#5f00af", 56: "#5f00d7", 57: "#5f00ff", 58: "#5f5f00", 59: "#5f5f5f", 60: "#5f5f87", 61: "#5f5faf", 62: "#5f5fd7", 63: "#5f5fff", 64: "#5f8700"})
      call extend(s:cterm_color, {65: "#5f875f", 66: "#5f8787", 67: "#5f87af", 68: "#5f87d7", 69: "#5f87ff", 70: "#5faf00", 71: "#5faf5f", 72: "#5faf87", 73: "#5fafaf", 74: "#5fafd7", 75: "#5fafff", 76: "#5fd700", 77: "#5fd75f", 78: "#5fd787", 79: "#5fd7af", 80: "#5fd7d7", 81: "#5fd7ff", 82: "#5fff00", 83: "#5fff5f", 84: "#5fff87", 85: "#5fffaf", 86: "#5fffd7", 87: "#5fffff", 88: "#870000", 89: "#87005f", 90: "#870087", 91: "#8700af", 92: "#8700d7", 93: "#8700ff", 94: "#875f00", 95: "#875f5f", 96: "#875f87", 97: "#875faf", 98: "#875fd7", 99: "#875fff", 100: "#878700", 101: "#87875f", 102: "#878787", 103: "#8787af", 104: "#8787d7", 105: "#8787ff", 106: "#87af00", 107: "#87af5f", 108: "#87af87", 109: "#87afaf", 110: "#87afd7", 111: "#87afff", 112: "#87d700"})
      call extend(s:cterm_color, {113: "#87d75f", 114: "#87d787", 115: "#87d7af", 116: "#87d7d7", 117: "#87d7ff", 118: "#87ff00", 119: "#87ff5f", 120: "#87ff87", 121: "#87ffaf", 122: "#87ffd7", 123: "#87ffff", 124: "#af0000", 125: "#af005f", 126: "#af0087", 127: "#af00af", 128: "#af00d7", 129: "#af00ff", 130: "#af5f00", 131: "#af5f5f", 132: "#af5f87", 133: "#af5faf", 134: "#af5fd7", 135: "#af5fff", 136: "#af8700", 137: "#af875f", 138: "#af8787", 139: "#af87af", 140: "#af87d7", 141: "#af87ff", 142: "#afaf00", 143: "#afaf5f", 144: "#afaf87", 145: "#afafaf", 146: "#afafd7", 147: "#afafff", 148: "#afd700", 149: "#afd75f", 150: "#afd787", 151: "#afd7af", 152: "#afd7d7", 153: "#afd7ff", 154: "#afff00", 155: "#afff5f", 156: "#afff87", 157: "#afffaf", 158: "#afffd7"})
      call extend(s:cterm_color, {159: "#afffff", 160: "#d70000", 161: "#d7005f", 162: "#d70087", 163: "#d700af", 164: "#d700d7", 165: "#d700ff", 166: "#d75f00", 167: "#d75f5f", 168: "#d75f87", 169: "#d75faf", 170: "#d75fd7", 171: "#d75fff", 172: "#d78700", 173: "#d7875f", 174: "#d78787", 175: "#d787af", 176: "#d787d7", 177: "#d787ff", 178: "#d7af00", 179: "#d7af5f", 180: "#d7af87", 181: "#d7afaf", 182: "#d7afd7", 183: "#d7afff", 184: "#d7d700", 185: "#d7d75f", 186: "#d7d787", 187: "#d7d7af", 188: "#d7d7d7", 189: "#d7d7ff", 190: "#d7ff00", 191: "#d7ff5f", 192: "#d7ff87", 193: "#d7ffaf", 194: "#d7ffd7", 195: "#d7ffff", 196: "#ff0000", 197: "#ff005f", 198: "#ff0087", 199: "#ff00af", 200: "#ff00d7", 201: "#ff00ff", 202: "#ff5f00", 203: "#ff5f5f", 204: "#ff5f87"})
      call extend(s:cterm_color, {205: "#ff5faf", 206: "#ff5fd7", 207: "#ff5fff", 208: "#ff8700", 209: "#ff875f", 210: "#ff8787", 211: "#ff87af", 212: "#ff87d7", 213: "#ff87ff", 214: "#ffaf00", 215: "#ffaf5f", 216: "#ffaf87", 217: "#ffafaf", 218: "#ffafd7", 219: "#ffafff", 220: "#ffd700", 221: "#ffd75f", 222: "#ffd787", 223: "#ffd7af", 224: "#ffd7d7", 225: "#ffd7ff", 226: "#ffff00", 227: "#ffff5f", 228: "#ffff87", 229: "#ffffaf", 230: "#ffffd7", 231: "#ffffff", 232: "#080808", 233: "#121212", 234: "#1c1c1c", 235: "#262626", 236: "#303030", 237: "#3a3a3a", 238: "#444444", 239: "#4e4e4e", 240: "#585858", 241: "#626262", 242: "#6c6c6c", 243: "#767676", 244: "#808080", 245: "#8a8a8a", 246: "#949494", 247: "#9e9e9e", 248: "#a8a8a8", 249: "#b2b2b2", 250: "#bcbcbc", 251: "#c6c6c6", 252: "#d0d0d0", 253: "#dadada", 254: "#e4e4e4", 255: "#eeeeee"})
    endif
  endif
endif

" Return good color specification: in GUI no transformation is done, in
" terminal return RGB values of known colors and empty string for unknown
if s:whatterm == "gui"
  function! s:HtmlColor(color)
    return a:color
  endfun
else
  function! s:HtmlColor(color)
    if has_key(s:cterm_color, a:color)
      return s:cterm_color[a:color]
    else
      return ""
    endif
  endfun
endif

if !s:settings.use_css
  " Return opening HTML tag for given highlight id
  function! s:HtmlOpening(id)
    let a = ""
    if synIDattr(a:id, "inverse")
      " For inverse, we always must set both colors (and exchange them)
      let x = s:HtmlColor(synIDattr(a:id, "fg#", s:whatterm))
      let a = a . '<span style="background-color: ' . ( x != "" ? x : s:fgc ) . '">'
      let x = s:HtmlColor(synIDattr(a:id, "bg#", s:whatterm))
      let a = a . '<font color="' . ( x != "" ? x : s:bgc ) . '">'
    else
      let x = s:HtmlColor(synIDattr(a:id, "bg#", s:whatterm))
      if x != "" | let a = a . '<span style="background-color: ' . x . '">' | endif
      let x = s:HtmlColor(synIDattr(a:id, "fg#", s:whatterm))
      if x != "" | let a = a . '<font color="' . x . '">' | endif
    endif
    if synIDattr(a:id, "bold") | let a = a . "<b>" | endif
    if synIDattr(a:id, "italic") | let a = a . "<i>" | endif
    if synIDattr(a:id, "underline") | let a = a . "<u>" | endif
    return a
  endfun

  " Return closing HTML tag for given highlight id
  function! s:HtmlClosing(id)
    let a = ""
    if synIDattr(a:id, "underline") | let a = a . "</u>" | endif
    if synIDattr(a:id, "italic") | let a = a . "</i>" | endif
    if synIDattr(a:id, "bold") | let a = a . "</b>" | endif
    if synIDattr(a:id, "inverse")
      let a = a . '</font></span>'
    else
      let x = s:HtmlColor(synIDattr(a:id, "fg#", s:whatterm))
      if x != "" | let a = a . '</font>' | endif
      let x = s:HtmlColor(synIDattr(a:id, "bg#", s:whatterm))
      if x != "" | let a = a . '</span>' | endif
    endif
    return a
  endfun
endif

" Return HTML valid characters enclosed in a span of class style_name with
" unprintable characters expanded and double spaces replaced as necessary.
function! s:HtmlFormat(text, style_name, diff_style_name)
  " Replace unprintable characters
  let formatted = strtrans(a:text)

  " separate the two classes by a space to apply them both if there is a diff
  " style name
  let l:style_name = a:style_name . (a:diff_style_name == '' ? '' : ' ') . a:diff_style_name

  " Replace the reserved html characters
  let formatted = substitute(formatted, '&', '\&amp;',  'g')
  let formatted = substitute(formatted, '<', '\&lt;',   'g')
  let formatted = substitute(formatted, '>', '\&gt;',   'g')
  let formatted = substitute(formatted, '"', '\&quot;', 'g')
  " TODO: Use &apos; for "'"?

  " Replace a "form feed" character with HTML to do a page break
  let formatted = substitute(formatted, "\x0c", '<hr class="PAGE-BREAK">', 'g')

  " Mangle modelines so Vim doesn't try to use HTML text as a modeline if
  " editing this file in the future
  let formatted = substitute(formatted, '\v(\s+%(vim?|ex)):', '\1\&#0058;', 'g')

  " Replace double spaces, leading spaces, and trailing spaces if needed
  if ' ' != s:HtmlSpace
    let formatted = substitute(formatted, '  ', s:HtmlSpace . s:HtmlSpace, 'g')
    let formatted = substitute(formatted, '^ ', s:HtmlSpace, 'g')
    let formatted = substitute(formatted, ' \+$', s:HtmlSpace, 'g')
  endif

  " Enclose in a span of class style_name
  let formatted = '<span class="' . l:style_name . '">' . formatted . '</span>'

  " Add the class to class list if it's not there yet.
  " Add normal groups to the beginning so diff groups can override them.
  let s:id = hlID(a:style_name)
  if index(s:idlist, s:id ) == -1
    if a:style_name =~ 'Diff\%(Add\|Change\|Delete\|Text\)'
      call add(s:idlist, s:id)
    else
      call insert(s:idlist, s:id)
    endif
  endif
  
  " Add the diff highlight class to class list if used and it's not there yet.
  " Add diff groups to the end so they override the other highlighting.
  if a:diff_style_name != ""
    let s:diff_id = hlID(a:diff_style_name)
    if index(s:idlist, s:diff_id) == -1
      call add(s:idlist, s:diff_id)
    endif
  endif

  return formatted
endfun

" Return CSS style describing given highlight id (can be empty)
function! s:CSS1(id)
  let a = ""
  if synIDattr(a:id, "inverse")
    " For inverse, we always must set both colors (and exchange them)
    let x = s:HtmlColor(synIDattr(a:id, "bg#", s:whatterm))
    let a = a . "color: " . ( x != "" ? x : s:bgc ) . "; "
    let x = s:HtmlColor(synIDattr(a:id, "fg#", s:whatterm))
    let a = a . "background-color: " . ( x != "" ? x : s:fgc ) . "; "
  else
    let x = s:HtmlColor(synIDattr(a:id, "fg#", s:whatterm))
    if x != "" | let a = a . "color: " . x . "; " | endif
    let x = s:HtmlColor(synIDattr(a:id, "bg#", s:whatterm))
    if x != "" | let a = a . "background-color: " . x . "; " | endif
  endif
  if synIDattr(a:id, "bold") | let a = a . "font-weight: bold; " | endif
  if synIDattr(a:id, "italic") | let a = a . "font-style: italic; " | endif
  if synIDattr(a:id, "underline") | let a = a . "text-decoration: underline; " | endif
  return a
endfun

if s:settings.dynamic_folds
  " compares two folds as stored in our list of folds
  " A fold is "less" than another if it starts at an earlier line number,
  " or ends at a later line number, ties broken by fold level
  function! s:FoldCompare(f1, f2)
    if a:f1.firstline != a:f2.firstline
      " put it before if it starts earlier
      return a:f1.firstline - a:f2.firstline
    elseif a:f1.lastline != a:f2.lastline
      " put it before if it ends later
      return a:f2.lastline - a:f1.lastline
    else
      " if folds begin and end on the same lines, put lowest fold level first
      return a:f1.level - a:f2.level
    endif
  endfunction

endif


" Set some options to make it work faster.
" Don't report changes for :substitute, there will be many of them.
" Don't change other windows; turn off scroll bind temporarily
let s:old_title = &title
let s:old_icon = &icon
let s:old_et = &l:et
let s:old_bind = &l:scrollbind
let s:old_report = &report
let s:old_search = @/
let s:old_more = &more
set notitle noicon
setlocal et
set nomore
set report=1000000
setlocal noscrollbind

if exists(':ownsyntax') && exists('w:current_syntax')
  let s:current_syntax = w:current_syntax
elseif exists('b:current_syntax')
  let s:current_syntax = b:current_syntax
else
  let s:current_syntax = 'none'
endif

if s:current_syntax == ''
  let s:current_syntax = 'none'
endif

" Split window to create a buffer with the HTML file.
let s:orgbufnr = winbufnr(0)
let s:origwin_stl = &l:stl
if expand("%") == ""
  exec 'new Untitled.'.(s:settings.use_xhtml ? 'x' : '').'html'
else
  exec 'new %.'.(s:settings.use_xhtml ? 'x' : '').'html'
endif

" Resize the new window to very small in order to make it draw faster
let s:old_winheight = winheight(0)
let s:old_winfixheight = &l:winfixheight
if s:old_winheight > 2
  resize 1 " leave enough room to view one line at a time
  norm! G
  norm! zt
endif
setlocal winfixheight

let s:newwin_stl = &l:stl

" on the new window, set the least time-consuming fold method
let s:old_fdm = &foldmethod
let s:old_fen = &foldenable
setlocal foldmethod=manual
setlocal nofoldenable

let s:newwin = winnr()
let s:orgwin = bufwinnr(s:orgbufnr)

setlocal modifiable
%d
let s:old_paste = &paste
set paste
let s:old_magic = &magic
set magic

" set the fileencoding to match the charset we'll be using
let &l:fileencoding=s:settings.vim_encoding

" According to http://www.w3.org/TR/html4/charset.html#doc-char-set, the byte
" order mark is highly recommend on the web when using multibyte encodings. But,
" it is not a good idea to include it on UTF-8 files. Otherwise, let Vim
" determine when it is actually inserted.
if s:settings.vim_encoding == 'utf-8'
  setlocal nobomb
else
  setlocal bomb
endif

let s:lines = []

if s:settings.use_xhtml
  if s:settings.encoding != ""
    call add(s:lines, "<?xml version=\"1.0\" encoding=\"" . s:settings.encoding . "\"?>")
  else
    call add(s:lines, "<?xml version=\"1.0\"?>")
  endif
  let s:tag_close = ' />'
else
  let s:tag_close = '>'
endif

let s:HtmlSpace = ' '
let s:LeadingSpace = ' '
let s:HtmlEndline = ''
if s:settings.no_pre
  let s:HtmlEndline = '<br' . s:tag_close
  let s:LeadingSpace = '&nbsp;'
  let s:HtmlSpace = '\' . s:LeadingSpace
endif

" HTML header, with the title and generator ;-). Left free space for the CSS,
" to be filled at the end.
call extend(s:lines, [
      \ "<html>",
      \ "<head>"])
" include encoding as close to the top as possible, but only if not already
" contained in XML information (to avoid haggling over content type)
if s:settings.encoding != "" && !s:settings.use_xhtml
  call add(s:lines, "<meta http-equiv=\"content-type\" content=\"text/html; charset=" . s:settings.encoding . '"' . s:tag_close)
endif
call extend(s:lines, [
      \ ("<title>".expand("%:p:~")."</title>"),
      \ ("<meta name=\"Generator\" content=\"Vim/".v:version/100.".".v:version%100.'"'.s:tag_close),
      \ ("<meta name=\"plugin-version\" content=\"".g:loaded_2html_plugin.'"'.s:tag_close)
      \ ])
call add(s:lines, '<meta name="syntax" content="'.s:current_syntax.'"'.s:tag_close)
call add(s:lines, '<meta name="settings" content="'.
      \ join(filter(keys(s:settings),'s:settings[v:val]'),',').
      \ '"'.s:tag_close)

if s:settings.use_css
  if s:settings.dynamic_folds
    if s:settings.hover_unfold
      " if we are doing hover_unfold, use css 2 with css 1 fallback for IE6
      call extend(s:lines, [
	    \ "<style type=\"text/css\">",
	    \ s:settings.use_xhtml ? "" : "<!--",
	    \ ".FoldColumn { text-decoration: none; white-space: pre; }",
	    \ "",
	    \ "body * { margin: 0; padding: 0; }", "",
	    \ ".open-fold   > .Folded { display: none;  }",
	    \ ".open-fold   > .fulltext { display: inline; }",
	    \ ".closed-fold > .fulltext { display: none;  }",
	    \ ".closed-fold > .Folded { display: inline; }",
	    \ "",
	    \ ".open-fold   > .toggle-open   { display: none;   }",
	    \ ".open-fold   > .toggle-closed { display: inline; }",
	    \ ".closed-fold > .toggle-open   { display: inline; }",
	    \ ".closed-fold > .toggle-closed { display: none;   }",
	    \ "", "",
	    \ '/* opening a fold while hovering won''t be supported by IE6 and other',
	    \ "similar browsers, but it should fail gracefully. */",
	    \ ".closed-fold:hover > .fulltext { display: inline; }",
	    \ ".closed-fold:hover > .toggle-filler { display: none; }",
	    \ ".closed-fold:hover > .Folded { display: none; }",
	    \ s:settings.use_xhtml ? "" : '-->',
	    \ '</style>'])
      " TODO: IE7 doesn't *actually* support XHTML, maybe we should remove this.
      " But if it's served up as tag soup, maybe the following will work, so
      " leave it in for now.
      call extend(s:lines, [
	    \ "<!--[if lt IE 7]><style type=\"text/css\">",
	    \ ".open-fold   .Folded      { display: none; }",
	    \ ".open-fold   .fulltext      { display: inline; }",
	    \ ".open-fold   .toggle-open   { display: none; }",
	    \ ".closed-fold .toggle-closed { display: inline; }",
	    \ "",
	    \ ".closed-fold .fulltext      { display: none; }",
	    \ ".closed-fold .Folded      { display: inline; }",
	    \ ".closed-fold .toggle-open   { display: inline; }",
	    \ ".closed-fold .toggle-closed { display: none; }",
	    \ "</style>",
	    \ "<![endif]-->",
	    \])
    else
      " if we aren't doing hover_unfold, use CSS 1 only
      call extend(s:lines, [
	    \ "<style type=\"text/css\">",
	    \ s:settings.use_xhtml ? "" :"<!--",
	    \ ".FoldColumn { text-decoration: none; white-space: pre; }",
	    \ ".open-fold   .Folded      { display: none; }",
	    \ ".open-fold   .fulltext      { display: inline; }",
	    \ ".open-fold   .toggle-open   { display: none; }",
	    \ ".closed-fold .toggle-closed { display: inline; }",
	    \ "",
	    \ ".closed-fold .fulltext      { display: none; }",
	    \ ".closed-fold .Folded      { display: inline; }",
	    \ ".closed-fold .toggle-open   { display: inline; }",
	    \ ".closed-fold .toggle-closed { display: none; }",
	    \ s:settings.use_xhtml ? "" : '-->',
	    \ '</style>'
	    \])
    endif
  else
    " if we aren't doing any dynamic folding, no need for any special rules
    call extend(s:lines, [
	  \ "<style type=\"text/css\">",
	  \ s:settings.use_xhtml ? "" : "<!--",
	  \ s:settings.use_xhtml ? "" : '-->',
	  \ "</style>",
	  \])
  endif
endif

" insert javascript to toggle folds open and closed
if s:settings.dynamic_folds
  call extend(s:lines, [
	\ "",
	\ "<script type='text/javascript'>",
	\ s:settings.use_xhtml ? '//<![CDATA[' : "<!--",
	\ "function toggleFold(objID)",
	\ "{",
	\ "  var fold;",
	\ "  fold = document.getElementById(objID);",
	\ "  if(fold.className == 'closed-fold')",
	\ "  {",
	\ "    fold.className = 'open-fold';",
	\ "  }",
	\ "  else if (fold.className == 'open-fold')",
	\ "  {",
	\ "    fold.className = 'closed-fold';",
	\ "  }",
	\ "}",
	\ s:settings.use_xhtml ? '//]]>' : '-->',
	\ "</script>"
	\])
endif

if s:settings.no_pre
  call extend(s:lines, ["</head>", "<body>"])
else
  call extend(s:lines, ["</head>", "<body>", "<pre>"])
endif

exe s:orgwin . "wincmd w"

" List of all id's
let s:idlist = []

" set up progress bar in the status line
if !s:settings.no_progress
  " ProgressBar Indicator
  let s:progressbar={}

  " Progessbar specific functions
  func! s:ProgressBar(title, max_value, winnr)
    let pgb=copy(s:progressbar)
    let pgb.title = a:title.' '
    let pgb.max_value = a:max_value
    let pgb.winnr = a:winnr
    let pgb.cur_value = 0
    let pgb.items = { 'title'   : { 'color' : 'Statusline' },
	  \'bar'     : { 'color' : 'Statusline' , 'fillcolor' : 'DiffDelete' , 'bg' : 'Statusline' } ,
	  \'counter' : { 'color' : 'Statusline' } }
    let pgb.last_value = 0
    let pgb.needs_redraw = 0
    " Note that you must use len(split) instead of len() if you want to use 
    " unicode in title.
    "
    " Subtract 3 for spacing around the title.
    " Subtract 4 for the percentage display.
    " Subtract 2 for spacing before this.
    " Subtract 2 more for the '|' on either side of the progress bar
    let pgb.subtractedlen=len(split(pgb.title, '\zs'))+3+4+2+2
    let pgb.max_len = 0
    set laststatus=2
    return pgb
  endfun

  " Function: progressbar.calculate_ticks() {{{1
  func! s:progressbar.calculate_ticks(pb_len)
    if a:pb_len<=0
      let pb_len = 100
    else
      let pb_len = a:pb_len
    endif
    let self.progress_ticks = map(range(pb_len+1), "v:val * self.max_value / pb_len")
  endfun

  "Function: progressbar.paint()
  func! s:progressbar.paint()
    " Recalculate widths.
    let max_len = winwidth(self.winnr)
    let pb_len = 0
    " always true on first call because of initial value of self.max_len
    if max_len != self.max_len
      let self.max_len = max_len

      " Progressbar length
      let pb_len = max_len - self.subtractedlen

      call self.calculate_ticks(pb_len)

      let self.needs_redraw = 1
      let cur_value = 0
      let self.pb_len = pb_len
    else
      " start searching at the last found index to make the search for the
      " appropriate tick value normally take 0 or 1 comparisons
      let cur_value = self.last_value
      let pb_len = self.pb_len
    endif

    let cur_val_max = pb_len > 0 ? pb_len : 100

    " find the current progress bar position based on precalculated thresholds
    while cur_value < cur_val_max && self.cur_value > self.progress_ticks[cur_value]
      let cur_value += 1
    endwhile

    " update progress bar
    if self.last_value != cur_value || self.needs_redraw || self.cur_value == self.max_value
      let self.needs_redraw = 1
      let self.last_value = cur_value

      let t_color  = self.items.title.color
      let b_fcolor = self.items.bar.fillcolor
      let b_color  = self.items.bar.color
      let c_color  = self.items.counter.color

      let stl =  "%#".t_color."#%-( ".self.title." %)".
	    \"%#".b_color."#".
	    \(pb_len>0 ?
	    \	('|%#'.b_fcolor."#%-(".repeat(" ",cur_value)."%)".
	    \	 '%#'.b_color."#".repeat(" ",pb_len-cur_value)."|"):
	    \	('')).
	    \"%=%#".c_color."#%( ".printf("%3.d ",100*self.cur_value/self.max_value)."%% %)"
      call setwinvar(self.winnr, '&stl', stl)
    endif
  endfun

  func! s:progressbar.incr( ... )
    let self.cur_value += (a:0 ? a:1 : 1)
    " if we were making a general-purpose progress bar, we'd need to limit to a
    " lower limit as well, but since we always increment with a positive value
    " in this script, we only need limit the upper value
    let self.cur_value = (self.cur_value > self.max_value ? self.max_value : self.cur_value)
    call self.paint()
  endfun
  " }}}
  if s:settings.dynamic_folds
    " to process folds we make two passes through each line
    let s:pgb = s:ProgressBar("Processing folds:", line('$')*2, s:orgwin)
  endif
endif

" First do some preprocessing for dynamic folding. Do this for the entire file
" so we don't accidentally start within a closed fold or something.
let s:allfolds = []

if s:settings.dynamic_folds
  let s:lnum = 1
  let s:end = line('$')
  " save the fold text and set it to the default so we can find fold levels
  let s:foldtext_save = &foldtext
  setlocal foldtext&

  " we will set the foldcolumn in the html to the greater of the maximum fold
  " level and the current foldcolumn setting
  let s:foldcolumn = &foldcolumn

  " get all info needed to describe currently closed folds
  while s:lnum <= s:end
    if foldclosed(s:lnum) == s:lnum
      " default fold text has '+-' and then a number of dashes equal to fold
      " level, so subtract 2 from index of first non-dash after the dashes
      " in order to get the fold level of the current fold
      let s:level = match(foldtextresult(s:lnum), '+-*\zs[^-]') - 2
      " store fold info for later use
      let s:newfold = {'firstline': s:lnum, 'lastline': foldclosedend(s:lnum), 'level': s:level,'type': "closed-fold"}
      call add(s:allfolds, s:newfold)
      " open the fold so we can find any contained folds
      execute s:lnum."foldopen"
    else
      if !s:settings.no_progress
	call s:pgb.incr()
	if s:pgb.needs_redraw
	  redrawstatus
	  let s:pgb.needs_redraw = 0
	endif
      endif
      let s:lnum = s:lnum + 1
    endif
  endwhile

  " close all folds to get info for originally open folds
  silent! %foldclose!
  let s:lnum = 1

  " the originally open folds will be all folds we encounter that aren't
  " already in the list of closed folds
  while s:lnum <= s:end
    if foldclosed(s:lnum) == s:lnum
      " default fold text has '+-' and then a number of dashes equal to fold
      " level, so subtract 2 from index of first non-dash after the dashes
      " in order to get the fold level of the current fold
      let s:level = match(foldtextresult(s:lnum), '+-*\zs[^-]') - 2
      let s:newfold = {'firstline': s:lnum, 'lastline': foldclosedend(s:lnum), 'level': s:level,'type': "closed-fold"}
      " only add the fold if we don't already have it
      if empty(s:allfolds) || index(s:allfolds, s:newfold) == -1
	let s:newfold.type = "open-fold"
	call add(s:allfolds, s:newfold)
      endif
      " open the fold so we can find any contained folds
      execute s:lnum."foldopen"
    else
      if !s:settings.no_progress
	call s:pgb.incr()
	if s:pgb.needs_redraw
	  redrawstatus
	  let s:pgb.needs_redraw = 0
	endif
      endif
      let s:lnum = s:lnum + 1
    endif
  endwhile

  " sort the folds so that we only ever need to look at the first item in the
  " list of folds
  call sort(s:allfolds, "s:FoldCompare")

  let &l:foldtext = s:foldtext_save
  unlet s:foldtext_save

  " close all folds again so we can get the fold text as we go
  silent! %foldclose!

  for afold in s:allfolds
    let removed = 0
    if exists("g:html_start_line") && exists("g:html_end_line")
      if afold.firstline < g:html_start_line
	if afold.lastline < g:html_end_line && afold.lastline > g:html_start_line
	  " if a fold starts before the range to convert but stops within the
	  " range, we need to include it. Make it start on the first converted
	  " line.
	  let afold.firstline = g:html_start_line
	else
	  " if the fold lies outside the range or the start and stop enclose
	  " the entire range, don't bother parsing it
	  call remove(s:allfolds, index(s:allfolds, afold))
	  let removed = 1
	endif
      elseif afold.firstline > g:html_end_line
	" If the entire fold lies outside the range we need to remove it.
	call remove(s:allfolds, index(s:allfolds, afold))
	let removed = 1
      endif
    elseif exists("g:html_start_line")
      if afold.firstline < g:html_start_line
	" if there is no last line, but there is a first line, the end of the
	" fold will always lie within the region of interest, so keep it
	let afold.firstline = g:html_start_line
      endif
    elseif exists("g:html_end_line")
      " if there is no first line we default to the first line in the buffer so
      " the fold start will always be included if the fold itself is included.
      " If however the entire fold lies outside the range we need to remove it.
      if afold.firstline > g:html_end_line
	call remove(s:allfolds, index(s:allfolds, afold))
	let removed = 1
      endif
    endif
    if !removed
      if afold.level+1 > s:foldcolumn
	let s:foldcolumn = afold.level+1
      endif
    endif
  endfor
endif

" Now loop over all lines in the original text to convert to html.
" Use html_start_line and html_end_line if they are set.
if exists("g:html_start_line")
  let s:lnum = html_start_line
  if s:lnum < 1 || s:lnum > line("$")
    let s:lnum = 1
  endif
else
  let s:lnum = 1
endif
if exists("g:html_end_line")
  let s:end = html_end_line
  if s:end < s:lnum || s:end > line("$")
    let s:end = line("$")
  endif
else
  let s:end = line("$")
endif

" stack to keep track of all the folds containing the current line
let s:foldstack = []

if !s:settings.no_progress
  let s:pgb = s:ProgressBar("Processing lines:", s:end - s:lnum + 1, s:orgwin)
endif

if s:settings.number_lines
  let s:margin = strlen(s:end) + 1
else
  let s:margin = 0
endif

if has('folding') && !s:settings.ignore_folding
  let s:foldfillchar = &fillchars[matchend(&fillchars, 'fold:')]
  if s:foldfillchar == ''
    let s:foldfillchar = '-'
  endif
endif
let s:difffillchar = &fillchars[matchend(&fillchars, 'diff:')]
if s:difffillchar == ''
  let s:difffillchar = '-'
endif

let s:foldId = 0

if !s:settings.expand_tabs
  " If keeping tabs, add them to printable characters so we keep them when
  " formatting text (strtrans() doesn't replace printable chars)
  let s:old_isprint = &isprint
  setlocal isprint+=9
endif

while s:lnum <= s:end

  " If there are filler lines for diff mode, show these above the line.
  let s:filler = diff_filler(s:lnum)
  if s:filler > 0
    let s:n = s:filler
    while s:n > 0
      let s:new = repeat(s:difffillchar, 3)

      if s:n > 2 && s:n < s:filler && !s:settings.whole_filler
	let s:new = s:new . " " . s:filler . " inserted lines "
	let s:n = 2
      endif

      if !s:settings.no_pre
	" HTML line wrapping is off--go ahead and fill to the margin
	let s:new = s:new . repeat(s:difffillchar, &columns - strlen(s:new) - s:margin)
      else
	let s:new = s:new . repeat(s:difffillchar, 3)
      endif

      let s:new = s:HtmlFormat(s:new, "DiffDelete", "")
      if s:settings.number_lines
	" Indent if line numbering is on; must be after escaping.
	let s:new = repeat(s:LeadingSpace, s:margin) . s:new
      endif
      call add(s:lines, s:new.s:HtmlEndline)

      let s:n = s:n - 1
    endwhile
    unlet s:n
  endif
  unlet s:filler

  " Start the line with the line number.
  if s:settings.number_lines
    let s:numcol = repeat(' ', s:margin - 1 - strlen(s:lnum)) . s:lnum . ' '
  else
    let s:numcol = ""
  endif

  let s:new = ""

  if has('folding') && !s:settings.ignore_folding && foldclosed(s:lnum) > -1 && !s:settings.dynamic_folds
    "
    " This is the beginning of a folded block (with no dynamic folding)
    "
    let s:new = s:numcol . foldtextresult(s:lnum)
    if !s:settings.no_pre
      " HTML line wrapping is off--go ahead and fill to the margin
      let s:new = s:new . repeat(s:foldfillchar, &columns - strlen(s:new))
    endif

    let s:new = s:HtmlFormat(s:new, "Folded", "")

    " Skip to the end of the fold
    let s:new_lnum = foldclosedend(s:lnum)

    if !s:settings.no_progress
      call s:pgb.incr(s:new_lnum - s:lnum)
    endif

    let s:lnum = s:new_lnum

  else
    "
    " A line that is not folded, or doing dynamic folding.
    "
    let s:line = getline(s:lnum)
    let s:len = strlen(s:line)

    if s:settings.dynamic_folds
      " First insert a closing for any open folds that end on this line
      while !empty(s:foldstack) && get(s:foldstack,0).lastline == s:lnum-1
	let s:new = s:new."</span></span>"
	call remove(s:foldstack, 0)
      endwhile

      " Now insert an opening for any new folds that start on this line
      let s:firstfold = 1
      while !empty(s:allfolds) && get(s:allfolds,0).firstline == s:lnum
	let s:foldId = s:foldId + 1
	let s:new .= "<span id='"
	let s:new .= (exists('g:html_diff_win_num') ? "win".g:html_diff_win_num : "")
	let s:new .= "fold".s:foldId."' class='".s:allfolds[0].type."'>"


	" Unless disabled, add a fold column for the opening line of a fold.
	"
	" Note that dynamic folds require using css so we just use css to take
	" care of the leading spaces rather than using &nbsp; in the case of
	" html_no_pre to make it easier
	if !s:settings.no_foldcolumn
	  " add fold column that can open the new fold
	  if s:allfolds[0].level > 1 && s:firstfold
	    let s:new = s:new . "<a class='toggle-open FoldColumn' href='javascript:toggleFold(\"fold".s:foldstack[0].id."\")'>"
	    let s:new = s:new . repeat('|', s:allfolds[0].level - 1) . "</a>"
	  endif
	  let s:new = s:new . "<a class='toggle-open FoldColumn' href='javascript:toggleFold(\"fold".s:foldId."\")'>+</a>"
	  let s:new = s:new . "<a class='toggle-open "
	  " If this is not the last fold we're opening on this line, we need
	  " to keep the filler spaces hidden if the fold is opened by mouse
	  " hover. If it is the last fold to open in the line, we shouldn't hide
	  " them, so don't apply the toggle-filler class.
	  if get(s:allfolds, 1, {'firstline': 0}).firstline == s:lnum
	    let s:new = s:new . "toggle-filler "
	  endif
	  let s:new = s:new . "FoldColumn' href='javascript:toggleFold(\"fold".s:foldId."\")'>"
	  let s:new = s:new . repeat(" ", s:foldcolumn - s:allfolds[0].level) . "</a>"

	  " add fold column that can close the new fold
	  let s:new = s:new . "<a class='toggle-closed FoldColumn' href='javascript:toggleFold(\"fold".s:foldId."\")'>"
	  if s:firstfold
	    let s:new = s:new . repeat('|', s:allfolds[0].level - 1)
	  endif
	  let s:new = s:new . "-"
	  " only add spaces if we aren't opening another fold on the same line
	  if get(s:allfolds, 1, {'firstline': 0}).firstline != s:lnum
	    let s:new = s:new . repeat(" ", s:foldcolumn - s:allfolds[0].level)
	  endif
	  let s:new = s:new . "</a>"
	  let s:firstfold = 0
	endif

	" add fold text, moving the span ending to the next line so collapsing
	" of folds works correctly
	let s:new = s:new . substitute(s:HtmlFormat(s:numcol . foldtextresult(s:lnum), "Folded", ""), '</span>', s:HtmlEndline.'\n\0', '')
	let s:new = s:new . "<span class='fulltext'>"

	" open the fold now that we have the fold text to allow retrieval of
	" fold text for subsequent folds
	execute s:lnum."foldopen"
	call insert(s:foldstack, remove(s:allfolds,0))
	let s:foldstack[0].id = s:foldId
      endwhile

      " Unless disabled, add a fold column for other lines.
      "
      " Note that dynamic folds require using css so we just use css to take
      " care of the leading spaces rather than using &nbsp; in the case of
      " html_no_pre to make it easier
      if !s:settings.no_foldcolumn
	if empty(s:foldstack)
	  " add the empty foldcolumn for unfolded lines if there is a fold
	  " column at all
	  if s:foldcolumn > 0
	    let s:new = s:new . s:HtmlFormat(repeat(' ', s:foldcolumn), "FoldColumn", "")
	  endif
	else
	  " add the fold column for folds not on the opening line
	  if get(s:foldstack, 0).firstline < s:lnum
	    let s:new = s:new . "<a class='FoldColumn' href='javascript:toggleFold(\"fold".s:foldstack[0].id."\")'>"
	    let s:new = s:new . repeat('|', s:foldstack[0].level)
	    let s:new = s:new . repeat(' ', s:foldcolumn - s:foldstack[0].level) . "</a>"
	  endif
	endif
      endif
    endif

    " Now continue with the unfolded line text
    if s:settings.number_lines
      " TODO: why not use the real highlight name here?
      let s:new = s:new . s:HtmlFormat(s:numcol, "lnr", "")
    endif

    " Get the diff attribute, if any.
    let s:diffattr = diff_hlID(s:lnum, 1)

    " initialize conceal info to act like not concealed, just in case
    let s:concealinfo = [0, '']

    " Loop over each character in the line
    let s:col = 1

    " most of the time we won't use the diff_id, initialize to zero
    let s:diff_id = 0
    let s:diff_id_name = ""

    while s:col <= s:len || (s:col == 1 && s:diffattr)
      let s:startcol = s:col " The start column for processing text
      if !s:settings.ignore_conceal && has('conceal')
	let s:concealinfo = synconcealed(s:lnum, s:col)
      endif
      if !s:settings.ignore_conceal && s:concealinfo[0]
	let s:col = s:col + 1
	" Speed loop (it's small - that's the trick)
	" Go along till we find a change in the match sequence number (ending
	" the specific concealed region) or until there are no more concealed
	" characters.
	while s:col <= s:len && s:concealinfo == synconcealed(s:lnum, s:col) | let s:col = s:col + 1 | endwhile
      elseif s:diffattr
	let s:diff_id = diff_hlID(s:lnum, s:col)
	let s:id = synID(s:lnum, s:col, 1)
	let s:col = s:col + 1
	" Speed loop (it's small - that's the trick)
	" Go along till we find a change in hlID
	while s:col <= s:len && s:id == synID(s:lnum, s:col, 1)
	      \   && s:diff_id == diff_hlID(s:lnum, s:col) |
	      \     let s:col = s:col + 1 |
	      \ endwhile
	if s:len < &columns && !s:settings.no_pre
	  " Add spaces at the end of the raw text line to extend the changed
	  " line to the full width.
	  let s:line = s:line . repeat(' ', &columns - virtcol([s:lnum, s:len]) - s:margin)
	  let s:len = &columns
	endif
      else
	let s:id = synID(s:lnum, s:col, 1)
	let s:col = s:col + 1
	" Speed loop (it's small - that's the trick)
	" Go along till we find a change in synID
	while s:col <= s:len && s:id == synID(s:lnum, s:col, 1) | let s:col = s:col + 1 | endwhile
      endif

      if s:settings.ignore_conceal || !s:concealinfo[0]
	" Expand tabs if needed
	let s:expandedtab = strpart(s:line, s:startcol - 1, s:col - s:startcol)
	if s:settings.expand_tabs
	  let s:offset = 0
	  let s:idx = stridx(s:expandedtab, "\t")
	  while s:idx >= 0
	    if has("multi_byte_encoding")
	      if s:startcol + s:idx == 1
		let s:i = &ts
	      else
		if s:idx == 0
		  let s:prevc = matchstr(s:line, '.\%' . (s:startcol + s:idx + s:offset) . 'c')
		else
		  let s:prevc = matchstr(s:expandedtab, '.\%' . (s:idx + 1) . 'c')
		endif
		let s:vcol = virtcol([s:lnum, s:startcol + s:idx + s:offset - len(s:prevc)])
		let s:i = &ts - (s:vcol % &ts)
	      endif
	      let s:offset -= s:i - 1
	    else
	      let s:i = &ts - ((s:idx + s:startcol - 1) % &ts)
	    endif
	    let s:expandedtab = substitute(s:expandedtab, '\t', repeat(' ', s:i), '')
	    let s:idx = stridx(s:expandedtab, "\t")
	  endwhile
	end

	" get the highlight group name to use
	let s:id = synIDtrans(s:id)
	let s:id_name = synIDattr(s:id, "name", s:whatterm)
	if s:diff_id
	  let s:diff_id_name = synIDattr(s:diff_id, "name", s:whatterm)
	endif
      else
	" use Conceal highlighting for concealed text
	let s:id_name = 'Conceal'
	let s:expandedtab = s:concealinfo[1]
      endif

      " Output the text with the same synID, with class set to {s:id_name},
      " unless it has been concealed completely.
      if strlen(s:expandedtab) > 0
	let s:new = s:new . s:HtmlFormat(s:expandedtab,  s:id_name, s:diff_id_name)
      endif
    endwhile
  endif

  call extend(s:lines, split(s:new.s:HtmlEndline, '\n', 1))
  if !s:settings.no_progress && s:pgb.needs_redraw
    redrawstatus
    let s:pgb.needs_redraw = 0
  endif
  let s:lnum = s:lnum + 1

  if !s:settings.no_progress
    call s:pgb.incr()
  endif
endwhile

if s:settings.dynamic_folds
  " finish off any open folds
  while !empty(s:foldstack)
    let s:lines[-1].="</span></span>"
    call remove(s:foldstack, 0)
  endwhile

  " add fold column to the style list if not already there
  let s:id = hlID('FoldColumn')
  if index(s:idlist, s:id) == -1
    call insert(s:idlist, s:id)
  endif
endif

if s:settings.no_pre
  if !s:settings.use_css
    " Close off the font tag that encapsulates the whole <body>
    call extend(s:lines, ["</font>", "</body>", "</html>"])
  else
    call extend(s:lines, ["</body>", "</html>"])
  endif
else
  call extend(s:lines, ["</pre>", "</body>", "</html>"])
endif

exe s:newwin . "wincmd w"
call setline(1, s:lines)
unlet s:lines

" Now, when we finally know which, we define the colors and styles
if s:settings.use_css
  1;/<style type="text/+1
endif

" Find out the background and foreground color.
let s:fgc = s:HtmlColor(synIDattr(hlID("Normal"), "fg#", s:whatterm))
let s:bgc = s:HtmlColor(synIDattr(hlID("Normal"), "bg#", s:whatterm))
if s:fgc == ""
  let s:fgc = ( &background == "dark" ? "#ffffff" : "#000000" )
endif
if s:bgc == ""
  let s:bgc = ( &background == "dark" ? "#000000" : "#ffffff" )
endif

" Normal/global attributes
" For Netscape 4, set <body> attributes too, though, strictly speaking, it's
" incorrect.
if s:settings.use_css
  if s:settings.no_pre
    execute "normal! A\nbody { color: " . s:fgc . "; background-color: " . s:bgc . "; font-family: ". s:htmlfont ."; }\e"
  else
    execute "normal! A\npre { font-family: ". s:htmlfont ."; color: " . s:fgc . "; background-color: " . s:bgc . "; }\e"
    yank
    put
    execute "normal! ^cwbody\e"
  endif
else
  execute '%s:<body>:<body bgcolor="' . s:bgc . '" text="' . s:fgc . '">\r<font face="'. s:htmlfont .'">'
endif

" Line numbering attributes
if s:settings.number_lines
  if s:settings.use_css
    execute "normal! A\n.lnr { " . s:CSS1(hlID("LineNr")) . "}\e"
  else
    execute '%s+^<span class="lnr">\([^<]*\)</span>+' . s:HtmlOpening(hlID("LineNr")) . '\1' . s:HtmlClosing(hlID("LineNr")) . '+g'
  endif
endif

" Gather attributes for all other classes
if !s:settings.no_progress && !empty(s:idlist)
  let s:pgb = s:ProgressBar("Processing classes:", len(s:idlist),s:newwin)
endif
while !empty(s:idlist)
  let s:attr = ""
  let s:id = remove(s:idlist, 0)
  let s:attr = s:CSS1(s:id)
  let s:id_name = synIDattr(s:id, "name", s:whatterm)

  " If the class has some attributes, export the style, otherwise DELETE all
  " its occurences to make the HTML shorter
  if s:attr != ""
    if s:settings.use_css
      execute "normal! A\n." . s:id_name . " { " . s:attr . "}"
    else
      " replace spans of just this class name with non-CSS style markup
      execute '%s+<span class="' . s:id_name . '">\([^<]*\)</span>+' . s:HtmlOpening(s:id) . '\1' . s:HtmlClosing(s:id) . '+ge'
      " Replace spans of this class name AND a diff class with non-CSS style
      " markup surrounding a span of just the diff class. The diff class will
      " be handled later because we know that information is at the end.
      execute '%s+<span class="' . s:id_name . ' \(Diff\%(Add\|Change\|Delete\|Text\)\)">\([^<]*\)</span>+' . s:HtmlOpening(s:id) . '<span class="\1">\2</span>' . s:HtmlClosing(s:id) . '+ge'
    endif
  else
    execute '%s+<span class="' . s:id_name . '">\([^<]*\)</span>+\1+ge'
    execute '%s+<span class="' . s:id_name . ' \(Diff\%(Add\|Change\|Delete\|Text\)\)">\([^<]*\)</span>+<span class="\1">\2</span>+ge'
    if s:settings.use_css
      1;/<\/style>/-2
    endif
  endif

  if !s:settings.no_progress
    call s:pgb.incr()
    if s:pgb.needs_redraw
      redrawstatus
      let s:pgb.needs_redraw = 0
      " TODO: sleep here to show the progress bar, but only if total time spent
      " so far on this step is < 1 second? Too slow for batch runs like the test
      " suite to sleep all the time. Maybe there's no good reason to sleep at
      " all.
    endif
  endif
endwhile

" Add hyperlinks
%s+\(https\=://\S\{-}\)\(\([.,;:}]\=\(\s\|$\)\)\|[\\"'<>]\|&gt;\|&lt;\|&quot;\)+<a href="\1">\1</a>\2+ge

" The DTD
if s:settings.use_xhtml
  exe "normal! gg$a\n<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">"
elseif s:settings.use_css && !s:settings.no_pre
  exe "normal! gg0i<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01//EN\" \"http://www.w3.org/TR/html4/strict.dtd\">\n"
else
  exe "normal! gg0i<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\" \"http://www.w3.org/TR/html4/loose.dtd\">\n"
endif

if s:settings.use_xhtml
  exe "normal! gg/<html/e\na xmlns=\"http://www.w3.org/1999/xhtml\"\e"
endif

" Cleanup
%s:\s\+$::e

" Restore old settings (new window first)
let &l:foldenable = s:old_fen
let &l:foldmethod = s:old_fdm
let &report = s:old_report
let &title = s:old_title
let &icon = s:old_icon
let &paste = s:old_paste
let &magic = s:old_magic
let @/ = s:old_search
let &more = s:old_more

" switch to original window to restore those settings
exe s:orgwin . "wincmd w"

if !s:settings.expand_tabs
  let &l:isprint = s:old_isprint
endif
let &l:stl = s:origwin_stl
let &l:et = s:old_et
let &l:scrollbind = s:old_bind

" and back to the new window again to end there
exe s:newwin . "wincmd w"

let &l:stl = s:newwin_stl
exec 'resize' s:old_winheight
let &l:winfixheight = s:old_winfixheight

let &ls=s:ls

" Save a little bit of memory (worth doing?)
unlet s:htmlfont
unlet s:old_et s:old_paste s:old_icon s:old_report s:old_title s:old_search
unlet s:old_magic s:old_more s:old_fdm s:old_fen s:old_winheight
unlet! s:old_isprint
unlet s:whatterm s:idlist s:lnum s:end s:margin s:fgc s:bgc s:old_winfixheight
unlet! s:col s:id s:attr s:len s:line s:new s:expandedtab s:concealinfo
unlet! s:orgwin s:newwin s:orgbufnr s:idx s:i s:offset s:ls s:origwin_stl
unlet! s:newwin_stl s:current_syntax
if !v:profiling
  delfunc s:HtmlColor
  delfunc s:HtmlFormat
  delfunc s:CSS1
  if !s:settings.use_css
    delfunc s:HtmlOpening
    delfunc s:HtmlClosing
  endif
  if s:settings.dynamic_folds
    delfunc s:FoldCompare
  endif

  if !s:settings.no_progress
    delfunc s:ProgressBar
    delfunc s:progressbar.paint
    delfunc s:progressbar.incr
    unlet s:pgb s:progressbar
  endif
endif

unlet! s:new_lnum s:diffattr s:difffillchar s:foldfillchar s:HtmlSpace
unlet! s:LeadingSpace s:HtmlEndline s:firstfold s:foldcolumn
unlet s:foldstack s:allfolds s:foldId s:numcol s:settings

let &cpo = s:cpo_sav
unlet! s:cpo_sav

" Make sure any patches will probably use consistent indent
"   vim: ts=8 sw=2 sts=2 noet
