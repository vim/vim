set complete+=kspell
set completeopt+=menu
set completeopt+=menuone
set completeopt+=noselect
set completeopt+=noinsert
let g:autocompletion = v:true
let g:autocomplete_type = "buffer"

" Omnicomplete
filetype plugin on
set omnifunc=syntaxcomplete#Complete

function! OpenCompletion()
    if pumvisible() && (g:autocompletion == v:true)
        if g:autocomplete_type == "buffer"
            call feedkeys("\<C-e>\<C-n>", "i")
        endif
        if g:autocomplete_type == "omni"
            call feedkeys("\<C-e>\<C-x>\<C-o>", "i")
        endif
        return
        redraw
    endif
    if ((v:char >= 'a' && v:char <= 'z') || (v:char >= 'A' && v:char <= 'Z')) && (g:autocompletion == v:true)
        if g:autocomplete_type == "buffer"
            call feedkeys("\<C-n>", "i")
        endif
        if g:autocomplete_type == "omni"
            call feedkeys("\<C-x>\<C-o>", "i")
        endif
        redraw
    endif
endfunction

function! ToggleAutocomplete()
    if g:autocompletion == v:true 
        echom "Auto-completion disabled"
        let g:autocompletion = v:false
    else
        echom "Auto-completion enabled"
        let g:autocompletion = v:true
    endif
endfunction

function! ToggleCompleteType()
    if g:autocomplete_type == "buffer" 
        echom "Auto-completion type set to omnicomplete"
        let g:autocomplete_type = "omni"
        return
    endif
    if g:autocomplete_type == "omni" 
        echom "Auto-completion type set to normal"
        let g:autocomplete_type = "buffer"
        return
    endif
    let g:autocomplete_type = "buffer"
endfunction

autocmd InsertCharPre * noautocmd call OpenCompletion()

setlocal spell! spelllang=en_us

func Test_auto_complete_no_halt()
    call feedkeys("iauto-complete-halt-test test test test test test test test test test test test test test test test test test test\<C-c>", "tx!")
    call assert_equal(["auto-complete-halt-test test test test test test test test test test test test test test test test test test test"], getline(1, "$"))
endfunc
