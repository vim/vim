" Default pre- and post-compiler actions for SpotBugs
" Maintainers:  @konfekt and @zzzxywvut
" Last Change:  2024 nov 24

let s:save_cpo = &cpo
set cpo&vim

if v:version > 900
  function! spotbugs#DeleteClassFiles() abort
    if !exists('b:spotbugs_class_files')
      return
    endif

    for pathname in b:spotbugs_class_files
      let classname = pathname =~# "^'.\\+\\.class'$"
          \ ? eval(pathname)
          \ : pathname

      if classname =~# '\.class$' && filereadable(classname)
        " After v9.0.0795 and since v8.2.2343.
        let octad = readblob(classname, 0, 8)

        " Test the magic number and the major version number (45 for v1.0).
        " Since v7.3.377.  Since v8.1.0735.  Since v8.2.5003.
        if len(octad) == 8 && octad[0 : 3] == 0zcafe.babe &&
              \ or((octad[6] << 8), octad[7]) >= 45
          echomsg printf('Deleting %s: %d', classname, delete(classname))
        endif
      endif
    endfor

    let b:spotbugs_class_files = []
  endfunction

else

  function! spotbugs#DeleteClassFiles() abort
    if !exists('b:spotbugs_class_files')
      return
    endif

    let encoding = &encoding

    try
      set encoding=latin1

      for pathname in b:spotbugs_class_files
        let classname = pathname =~# "^'.\\+\\.class'$"
            \ ? eval(pathname)
            \ : pathname

        if classname =~# '\.class$' && filereadable(classname)
          let line = get(readfile(classname, 'b', 1), 0, '')

          " Test the magic number and the major version number (45 for v1.0).
          if strlen(line) > 7 && line[0 : 3] == "\xca\xfe\xba\xbe" &&
                \ ((line[6] == "\n" ? 0 : char2nr(line[6]) % 256) * 256 +
                    \ char2nr(line[7]) % 256) >= 45
            echomsg printf('Deleting %s: %d', classname, delete(classname))
          endif
        endif
      endfor
    finally
      let &encoding = encoding
    endtry

    let b:spotbugs_class_files = []
  endfunction
endif

function! spotbugs#DefaultPostCompilerAction() abort
  " Since v7.4.191.
  make %:S
endfunction

" Look for "spotbugs#compiler" in "ftplugin/java.vim".
let s:compiler = exists('spotbugs#compiler') ? spotbugs#compiler : ''
let s:readable = filereadable($VIMRUNTIME . '/compiler/' . s:compiler . '.vim')

if s:readable && s:compiler ==# 'maven' && executable('mvn')

  function! spotbugs#DefaultPreCompilerAction() abort
    call spotbugs#DeleteClassFiles()
    compiler maven
    make compile
  endfunction

  function! spotbugs#DefaultPreCompilerTestAction() abort
    call spotbugs#DeleteClassFiles()
    compiler maven
    make test-compile
  endfunction

  function! spotbugs#DefaultProperties() abort
    return {
        \ 'PreCompilerAction':
            \ function('spotbugs#DefaultPreCompilerAction'),
        \ 'PreCompilerTestAction':
            \ function('spotbugs#DefaultPreCompilerTestAction'),
        \ 'PostCompilerAction':
            \ function('spotbugs#DefaultPostCompilerAction'),
        \ 'sourceDirPath':      'src/main/java',
        \ 'classDirPath':       'target/classes',
        \ 'testSourceDirPath':  'src/test/java',
        \ 'testClassDirPath':   'target/test-classes',
        \ }
  endfunction

elseif s:readable && s:compiler ==# 'ant' && executable('ant')

  function! spotbugs#DefaultPreCompilerAction() abort
    call spotbugs#DeleteClassFiles()
    compiler ant
    make compile
  endfunction

  function! spotbugs#DefaultPreCompilerTestAction() abort
    call spotbugs#DeleteClassFiles()
    compiler ant
    make compile-test
  endfunction

  function! spotbugs#DefaultProperties() abort
    return {
        \ 'PreCompilerAction':
            \ function('spotbugs#DefaultPreCompilerAction'),
        \ 'PreCompilerTestAction':
            \ function('spotbugs#DefaultPreCompilerTestAction'),
        \ 'PostCompilerAction':
            \ function('spotbugs#DefaultPostCompilerAction'),
        \ 'sourceDirPath':      'src',
        \ 'classDirPath':       'build/classes',
        \ 'testSourceDirPath':  'test',
        \ 'testClassDirPath':   'build/test/classes',
        \ }
  endfunction

elseif s:readable && s:compiler ==# 'javac' && executable('javac')

  function! spotbugs#DefaultPreCompilerAction() abort
    call spotbugs#DeleteClassFiles()
    compiler javac
    make %:S
  endfunction

  function! spotbugs#DefaultPreCompilerTestAction() abort
    call spotbugs#DefaultPreCompilerAction()
  endfunction

  function! spotbugs#DefaultProperties() abort
    return {
        \ 'PreCompilerAction':
            \ function('spotbugs#DefaultPreCompilerAction'),
        \ 'PreCompilerTestAction':
            \ function('spotbugs#DefaultPreCompilerTestAction'),
        \ 'PostCompilerAction':
            \ function('spotbugs#DefaultPostCompilerAction'),
        \ }
  endfunction

else

  function! spotbugs#DefaultPreCompilerAction() abort
    echomsg printf('Not supported: "%s"', s:compiler)
  endfunction

  function! spotbugs#DefaultPreCompilerTestAction() abort
    call spotbugs#DefaultPreCompilerAction()
  endfunction

  function! spotbugs#DefaultProperties() abort
    return {}
  endfunction

endif

let &cpo = s:save_cpo
unlet s:readable s:save_cpo

" vim: set foldmethod=syntax shiftwidth=2 expandtab:
