" Vim compiler file
" Compiler:     Spotbugs (Java static checker; needs javac compiled classes)
" Maintainer:   @konfekt and @zzzxywvut
" Last Change:  2024 nov 16

if exists('g:current_compiler') || bufname() !~# '\.java\=$'
  finish
endif

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

if has('syntax') && exists('g:syntax_on') && exists('b:current_syntax') &&
    \ b:current_syntax == 'java' && hlexists('javaClassDecl')

  function! s:GetDeclaredTypeNames() abort
    if bufname() =~# '\<\%(module\|package\)-info\.java\=$'
      return [expand('%:t:r')]
    endif
    defer execute('normal! g``')
    call cursor(1, 1)
    let type_names = []
    let lnum = search(s:types, 'eW')
    while lnum > 0
      if synIDattr(synID(lnum, (col('.') - 1), 0), 'name') ==# 'javaClassDecl'
        let tokens = matchlist(getline(lnum)..getline(lnum + 1), s:names)
        if !empty(tokens) | call add(type_names, tokens[1]) | endif
      endif
      let lnum = search(s:types, 'eW')
    endwhile
    return type_names
  endfunction

else
  function! s:GetDeclaredTypeNames() abort
    if bufname() =~# '\<\%(module\|package\)-info\.java\=$'
      return [expand('%:t:r')]
    endif
    " Undo the unsetting of &hls, see below
    if &hls
      defer execute('set hls')
    endif
    " Possibly restore the current values for registers '"' and "y", see below
    defer call('setreg', ['"', getreg('"'), getregtype('"')])
    defer call('setreg', ['y', getreg('y'), getregtype('y')])
    defer execute('silent bwipeout')
    " Copy buffer contents for modification
    silent %y y
    new
    " Apply ":help scratch-buffer" effects and match "$" in Java (generated)
    " type names (see s:names)
    setlocal iskeyword+=$ buftype=nofile bufhidden=hide noswapfile nohls
    0put y
    " Discard text blocks and strings
    silent keeppatterns %s/\\\@<!"""\_.\{-}\\\@<!"""\|\\"//ge
    silent keeppatterns %s/".*"//ge
    " Discard comments
    silent keeppatterns %s/\/\/.\+$//ge
    silent keeppatterns %s/\/\*\_.\{-}\*\///ge
    call cursor(1, 1)
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

if exists('g:spotbugs_properties') &&
    \ has_key(g:spotbugs_properties, 'sourceDirPath') &&
    \ has_key(g:spotbugs_properties, 'classDirPath')

function! s:FindClassFiles(src_type_name) abort
  let src_dir_path = g:spotbugs_properties.sourceDirPath
  let bin_dir_path = g:spotbugs_properties.classDirPath
  let class_files = []
  " Match pairwise the components of source and class pathnames
  for dir_idx in range(min([len(src_dir_path), len(bin_dir_path)]))
    " Since only the rightmost "src" is sought, while there can be any number of
    " such filenames, no "fnamemodify(a:src_type_name, ':p:s?src?bin?')" is used
    let tail_idx = strridx(a:src_type_name, src_dir_path[dir_idx])
    " No such directory or no such inner type (i.e. without "$")
    if tail_idx < 0 | continue | endif
    " Substitute "bin_dir_path[dir_idx]" for the rightmost "src_dir_path[dir_idx]"
    let candidate_type_name = strpart(a:src_type_name, 0, tail_idx)..
        \ bin_dir_path[dir_idx]..
        \ strpart(a:src_type_name, (tail_idx + strlen(src_dir_path[dir_idx])))
    for candidate in insert(glob(candidate_type_name..'\$*.class', 1, 1),
            \ candidate_type_name..'.class')
      if filereadable(candidate) | call add(class_files, shellescape(candidate)) | endif
    endfor
    if !empty(class_files) | break | endif
  endfor
  return class_files
endfunction

else
function! s:FindClassFiles(src_type_name) abort
  let class_files = []
  for candidate in insert(glob(a:src_type_name..'\$*.class', 1, 1),
          \ a:src_type_name..'.class')
    if filereadable(candidate) | call add(class_files, shellescape(candidate)) | endif
  endfor
  return class_files
endfunction
endif

function! s:CollectClassFiles() abort
  " Get a platform-independent pathname prefix, cf. "expand('%:p:h')..'/'"
  let pathname = expand('%:p')
  let tail_idx = strridx(pathname, expand('%:t'))
  let src_pathname = strpart(pathname, 0, tail_idx)
  let all_class_files = []
  " Get all type names in the current buffer and let the filename globbing
  " discover inner type names from arbitrary type names
  for type_name in s:GetDeclaredTypeNames()
    call extend(all_class_files, s:FindClassFiles(src_pathname..type_name))
  endfor
  return all_class_files
endfunction

" Expose class files for removal etc.
let b:spotbugs_class_files = s:CollectClassFiles()
let g:current_compiler = 'spotbugs'
" CompilerSet makeprg=spotbugs
let &makeprg = 'spotbugs'..(has('win32')?'.bat':'')..' '..
    \ get(b:, 'spotbugs_makeprg_params', get(g:, 'spotbugs_makeprg_params', '-workHard -experimental'))..
    \ ' -textui -emacs -auxclasspath %:p:h:S -sourcepath %:p:h:S '..
    \ join(b:spotbugs_class_files, ' ')
exe 'CompilerSet makeprg='..escape(&l:makeprg, ' "')
" Emacs expects doubled line numbers
CompilerSet errorformat=%f:%l:%*[0-9]\ %m,%f:-%*[0-9]:-%*[0-9]\ %m

delfunction s:CollectClassFiles
delfunction s:FindClassFiles
delfunction s:GetDeclaredTypeNames
let &cpo = s:cpo_save
unlet s:names s:types s:cpo_save
