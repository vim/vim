" Vim autoload file for the tohtml plugin.
" Maintainer: Bram Moolenaar <Bram@vim.org>
" Last Change: 2010 Jul 11
"
" Diff2HTML() added by Christian Brabandt <cb@256bit.org>

func! tohtml#Convert2HTML(line1, line2)
  if !&diff || exists("g:diff_one_file")
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
    windo | if (&diff) | call add(win_list, winbufnr(0)) | endif
    let save_hwf = exists("g:html_whole_filler")
    let g:html_whole_filler = 1
    for window in win_list
      exe ":" . bufwinnr(window) . "wincmd w"
      let g:html_start_line = 1
      let g:html_end_line = line('$')
      runtime syntax/2html.vim
      call add(buf_list, bufnr('%'))
    endfor
    if !save_hwf
      unlet g:html_whole_filler
    endif
    call tohtml#Diff2HTML(win_list, buf_list)
  endif

  unlet g:html_start_line
  unlet g:html_end_line
endfunc

func! tohtml#Diff2HTML(win_list, buf_list)
  let style = []
  let html = []
  call add(html, '<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN"')
  call add(html, '  "http://www.w3.org/TR/html4/loose.dtd">')
  call add(html, '<html>')
  call add(html, '<head>')
  call add(html, '<title>diff</title>')
  call add(html, '<meta name="Generator" content="Vim/7.3">')
  "call add(html, '<meta http-equiv="content-type" content="text/html; charset=iso-8859-1">')
  call add(html, '</head>')
  call add(html, '<body>')
  call add(html, '<table border="1" width="100%">')
  "call add(html, '<font face="monospace">')
  call add(html, '<tr>')
  for buf in a:win_list
    call add(html, '<th>'.bufname(buf).'</th>')
  endfor
  call add(html, '</tr><tr>')

  for buf in a:buf_list
    let temp = []
    exe bufwinnr(buf) . 'wincmd w'

    " Grab the style information.  Some of this will be duplicated...
    1
    let style_start = search('^<style type="text/css">')
    1
    let style_end = search('^</style>')
    if style_start > 0 && style_end > 0
      let style += getline(style_start + 1, style_end - 1)
    endif

    " Delete those parts that are not needed so
    " we can include the rest into the resulting table
    1,/^<body/d_
    $
    ?</body>?,$d_
    let temp = getline(1,'$')
    " undo deletion of start and end part
    " so we can later save the file as valid html
    normal 2u
    call add(html, '<td nowrap valign="top">')
    let html += temp
    call add(html, '</td>')

    " Close this buffer
    quit!
  endfor

  call add(html, '</tr>')
  call add(html, '</table>')
  call add(html, '</body>')
  call add(html, '</html>')

  let i = 1
  let name = "Diff" . ".html"
  while filereadable(name)
    let name = substitute(name, '\d*\.html$', '', '') . i . ".html"
    let i += 1
  endw
  exe "new " . name
  set modifiable
  call append(0, html)
  if len(style) > 0
    1
    let style_start = search('^</head>')
    call append(style_start, '</style>')
    call append(style_start, style)
    call append(style_start, '<style type="text/css">')
  endif
endfunc
