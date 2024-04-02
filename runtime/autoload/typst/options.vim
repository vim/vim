" Language: Typst
" Maintainer: Kaj Munhoz Arfvidsson <kajarfvidsson@gmail.com>
" Upstream: https://github.com/kaarmu/typst.vim

let s:initialized = v:false

function! typst#options#init() abort " {{{1
    if s:initialized | return | endif

    call s:declare_option('typst_cmd', 'typst')
    call s:declare_option('typst_pdf_viewer', '')
    call s:declare_option('typst_conceal', 0)
    call s:declare_option('typst_conceal_math', g:typst_conceal)
    call s:declare_option('typst_conceal_emoji', g:typst_conceal)
    call s:declare_option('typst_auto_close_toc', 0)
    call s:declare_option('typst_auto_open_quickfix', 1)
    call s:declare_option('typst_embedded_languages', [])

    let s:initialized = v:true
endfunction " }}}1

function! s:declare_option(option, default) " {{{1
    let l:option = 'g:' . a:option
    if !exists(l:option)
        let {l:option} = a:default
    endif
endfunction " }}}1

" vim: tabstop=8 shiftwidth=4 softtabstop=4 expandtab
