" FUNCTIONS IN THIS FILE ARE MEANT TO BE USED BY NETRW.VIM AND NETRW.VIM ONLY.
" THESE FUNCTIONS DON'T COMMIT TO ANY BACKWARDS COMPATIBILITY. SO CHANGES AND
" BREAKAGES IF USED OUTSIDE OF NETRW.VIM ARE EXPECTED.

let s:deprecation_msgs = []
function! netrw#msg#Deprecate(name, version, alternatives)
    " If running on neovim use vim.deprecate
    if has('nvim')
        let s:alternative = a:alternatives->get('nvim', v:null)
        call v:lua.vim.deprecate(a:name, s:alternative, a:version, "netrw", v:false)
        return
    endif

    " If we did notify for something only do it once
    if s:deprecation_msgs->index(a:name) >= 0
        return
    endif

    let s:alternative = a:alternatives->get('vim', v:null)
    echohl WarningMsg
    echomsg s:alternative != v:null
                \ ? printf('%s is deprecated, use %s instead.', a:name, s:alternative)
                \ : printf('%s is deprecated.', a:name)
    echomsg printf('Feature will be removed in netrw %s', a:version)
    echohl None

    call add(s:deprecation_msgs, a:name)
endfunction

" netrw#msg#Notify: {{{
"   Usage: netrw#ErrorMsg(g:_netrw_log, 'some message')
"          netrw#ErrorMsg(g:_netrw_log, ["message1","message2",...],error-number)
"          (this function can optionally take a list of messages)
function! netrw#msg#Notify(level, msg)
    if has('nvim')
        call v:lua.vim.notify(level . a:msg, a:level + 2)
        return
    endif

    if a:level == g:_netrw_log.WARN
        echohl WarningMsg
    elseif a:level == g:_netrw_log.ERROR
        echohl ErrorMsg
    endif

    if type(a:msg) == v:t_list
        for msg in a:msg
            echomsg msg
        endfor
    else
        echomsg a:msg
    endif

    echohl None
endfunction

" }}}

" vim:ts=8 sts=4 sw=4 et fdm=marker
