" Language: Typst
" Maintainer: Kaj Munhoz Arfvidsson <kajarfvidsson@gmail.com>
" Upstream: https://github.com/kaarmu/typst.vim

function! typst#TypstWatch(...)
    " Prepare command
    " NOTE: added arguments #23 but they will always be like
    " `typst <args> watch <file> --open` so in the future this might be
    " sensitive to in which order typst options should come.
    let l:cmd = g:typst_cmd
        \ . ' ' . join(a:000)
        \ . ' watch'
        \ . ' --diagnostic-format short'
        \ . " '" . expand('%') . "'"

    if !empty(g:typst_pdf_viewer)
        let l:cmd = l:cmd . ' --open ' . g:typst_pdf_viewer 
    else
        let l:cmd = l:cmd . ' --open'
    endif

    " Write message
    echom 'Starting: ' . l:cmd

    let l:str = has('win32')
              \ ? 'cmd /s /c "' . l:cmd . '"'
              \ : 'sh -c "' . l:cmd . '"'

    if has('nvim')
        let l:JobStart = function('jobstart')
        let l:JobStop = function('jobstop')
        let l:options = {'on_stderr': 'typst#TypstWatcherCb'}
    else
        let l:JobStart = function('job_start')
        let l:JobStop = function('job_stop')
        let l:options = {'err_mode': 'raw',
                        \'err_cb': 'typst#TypstWatcherCb'}
    endif

    if exists('s:watcher') " && job_status(s:watcher) == 'run'
        " echoerr 'TypstWatch is already running.'
        call l:JobStop(s:watcher)
    endif

    let s:watcher = l:JobStart(l:str, l:options)

endfunction

" Callback function for job exit
function! typst#TypstWatcherCb(channel, content, ...)
    let l:errors = []
    let l:lines = a:content
    if !has('nvim')
	let l:lines = split(l:lines, "\n")
    endif
    for l:line in l:lines
        " Probably this match can be done using errorformat.
	" Maybe do something like vim-dispatch.
        let l:match = matchlist(l:line, '\v^([^:]+):(\d+):(\d+):\s*(.+)$')
        if 0 < len(l:match)
            let l:error = {'filename': l:match[1],
                          \'lnum': l:match[2],
                          \'col': l:match[3],
                          \'text': l:match[4]}
            call add(l:errors, l:error)
        endif
    endfor
    call setqflist(l:errors)
    if g:typst_auto_open_quickfix
        execute empty(l:errors) ? 'cclose' : 'copen'
    endif
endfunction

" Below are adapted from preservim/vim-markdown
" They have their own MIT License at https://github.com/preservim/vim-markdown#license
let s:headersRegexp = '^='

" For each level, contains the regexp that matches at that level only.
"
let s:levelRegexpDict = {
    \ 1: '^=[^=]',
    \ 2: '^==[^=]',
    \ 3: '^===[^=]',
    \ 4: '^====[^=]',
    \ 5: '^=====[^=]',
    \ 6: '^======[^=]'
\ }


" Returns the level of the header at the given line.
"
" If there is no header at the given line, returns `0`.
"
function! s:GetLevelOfHeaderAtLine(linenum)
    let l:lines = join(getline(a:linenum, a:linenum + 1), "\n")
    for l:key in keys(s:levelRegexpDict)
        if l:lines =~ get(s:levelRegexpDict, l:key)
            return l:key
        endif
    endfor
    return 0
endfunction

function! s:GetHeaderLineNum(...)
    if a:0 == 0
        let l:l = line('.')
    else
        let l:l = a:1
    endif
    while(l:l > 0)
        if join(getline(l:l, l:l + 1), "\n") =~ s:headersRegexp
            return l:l
        endif
        let l:l -= 1
    endwhile
    return 0
endfunction

function! s:GetHeaderLevel(...)
    if a:0 == 0
        let l:line = line('.')
    else
        let l:line = a:1
    endif
    let l:linenum = s:GetHeaderLineNum(l:line)
    if l:linenum !=# 0
        return s:GetLevelOfHeaderAtLine(l:linenum)
    else
        return 0
    endif
endfunction

function! s:GetHeaderList()
    let l:bufnr = bufnr('%')
    let l:fenced_block = 0
    let l:front_matter = 0
    let l:header_list = []
    let l:vim_markdown_frontmatter = get(g:, 'vim_markdown_frontmatter', 0)
    for i in range(1, line('$'))
        let l:lineraw = getline(i)
        let l:l1 = getline(i+1)
        let l:line = substitute(l:lineraw, '#', "\\\#", 'g')
        if join(getline(i, i + 1), "\n") =~# s:headersRegexp && l:line =~# '^\S'
            let l:is_header = 1
        else
            let l:is_header = 0
        endif
        if l:is_header ==# 1 && l:fenced_block ==# 0 && l:front_matter ==# 0
            if match(l:line, '^#') > -1
                let l:line = substitute(l:line, '\v^#*[ ]*', '', '')
                let l:line = substitute(l:line, '\v[ ]*#*$', '', '')
            endif
            let l:level = s:GetHeaderLevel(i)
            let l:item = {'level': l:level, 'text': l:line, 'lnum': i, 'bufnr': bufnr}
            let l:header_list = l:header_list + [l:item]
        endif
    endfor
    return l:header_list
