" Test for completion menu

let g:months = ['January', 'February', 'March', 'April', 'May', 'June', 'July', 'August', 'September', 'October', 'November', 'December']
let g:setting = ''

func! ListMonths()
  if g:setting != ''
    exe ":set" g:setting
  endif
  let mth=copy(g:months)
  let entered = strcharpart(getline('.'),0,col('.'))
  if !empty(entered)
    let mth=filter(mth, 'v:val=~"^".entered')
  endif
  call complete(1, mth) 
  return ''
endfunc

func! Test_popup_complete2()
  " Insert match immediately, if there is only one match
  "  <c-e> Should select a character from the line below
  " TODO: test disabled because the code change has been reverted.
  throw "Skipped: Bug with <c-e> and popupmenu not fixed yet"
  new
  inoremap <f5> <c-r>=ListMonths()<cr>
  call append(1, ["December2015"])
  :1
  call feedkeys("aD\<f5>\<C-E>\<C-E>\<C-E>\<C-E>\<enter>\<esc>", 'tx')
  call assert_equal(["December2015", "", "December2015"], getline(1,3))
  %d
  bw!
endfu

func! Test_popup_complete()
  new
  inoremap <f5> <c-r>=ListMonths()<cr>

  " <C-E> - select original typed text before the completion started
  call feedkeys("aJu\<f5>\<down>\<c-e>\<esc>", 'tx')
  call assert_equal(["Ju"], getline(1,2))
  %d

  " <C-Y> - accept current match
  call feedkeys("a\<f5>". repeat("\<down>",7). "\<c-y>\<esc>", 'tx')
  call assert_equal(["August"], getline(1,2))
  %d

  " <BS> - Delete one character from the inserted text (state: 1)
  " TODO: This should not end the completion, but it does.
  " This should according to the documentation:
  " January
  " but instead, this does
  " Januar
  " (idea is, C-L inserts the match from the popup menu
  " but if the menu is closed, it will insert the character <c-l>
  call feedkeys("aJ\<f5>\<bs>\<c-l>\<esc>", 'tx')
  call assert_equal(["Januar"], getline(1,2))
  %d

  " any-non special character: Stop completion without changing the match
  " and insert the typed character
  call feedkeys("a\<f5>20", 'tx')
  call assert_equal(["January20"], getline(1,2))
  %d

  " any-non printable, non-white character: Add this character and
  " reduce number of matches
  call feedkeys("aJu\<f5>\<c-p>l\<c-y>", 'tx')
  call assert_equal(["Jul"], getline(1,2))
  %d
  
  " any-non printable, non-white character: Add this character and
  " reduce number of matches
  call feedkeys("aJu\<f5>\<c-p>l\<c-n>\<c-y>", 'tx')
  call assert_equal(["July"], getline(1,2))
  %d

  " any-non printable, non-white character: Add this character and
  " reduce number of matches
  call feedkeys("aJu\<f5>\<c-p>l\<c-e>", 'tx')
  call assert_equal(["Jul"], getline(1,2))
  %d

  " <BS> - Delete one character from the inserted text (state: 2)
  call feedkeys("a\<f5>\<c-n>\<bs>", 'tx')
  call assert_equal(["Februar"], getline(1,2))
  %d

  " <c-l> - Insert one character from the current match
  call feedkeys("aJ\<f5>".repeat("\<c-n>",3)."\<c-l>\<esc>", 'tx')
  call assert_equal(["J"], getline(1,2))
  %d
  
  " <c-l> - Insert one character from the current match
  call feedkeys("aJ\<f5>".repeat("\<c-n>",4)."\<c-l>\<esc>", 'tx')
  call assert_equal(["January"], getline(1,2))
  %d

  " <c-y> - Accept current selected match
  call feedkeys("aJ\<f5>\<c-y>\<esc>", 'tx')
  call assert_equal(["January"], getline(1,2))
  %d

  " <c-e> - End completion, go back to what was there before selecting a match
  call feedkeys("aJu\<f5>\<c-e>\<esc>", 'tx')
  call assert_equal(["Ju"], getline(1,2))
  %d

  " <PageUp> - Select a match several entries back
  call feedkeys("a\<f5>\<PageUp>\<c-y>\<esc>", 'tx')
  call assert_equal([""], getline(1,2))
  %d

  " <PageUp><PageUp> - Select a match several entries back
  call feedkeys("a\<f5>\<PageUp>\<PageUp>\<c-y>\<esc>", 'tx')
  call assert_equal(["December"], getline(1,2))
  %d

  " <PageUp><PageUp><PageUp> - Select a match several entries back
  call feedkeys("a\<f5>\<PageUp>\<PageUp>\<PageUp>\<c-y>\<esc>", 'tx')
  call assert_equal(["February"], getline(1,2))
  %d

  " <PageDown> - Select a match several entries further
  call feedkeys("a\<f5>\<PageDown>\<c-y>\<esc>", 'tx')
  call assert_equal(["November"], getline(1,2))
  %d

  " <PageDown><PageDown> - Select a match several entries further
  call feedkeys("a\<f5>\<PageDown>\<PageDown>\<c-y>\<esc>", 'tx')
  call assert_equal(["December"], getline(1,2))
  %d

  " <PageDown><PageDown><PageDown> - Select a match several entries further
  call feedkeys("a\<f5>\<PageDown>\<PageDown>\<PageDown>\<c-y>\<esc>", 'tx')
  call assert_equal([""], getline(1,2))
  %d

  " <PageDown><PageDown><PageDown><PageDown> - Select a match several entries further
  call feedkeys("a\<f5>".repeat("\<PageDown>",4)."\<c-y>\<esc>", 'tx')
  call assert_equal(["October"], getline(1,2))
  %d

  " <Up> - Select a match don't insert yet
  call feedkeys("a\<f5>\<Up>\<c-y>\<esc>", 'tx')
  call assert_equal([""], getline(1,2))
  %d

  " <Up><Up> - Select a match don't insert yet
  call feedkeys("a\<f5>\<Up>\<Up>\<c-y>\<esc>", 'tx')
  call assert_equal(["December"], getline(1,2))
  %d

  " <Up><Up><Up> - Select a match don't insert yet
  call feedkeys("a\<f5>\<Up>\<Up>\<Up>\<c-y>\<esc>", 'tx')
  call assert_equal(["November"], getline(1,2))
  %d

  " <Tab> - Stop completion and insert the match
  call feedkeys("a\<f5>\<Tab>\<c-y>\<esc>", 'tx')
  call assert_equal(["January	"], getline(1,2))
  %d

  " <Space> - Stop completion and insert the match
  call feedkeys("a\<f5>".repeat("\<c-p>",5)." \<esc>", 'tx')
  call assert_equal(["September "], getline(1,2))
  %d

  " <Enter> - Use the text and insert line break (state: 1)
  call feedkeys("a\<f5>\<enter>\<esc>", 'tx')
  call assert_equal(["January", ''], getline(1,2))
  %d

  " <Enter> - Insert the current selected text (state: 2)
  call feedkeys("a\<f5>".repeat("\<Up>",5)."\<enter>\<esc>", 'tx')
  call assert_equal(["September"], getline(1,2))
  %d

  " Insert match immediately, if there is only one match
  " <c-y> selects a character from the line above
  call append(0, ["December2015"])
  call feedkeys("aD\<f5>\<C-Y>\<C-Y>\<C-Y>\<C-Y>\<enter>\<esc>", 'tx')
  call assert_equal(["December2015", "December2015", ""], getline(1,3))
  %d

  " use menuone for 'completeopt'
  " Since for the first <c-y> the menu is still shown, will only select
  " three letters from the line above
  set completeopt&vim
  set completeopt+=menuone
  call append(0, ["December2015"])
  call feedkeys("aD\<f5>\<C-Y>\<C-Y>\<C-Y>\<C-Y>\<enter>\<esc>", 'tx')
  call assert_equal(["December2015", "December201", ""], getline(1,3))
  %d

  " use longest for 'completeopt'
  set completeopt&vim
  call feedkeys("aM\<f5>\<C-N>\<C-P>\<c-e>\<enter>\<esc>", 'tx')
  set completeopt+=longest
  call feedkeys("aM\<f5>\<C-N>\<C-P>\<c-e>\<enter>\<esc>", 'tx')
  call assert_equal(["M", "Ma", ""], getline(1,3))
  %d

  " use noselect/noinsert for 'completeopt'
  set completeopt&vim
  call feedkeys("aM\<f5>\<enter>\<esc>", 'tx')
  set completeopt+=noselect
  call feedkeys("aM\<f5>\<enter>\<esc>", 'tx')
  set completeopt-=noselect completeopt+=noinsert
  call feedkeys("aM\<f5>\<enter>\<esc>", 'tx')
  call assert_equal(["March", "M", "March"], getline(1,4))
  %d
