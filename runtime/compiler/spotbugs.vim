" Vim compiler file
" Compiler:     Spotbugs (Java static checker; needs javac compiled classes)
" Maintainer:   @konfekt and @zzzxywvut
" Last Change:  2024 nov 12

if exists("current_compiler") | finish | endif

let s:cpo_save = &cpo
set cpo&vim

" Unfortunately Spotbugs does not output absolute paths, so you need to
" pass the directory of the files being checked as `-sourcepath` parameter.
" The regex, auxpath and glob try to include all dependent classes of the
" current buffer. See https://github.com/spotbugs/spotbugs/issues/856

" FIXME: When "search()" is used with the "e" flag, it makes no _further_
" progress after claiming an EOL match (i.e. "\_" or "\n", but not "$").
" XXX: Omit anonymous class declarations
let s:types = '\C\<\%(\.\@1<!class\|@\=interface\|enum\|record\)\%(\s\|$\)'
let s:names = '\C\<\%(\.\@1<!class\|@\=interface\|enum\|record\)\s*\(\K\k*\)\>'
let s:slash = exists('+shellslash') && !&shellslash ? '\' : '/'

if has('syntax') && exists('g:syntax_on') && exists('b:current_syntax') &&
    \ b:current_syntax == 'java' && hlexists('javaClassDecl')

  function! s:GetDeclaredTypeNames() abort
    defer execute('keepjumps normal ``')
    normal gg
    let type_names = []
    let lnum = search(s:types, 'eW')
    while lnum > 0
      if synIDattr(synID(lnum, (col('.') - 1), 0), 'name') == 'javaClassDecl'
        let tokens = matchlist(getline(lnum)..getline(lnum + 1), s:names)
        if !empty(tokens) | call add(type_names, tokens[1]) | endif
      endif
      let lnum = search(s:types, 'eW')
    endwhile
    return type_names
  endfunction

else
  function! s:GetDeclaredTypeNames() abort
    " Undo the unsetting of &hls, see below
    if &hls | defer execute('set hls') | endif
    " Possibly restore the current value for register "y", see below
    defer execute('let @y = '..(!empty(string(@y)) ? string(@y) : '""'))
    " Copy buffer contents for modification
    silent %y y
    new
    defer execute('silent bwipeout')
    " Apply ":help scratch-buffer" effects
    setlocal buftype=nofile bufhidden=hide noswapfile nohls
    silent normal "yP
    " Discard text blocks and strings
    silent keeppatterns %s/\\\@<!"""\_.\{-}\\\@<!"""\|\\"//ge
    silent keeppatterns %s/".*"//ge
    " Discard comments
    silent keeppatterns %s/\/\/.\+$//ge
    silent keeppatterns %s/\/\*\_.\{-}\*\///ge
    normal gg
    let type_names = []
    let lnum = search(s:types, 'eW')
    while lnum > 0
      let tokens = matchlist(getline(lnum)..getline(lnum + 1), s:names)
      if !empty(tokens) | call add(type_names, tokens[1]) | endif
      let lnum = search(s:types, 'eW')
    endwhile
    return type_names
  endfunction
endif

function! s:GetClassFiles() abort
  " Get all type names in the current buffer and let the filename globbing
  " discover inner type names from arbitrary type names
  let class_files = []

  for type_name in s:GetDeclaredTypeNames()
    for candidate in insert(glob(expand('%:p:h')..s:slash..type_name..'\$*.class', 1, 1),
        \ expand('%:p:h')..s:slash..type_name..'.class')
      if filereadable(candidate) | call add(class_files, shellescape(candidate)) | endif
    endfor
  endfor

  return class_files
endfunction

function! s:IsClassFileCurrent(javaFile)
    let classFile = substitute(a:javaFile, '\.java$', '.class', '')
    return filereadable(classFile) && getftime(classFile) > getftime(a:javaFile)
endfunction

if !s:IsClassFileCurrent(expand('%')) && executable('javac') | compiler javac | make %:S | endif

let current_compiler = "spotbugs"
" CompilerSet makeprg=spotbugs
let &makeprg = 'spotbugs'..(has('win32')?'.bat':'')..' '..
    \ get(b:, 'spotbugs_makeprg_params', get(g:, 'spotbugs_makeprg_params', '-workHard -experimental'))..
    \ ' -textui -emacs -auxclasspath %:p:h:S -sourcepath %:p:h:S '..
    \ join(s:GetClassFiles(), ' ')
exe 'CompilerSet makeprg='..escape(&l:makeprg, ' "')
" Emacs expects doubled line numbers
CompilerSet errorformat=%f:%l:%*[0-9]\ %m,%f:-%*[0-9]:-%*[0-9]\ %m

delfunction s:GetClassFiles
delfunction s:GetDeclaredTypeNames
delfunction s:IsClassFileCurrent
let &cpo = s:cpo_save
unlet s:slash s:names s:types s:cpo_save