endfunction

function! typst#Toc(...)
    if a:0 > 0
        let l:window_type = a:1
    else
        let l:window_type = 'vertical'
    endif

    let l:cursor_line = line('.')
    let l:cursor_header = 0
    let l:header_list = s:GetHeaderList()
    let l:indented_header_list = []
    if len(l:header_list) == 0
        echom 'Toc: No headers.'
        return
    endif
    let l:header_max_len = 0
    let l:vim_markdown_toc_autofit = get(g:, 'vim_markdown_toc_autofit', 0)
    for h in l:header_list
        if l:cursor_header == 0
            let l:header_line = h.lnum
            if l:header_line == l:cursor_line
                let l:cursor_header = index(l:header_list, h) + 1
            elseif l:header_line > l:cursor_line
                let l:cursor_header = index(l:header_list, h)
            endif
        endif
        let l:text = repeat('  ', h.level-1) . h.text
        let l:total_len = strdisplaywidth(l:text)
        if l:total_len > l:header_max_len
            let l:header_max_len = l:total_len
        endif
        let l:item = {'lnum': h.lnum, 'text': l:text, 'valid': 1, 'bufnr': h.bufnr, 'col': 1}
        let l:indented_header_list = l:indented_header_list + [l:item]
    endfor

    " Open the TOC buffer in a new window
    let l:orig_winid = win_getid()
    let l:toc_bufnr = bufnr('TOC', 1)
    " execute 'sbuffer ' . l:toc_bufnr
    if a:0 > 0
        if a:1 == 'vertical'
            execute 'vsplit +buffer' . l:toc_bufnr
            if (&columns/2) > l:header_max_len && l:vim_markdown_toc_autofit == 1
                execute 'vertical resize ' . (l:header_max_len + 1 + 3)
            else
                execute 'vertical resize ' . (&columns/2)
            endif
        elseif a:1 == 'tab'
            execute 'tabnew | buffer' . l:toc_bufnr
        else
            execute 'sbuffer ' . l:toc_bufnr
        endif
    else
        execute 'sbuffer ' . l:toc_bufnr
    endif

    setlocal buftype=nofile
    setlocal bufhidden=delete
    call setbufline(l:toc_bufnr, 1, map(copy(l:indented_header_list), 'v:val.text'))
    let b:indented_header_list = l:indented_header_list
    let b:orig_winid = l:orig_winid

    " Define a mapping to jump to the corresponding line in the original file when a line is clicked
    nnoremap <buffer> <silent> <Enter> :call <SID>JumpToHeader()<CR>

    " Move the cursor to the current header in the TOC
    execute 'normal! ' . l:cursor_header . 'G'

endfunction

function! s:JumpToHeader()
    let l:lnum = line('.')
    let l:header_info = b:indented_header_list[l:lnum - 1]
    let l:orig_winid = b:orig_winid
    call win_execute(l:orig_winid, 'buffer ' . l:header_info.bufnr)
    call win_execute(l:orig_winid, 'normal! ' . l:header_info.lnum . 'G')
    if g:typst_auto_close_toc
        bwipeout!
    endif
    call win_gotoid(l:orig_winid)
endfunction

" Detect context for #51
" Detects the inner most syntax group under the cursor by default.
function! typst#synstack(kwargs = {}) abort 
    let l:pos = get(a:kwargs, 'pos', getcurpos()[1:3])
    let l:only_inner = get(a:kwargs, 'only_inner', v:true)
    if mode() ==# 'i'
        let l:pos[1] -= 1
    endif
    call map(l:pos, 'max([v:val, 1])')

    let l:stack = map(synstack(l:pos[0], l:pos[1]), "synIDattr(v:val, 'name')")
    return l:only_inner ? l:stack[-1:] : l:stack
endfunction

function! typst#in_markup(...) abort
    let l:stack = call('typst#synstack', a:000)
    let l:ret = empty(l:stack)
    for l:name in l:stack
        let l:ret = l:ret 
            \ || l:name =~? '^typstMarkup'
            \ || l:name =~? 'Bracket$'
    endfor
    return l:ret
endfunction

function! typst#in_code(...) abort
    let l:ret = v:false
    for l:name in call('typst#synstack', a:000)
        let l:ret = l:ret 
            \ || l:name =~? '^typstCode'
            \ || l:name =~? 'Brace$'
    endfor 
    return l:ret
endfunction

function! typst#in_math(...) abort
    let l:ret = v:false
    for l:name in call('typst#synstack', a:000)
        let l:ret = l:ret 
            \ || l:name =~? '^typstMath'
            \ || l:name =~? 'Dollar$'
    endfor
    return l:ret
endfunction

function! typst#in_comment(...) abort
    let l:ret = v:false
    for l:name in call('typst#synstack', a:000)
        let l:ret = l:ret 
            \ || l:name =~? '^typstComment'
    endfor
    return l:ret
endfunction