endfu


func! Test_popup_completion_insertmode()
  new
  inoremap <F5> <C-R>=ListMonths()<CR>

  call feedkeys("a\<f5>\<down>\<enter>\<esc>", 'tx')
  call assert_equal('February', getline(1))
  %d
  " Set noinsertmode
  let g:setting = 'noinsertmode'
  call feedkeys("a\<f5>\<down>\<enter>\<esc>", 'tx')
  call assert_equal('February', getline(1))
  call assert_false(pumvisible())
  %d
  " Go through all matches, until none is selected
  let g:setting = ''
  call feedkeys("a\<f5>". repeat("\<c-n>",12)."\<enter>\<esc>", 'tx')
  call assert_equal('', getline(1))
  %d
  " select previous entry
  call feedkeys("a\<f5>\<c-p>\<enter>\<esc>", 'tx')
  call assert_equal('', getline(1))
  %d
  " select last entry
  call feedkeys("a\<f5>\<c-p>\<c-p>\<enter>\<esc>", 'tx')
  call assert_equal('December', getline(1))

  iunmap <F5>
endfunc

func Test_noinsert_complete()
  function! s:complTest1() abort
    call complete(1, ['source', 'soundfold'])
    return ''
  endfunction

  function! s:complTest2() abort
    call complete(1, ['source', 'soundfold'])
    return ''
  endfunction

  new
  set completeopt+=noinsert
  inoremap <F5>  <C-R>=s:complTest1()<CR>
  call feedkeys("i\<F5>soun\<CR>\<CR>\<ESC>.", 'tx')
  call assert_equal('soundfold', getline(1))
  call assert_equal('soundfold', getline(2))
  bwipe!

  new
  inoremap <F5>  <C-R>=s:complTest2()<CR>
  call feedkeys("i\<F5>\<CR>\<ESC>", 'tx')
  call assert_equal('source', getline(1))
  bwipe!

  set completeopt-=noinsert
  iunmap <F5>
endfunc

func Test_compl_vim_cmds_after_register_expr()
  function! s:test_func()
    return 'autocmd '
  endfunction
  augroup AAAAA_Group
    au!
  augroup END

  new
  call feedkeys("i\<c-r>=s:test_func()\<CR>\<C-x>\<C-v>\<Esc>", 'tx')
  call assert_equal('autocmd AAAAA_Group', getline(1))
  autocmd! AAAAA_Group
  augroup! AAAAA_Group
  bwipe!
endfunc

" vim: shiftwidth=2 sts=2 expandtab
