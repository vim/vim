" Vim autoload file for the tohtml plugin.
" Maintainer: Ben Fritz <fritzophrenic@gmail.com>
" Last Change: 2010 July 16
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
  let old_vals = tohtml#OverrideUserSettings()

  if !&diff || exists("g:html_diff_one_file")
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
    let save_hwf = exists("g:html_whole_filler")
    let g:html_whole_filler = 1
    let g:html_diff_win_num = 0
    for window in win_list
      exe ":" . bufwinnr(window) . "wincmd w"
      let g:html_start_line = 1
      let g:html_end_line = line('$')
      let g:html_diff_win_num += 1
      runtime syntax/2html.vim
      call add(buf_list, bufnr('%'))
      "exec '%s#<span id=''\zsfold\d\+\ze''#win'.win_num.'\0#ge'
    endfor
    unlet g:html_diff_win_num
    if !save_hwf
      unlet g:html_whole_filler
    endif
    call tohtml#Diff2HTML(win_list, buf_list)
  endif

  call tohtml#RestoreUserSettings(old_vals)

  unlet g:html_start_line
  unlet g:html_end_line
endfunc

func! tohtml#Diff2HTML(win_list, buf_list)
  " TODO: add logic for xhtml
  let style = []
  let html = []
  call add(html, '<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN"')
  call add(html, '  "http://www.w3.org/TR/html4/loose.dtd">')
  call add(html, '<html>')
  call add(html, '<head>')
  call add(html, '<title>diff</title>')
  call add(html, '<meta name="Generator" content="Vim/'.v:version/100.'.'.v:version%100.'">')
  " TODO: copy or move encoding logic from 2html.vim so generated markup can
  " validate without warnings about encoding

  call add(html, '</head>')
  call add(html, '<body>')
  call add(html, '<table border="1" width="100%">')

  call add(html, '<tr>')
  for buf in a:win_list
    call add(html, '<th>'.bufname(buf).'</th>')
  endfor
  call add(html, '</tr><tr>')

  for buf in a:buf_list
    let temp = []
    exe bufwinnr(buf) . 'wincmd w'

    " If text is folded because of user foldmethod settings, etc. we don't want
    " to act on everything in a fold by mistake.
    setlocal nofoldenable

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
    " TODO: restore using grabbed lines if undolevel is 1?
    normal 2u
    call add(html, '<td nowrap valign="top"><div>')
    let html += temp
    call add(html, '</div></td>')

    " Close this buffer
    " TODO: the comment above says we're going to allow saving the file
    " later...but here we discard it?
    quit!
  endfor

  call add(html, '</tr>')
  call add(html, '</table>')
  call add(html, '</body>')
  call add(html, '</html>')

  let i = 1
  let name = "Diff" . ".html"
  " Find an unused file name if current file name is already in use
  while filereadable(name)
    let name = substitute(name, '\d*\.html$', '', '') . i . ".html"
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
    if exists("g:html_dynamic_folds")
      call append(style_start, [
	    \  "<script type='text/javascript'>",
	    \  "  <!--",
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
	    \  "  -->",
	    \  "</script>"
	    \ ])
    endif

    " Insert styles from all the generated html documents and additional styles
    " for the table-based layout of the side-by-side diff. The diff should take
    " up the full browser window (but not more), and be static in size,
    " horizontally scrollable when the lines are too long. Otherwise, the diff
    " is pretty useless for really long lines.
    if exists("g:html_use_css")
      call append(style_start, [
	    \ '<style type="text/css">']+
	    \  style+[
	    \ '<!--',
	    \ 'table { table-layout: fixed; }',
	    \ 'html, body, table, tbody { width: 100%; margin: 0; padding: 0; }',
	    \ 'th, td { width: '.printf("%.1f",100.0/len(a:win_list)).'%; }',
	    \ 'td div { overflow: auto; }',
	    \ '-->',
	    \  '</style>'
	    \ ])
    endif
  endif
endfunc

func! tohtml#OverrideUserSettings()
  let old_settings = {}
  " make copies of the user-defined settings that we may overrule
  let old_settings.html_dynamic_folds = exists("g:html_dynamic_folds")
  let old_settings.html_hover_unfold = exists("g:html_hover_unfold")
  let old_settings.html_use_css = exists("g:html_use_css")

  " hover opening implies dynamic folding
  if exists("g:html_hover_unfold")
    let g:html_dynamic_folds = 1
  endif

  " dynamic folding with no foldcolumn implies hover opens
  if exists("g:html_dynamic_folds") && exists("g:html_no_foldcolumn")
    let g:html_hover_unfold = 1
  endif

  " ignore folding overrides dynamic folding
  if exists("g:html_ignore_folding") && exists("g:html_dynamic_folds")
    unlet g:html_dynamic_folds
  endif

  " dynamic folding implies css
  if exists("g:html_dynamic_folds")
    let g:html_use_css = 1
  endif

  return old_settings
endfunc

func! tohtml#RestoreUserSettings(old_settings)
  " restore any overridden user options
  if a:old_settings.html_dynamic_folds
    let g:html_dynamic_folds = 1
  else
    unlet! g:html_dynamic_folds
  endif
  if a:old_settings.html_hover_unfold
    let g:html_hover_unfold = 1
  else
    unlet! g:html_hover_unfold
  endif
  if a:old_settings.html_use_css
    let g:html_use_css = 1
  else
    unlet! g:html_use_css
  endif
endfunc

let &cpo = s:cpo_sav
unlet s:cpo_sav

" Make sure any patches will probably use consistent indent
"   vim: ts=8 sw=2 sts=2 noet
