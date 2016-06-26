" Test for completion menu

inoremap <F5> <C-R>=ListMonths()<CR>
let g:months = ['January', 'February', 'March', 'April', 'May', 'June', 'July', 'August', 'September', 'October', 'November', 'December']
let g:setting = ''

func ListMonths()
    if g:setting != ''
	exe ":set" g:setting
    endif
    call complete(col('.'), g:months) 
    return ''
endfunc

func! Test_popup_completion_insertmode()
    new
    call feedkeys("a\<f5>\<down>\<enter>\<esc>", 'tx')
    call assert_equal('February', getline(1))
    %d
    let g:setting = 'noinsertmode'
    call feedkeys("a\<f5>\<down>\<enter>\<esc>", 'tx')
    call assert_equal('February', getline(1))
    call assert_false(pumvisible())
    %d
    let g:setting = ''
    call feedkeys("a\<f5>". repeat("\<c-n>",12)."\<enter>\<esc>", 'tx')
    call assert_equal('', getline(1))
    %d
    call feedkeys("a\<f5>\<c-p>\<enter>\<esc>", 'tx')
    call assert_equal('', getline(1))
    %d
    call feedkeys("a\<f5>\<c-p>\<c-p>\<enter>\<esc>", 'tx')
    call assert_equal('December', getline(1))
    bwipe!
endfunc
