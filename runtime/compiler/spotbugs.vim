" Vim compiler file
" Compiler:     Spotbugs (Java static checker; needs javac compiled classes)
" Maintainer:   @Konfekt
" Last Change:  2024 nov 6

if exists("current_compiler") | finish | endif
let current_compiler = "spotbugs"

let s:cpo_save = &cpo
set cpo&vim

" Unfortunately Spotbugs does not output absolute paths, so you need to
" pass the directory of the files being checked as `-sourcepath` parameter.
" The regex, auxpath and glob try to include all dependent classes of the
" current buffer. See https://github.com/spotbugs/spotbugs/issues/856

let s:slash = exists('+shellslash') && !&shellslash ? '\' : '/'
let s:regex = '\v%(public |protected |private )?\s*%(abstract |final )?\s*class\s+(\w+)\s+%(extends\s+\w+ |implements\s+\w+%(\s*,\s*\w+)*)?\s*\{'

silent! function s:GetClassFiles() abort
  " Get all class names in the current buffer
  let class_names = []
  for line in getbufline(bufnr('%'), 1, '$')
    let matches = matchlist(line, s:regex)
    if len(matches) > 1 | let class_names += [ matches[1] ] | endif
  endfor

  let class_files = []
  for class_name in class_names
    let class_files += [ shellescape(expand('%:p:h')..s:slash..class_name..'.class') ]
    " add files of inner classes of each class
    let class_files += map(glob(expand('%:p:h')..s:slash..class_name..'\$*.class', 1, 1), 'shellescape(v:val)')
  endfor

  return class_files
endfunction

" CompilerSet makeprg=spotbugs
let &makeprg = 'spotbugs'..(has('win32')?'.bat':'')..' '..
    \ get(b:, 'spotbugs_makeprg_params', get(g:, 'spotbugs_makeprg_params', '-workHard -experimental'))..
    \ ' -textui -emacs -auxclasspath %:p:h:S -sourcepath %:p:h:S '..
    \ join(s:GetClassFiles(), ' ')
exe 'CompilerSet makeprg='..escape(&l:makeprg, ' "')
" Emacs expects doubled line numbers
CompilerSet errorformat=%f:%l:%*[0-9]\ %m,%f:-%*[0-9]:-%*[0-9]\ %m

let &cpo = s:cpo_save
unlet s:cpo_save

