" Vim autoload file for the tohtml plugin.
" Maintainer: Ben Fritz <fritzophrenic@gmail.com>
" Last Change: 2010 Aug 12
"
" Additional contributors:
"
"	      Original by Bram Moolenaar <Bram@vim.org>
"	      Diff2HTML() added by Christian Brabandt <cb@256bit.org>
"
"	      See Mercurial change logs for more!

" this file uses line continuations
let s:cpo_sav = &cpo
set cpo-=C

func! tohtml#Convert2HTML(line1, line2)
  let s:settings = tohtml#GetUserSettings()

  if !&diff || s:settings.diff_one_file
    if a:line2 >= a:line1
      let g:html_start_line = a:line1
      let g:html_end_line = a:line2
    else
      let g:html_start_line = a:line2
      let g:html_end_line = a:line1
    endif
    runtime syntax/2html.vim
  else
    let win_list = []
    let buf_list = []
    windo | if &diff | call add(win_list, winbufnr(0)) | endif
    let s:settings.whole_filler = 1
    let g:html_diff_win_num = 0
    for window in win_list
      exe ":" . bufwinnr(window) . "wincmd w"
      let g:html_start_line = 1
      let g:html_end_line = line('$')
      let g:html_diff_win_num += 1
      runtime syntax/2html.vim
      call add(buf_list, bufnr('%'))
    endfor
    unlet g:html_diff_win_num
    call tohtml#Diff2HTML(win_list, buf_list)
  endif

  unlet g:html_start_line
  unlet g:html_end_line
  unlet s:settings
endfunc

func! tohtml#Diff2HTML(win_list, buf_list)
  let xml_line = ""
  let tag_close = '>'

  let s:old_paste = &paste
  set paste
  let s:old_magic = &magic
  set magic

  if s:settings.use_xhtml
    if s:settings.encoding != ""
      let xml_line = "<?xml version=\"1.0\" encoding=\"" . s:settings.encoding . "\"?>"
    else
      let xml_line = "<?xml version=\"1.0\"?>"
    endif
    let tag_close = ' />'
  endif

  let style = [s:settings.use_xhtml ? "" : '-->']
  let body_line = ''

  let html = []
  if s:settings.use_xhtml
    call add(html, xml_line)
  endif
  if s:settings.use_xhtml
    call add(html, "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">")
    call add(html, '<html xmlns="http://www.w3.org/1999/xhtml">')
  elseif s:settings.use_css && !s:settings.no_pre
    call add(html, "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01//EN\" \"http://www.w3.org/TR/html4/strict.dtd\">")
    call add(html, '<html>')
  else
    call add(html, '<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN"')
    call add(html, '  "http://www.w3.org/TR/html4/loose.dtd">')
    call add(html, '<html>')
  endif
  call add(html, '<head>')

  " include encoding as close to the top as possible, but only if not already
  " contained in XML information (to avoid haggling over content type)
  if s:settings.encoding != "" && !s:settings.use_xhtml
    call add(html, "<meta http-equiv=\"content-type\" content=\"text/html; charset=" . s:settings.encoding . '"' . tag_close)
  endif

  call add(html, '<title>diff</title>')
  call add(html, '<meta name="Generator" content="Vim/'.v:version/100.'.'.v:version%100.'"'.tag_close)
  call add(html, '<meta name="plugin-version" content="'.g:loaded_2html_plugin.'"'.tag_close)
  call add(html, '<meta name="settings" content="'.
	\ join(filter(keys(s:settings),'s:settings[v:val]'),',').
	\ '"'.tag_close)

  call add(html, '</head>')
  let body_line_num = len(html)
  call add(html, '<body>')
  call add(html, '<table border="1" width="100%">')

  call add(html, '<tr>')
  for buf in a:win_list
    call add(html, '<th>'.bufname(buf).'</th>')
  endfor
  call add(html, '</tr><tr>')

  let diff_style_start = 0
  let insert_index = 0

  for buf in a:buf_list
    let temp = []
    exe bufwinnr(buf) . 'wincmd w'

    " If text is folded because of user foldmethod settings, etc. we don't want
    " to act on everything in a fold by mistake.
    setlocal nofoldenable

    " When not using CSS or when using xhtml, the <body> line can be important.
    " Assume it will be the same for all buffers and grab it from the first
    " buffer. Similarly, need to grab the body end line as well.
    if body_line == ''
      1
      call search('<body')
      let body_line = getline('.')
      $
      call search('</body>', 'b')
      let s:body_end_line = getline('.')
    endif

    " Grab the style information.  Some of this will be duplicated...
    1
    let style_start = search('^<style type="text/css">')
    1
    let style_end = search('^</style>')
    if style_start > 0 && style_end > 0
      let buf_styles = getline(style_start + 1, style_end - 1)
      for a_style in buf_styles
	if index(style, a_style) == -1
	  if diff_style_start == 0
	    if a_style =~ '\<Diff\(Change\|Text\|Add\|Delete\)'
	      let diff_style_start = len(style)-1
	    endif
	  endif
	  call insert(style, a_style, insert_index)
	  let insert_index += 1
	endif
      endfor
    endif

    if diff_style_start != 0
      let insert_index = diff_style_start
    endif

    " Delete those parts that are not needed so
    " we can include the rest into the resulting table
    1,/^<body/d_
    $
    ?</body>?,$d_
    let temp = getline(1,'$')
    " undo deletion of start and end part
    " so we can later save the file as valid html
    " TODO: restore using grabbed lines if undolevel is 1?
    normal 2u
    if s:settings.use_css
      call add(html, '<td valign="top"><div>')
    elseif s:settings.use_xhtml
      call add(html, '<td nowrap="nowrap" valign="top"><div>')
    else
      call add(html, '<td nowrap valign="top"><div>')
    endif
    let html += temp
    call add(html, '</div></td>')

    " Close this buffer
    " TODO: the comment above says we're going to allow saving the file
    " later...but here we discard it?
    quit!
  endfor

  let html[body_line_num] = body_line

  call add(html, '</tr>')
  call add(html, '</table>')
  call add(html, s:body_end_line)
  call add(html, '</html>')

  let i = 1
  let name = "Diff" . (s:settings.use_xhtml ? ".xhtml" : ".html")
  " Find an unused file name if current file name is already in use
  while filereadable(name)
    let name = substitute(name, '\d*\.x\?html$', '', '') . i . '.' . fnamemodify(copy(name), ":t:e")
    let i += 1
  endwhile
  exe "topleft new " . name
  setlocal modifiable

  " just in case some user autocmd creates content in the new buffer, make sure
  " it is empty before proceeding
  %d
  call append(0, html)

  if len(style) > 0
    1
    let style_start = search('^</head>')-1

    " Insert javascript to toggle matching folds open and closed in all windows,
    " if dynamic folding is active.
    if s:settings.dynamic_folds
      call append(style_start, [
	    \  "<script type='text/javascript'>",
	    \  s:settings.use_xhtml ? '//<![CDATA[' : "  <!--",
	    \  "  function toggleFold(objID)",
	    \  "  {",
	    \  "    for (win_num = 1; win_num <= ".len(a:buf_list)."; win_num++)",
	    \  "    {",
	    \  "      var fold;",
	    \  '      fold = document.getElementById("win"+win_num+objID);',
	    \  "      if(fold.className == 'closed-fold')",
	    \  "      {",
	    \  "        fold.className = 'open-fold';",
	    \  "      }",
	    \  "      else if (fold.className == 'open-fold')",
	    \  "      {",
	    \  "        fold.className = 'closed-fold';",
	    \  "      }",
	    \  "    }",
	    \  "  }",
	    \  s:settings.use_xhtml ? '//]]>' : "  -->",
	    \  "</script>"
	    \ ])
    endif

    " Insert styles from all the generated html documents and additional styles
    " for the table-based layout of the side-by-side diff. The diff should take
    " up the full browser window (but not more), and be static in size,
    " horizontally scrollable when the lines are too long. Otherwise, the diff
    " is pretty useless for really long lines.
    if s:settings.use_css
      call append(style_start,
	    \ ['<style type="text/css">']+
	    \ style+
	    \ [ s:settings.use_xhtml ? '' : '<!--',
	    \   'table { table-layout: fixed; }',
	    \   'html, body, table, tbody { width: 100%; margin: 0; padding: 0; }',
	    \   'th, td { width: '.printf("%.1f",100.0/len(a:win_list)).'%; }',
	    \   'td div { overflow: auto; }',
	    \   s:settings.use_xhtml ? '' : '-->',
	    \   '</style>'
	    \ ])
    endif
  endif

  let &paste = s:old_paste
  let &magic = s:old_magic
endfunc

" Gets a single user option and sets it in the passed-in Dict, or gives it the
" default value if the option doesn't actually exist.
func! tohtml#GetOption(settings, option, default)
  if exists('g:html_'.a:option)
    let a:settings[a:option] = g:html_{a:option}
  else
    let a:settings[a:option] = a:default
  endif
endfunc

" returns a Dict containing the values of all user options for 2html, including
" default values for those not given an explicit value by the user. Discards the
" html_ prefix of the option for nicer looking code.
func! tohtml#GetUserSettings()
  if exists('s:settings')
    " just restore the known options if we've already retrieved them
    return s:settings
  else
    " otherwise figure out which options are set
    let user_settings = {}

    " Define the correct option if the old option name exists and we haven't
    " already defined the correct one. Maybe I'll put out a warnig message about
    " this sometime and remove the old option entirely at some even later time,
    " but for now just silently accept the old option.
    if exists('g:use_xhtml') && !exists("g:html_use_xhtml")
      let g:html_use_xhtml = g:use_xhtml
    endif

    " get current option settings with appropriate defaults
    call tohtml#GetOption(user_settings,    'no_progress',  !has("statusline") )
    call tohtml#GetOption(user_settings,  'diff_one_file',  0 )
    call tohtml#GetOption(user_settings,   'number_lines',  &number )
    call tohtml#GetOption(user_settings,        'use_css',  1 )
    call tohtml#GetOption(user_settings, 'ignore_conceal',  0 )
    call tohtml#GetOption(user_settings, 'ignore_folding',  0 )
    call tohtml#GetOption(user_settings,  'dynamic_folds',  0 )
    call tohtml#GetOption(user_settings,  'no_foldcolumn',  0 )
    call tohtml#GetOption(user_settings,   'hover_unfold',  0 )
    call tohtml#GetOption(user_settings,         'no_pre',  0 )
    call tohtml#GetOption(user_settings,   'whole_filler',  0 )
    call tohtml#GetOption(user_settings,      'use_xhtml',  0 )
    
    " override those settings that need it

    " hover opening implies dynamic folding
    if user_settings.hover_unfold
      let user_settings.dynamic_folds = 1
    endif

    " ignore folding overrides dynamic folding
    if user_settings.ignore_folding && user_settings.dynamic_folds
      let user_settings.dynamic_folds = 0
      let user_settings.hover_unfold = 0
    endif

    " dynamic folding with no foldcolumn implies hover opens
    if user_settings.dynamic_folds && user_settings.no_foldcolumn
      let user_settings.hover_unfold = 1
    endif

    " dynamic folding implies css
    if user_settings.dynamic_folds
      let user_settings.use_css = 1
    endif

    " if we're not using CSS we cannot use a pre section because <font> tags
    " aren't allowed inside a <pre> block
    if !user_settings.use_css
      let user_settings.no_pre = 1
    endif

    " Figure out proper MIME charset from the 'encoding' option.
    if exists("g:html_use_encoding")
      let user_settings.encoding = g:html_use_encoding
    else
      let vim_encoding = &encoding
      if vim_encoding =~ '^8bit\|^2byte'
	let vim_encoding = substitute(vim_encoding, '^8bit-\|^2byte-', '', '')
      endif
      if vim_encoding == 'latin1'
	let user_settings.encoding = 'iso-8859-1'
      elseif vim_encoding =~ "^cp12"
	let user_settings.encoding = substitute(vim_encoding, 'cp', 'windows-', '')
      elseif vim_encoding == 'sjis' || vim_encoding == 'cp932'
	let user_settings.encoding = 'Shift_JIS'
      elseif vim_encoding == 'big5' || vim_encoding == 'cp950'
	let user_settings.encoding = "Big5"
      elseif vim_encoding == 'euc-cn'
	let user_settings.encoding = 'GB_2312-80'
      elseif vim_encoding == 'euc-tw'
	let user_settings.encoding = ""
      elseif vim_encoding =~ '^euc\|^iso\|^koi'
	let user_settings.encoding = substitute(vim_encoding, '.*', '\U\0', '')
      elseif vim_encoding == 'cp949'
	let user_settings.encoding = 'KS_C_5601-1987'
      elseif vim_encoding == 'cp936'
	let user_settings.encoding = 'GBK'
      elseif vim_encoding =~ '^ucs\|^utf'
	let user_settings.encoding = 'UTF-8'
      else
	let user_settings.encoding = ""
      endif
    endif

    " TODO: font

    return user_settings
  endif
endfunc

let &cpo = s:cpo_sav
unlet s:cpo_sav

" Make sure any patches will probably use consistent indent
"   vim: ts=8 sw=2 sts=2 noet
