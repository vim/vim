" Vim filetype plugin file
" Language:             Scala
" Maintainer:           Derek Wyatt
" URL:                  https://github.com/derekwyatt/vim-scala
" License:              Same as Vim
" Last Change:          02 August 2016
" ----------------------------------------------------------------------------

if exists('b:did_ftplugin') || &cp
  finish
endif
let b:did_ftplugin = 1

" j is fairly new in Vim, so don't complain if it's not there
setlocal formatoptions-=t formatoptions+=croqnl
silent! setlocal formatoptions+=j

" Just like c.vim, but additionally doesn't wrap text onto /** line when
" formatting. Doesn't bungle bulleted lists when formatting.
if get(g:, 'scala_scaladoc_indent', 0)
  setlocal comments=sO:*\ -,mO:*\ \ ,exO:*/,s2:/**,mb:*,ex:*/,s1:/*,mb:*,ex:*/,://
else
  setlocal comments=sO:*\ -,mO:*\ \ ,exO:*/,s1:/**,mb:*,ex:*/,s1:/*,mb:*,ex:*/,://
endif
setlocal commentstring=//\ %s

setlocal shiftwidth=2 softtabstop=2 expandtab

setlocal include='^\s*import'
setlocal includeexpr='substitute(v:fname,"\\.","/","g")'

setlocal path+=src/main/scala,src/test/scala
setlocal suffixesadd=.scala

compiler sbt

if globpath(&rtp, 'plugin/fuf.vim') != ''
    "
    " FuzzyFinder stuff
    "
    "
    " SanitizeDirForFuzzyFinder()
    "
    " This is really just a convenience function to clean up any stray '/'
    " characters in the path, should they be there.
    "
    function! scala#SanitizeDirForFuzzyFinder(dir)
        let dir = expand(a:dir)
        let dir = substitute(dir, '/\+$', '', '')
        let dir = substitute(dir, '/\+', '/', '')

        return dir
    endfunction

    "
    " GetDirForFuzzyFinder()
    "
    " Given a directory to start 'from', walk up the hierarchy, looking for a path
    " that matches the 'addon' you want to see.
    "
    " If nothing can be found, then we just return the 'from' so we don't really get
    " the advantage of a hint, but just let the user start from wherever he was
    " starting from anyway.
    "
    function! scala#GetDirForFuzzyFinder(from, addon)
        let from = scala#SanitizeDirForFuzzyFinder(a:from)
        let addon = expand(a:addon)
        let addon = substitute(addon, '^/\+', '', '')
        let found = ''
        " If the addon is right here, then we win
        if isdirectory(from . '/' . addon)
            let found = from . '/' . addon
        else
            let dirs = split(from, '/')
        if !has('win32') && !has('win64')
          let dirs[0] = '/' . dirs[0]
        endif
            " Walk up the tree and see if it's anywhere there
            for n in range(len(dirs) - 1, 0, -1)
                let path = join(dirs[0:n], '/')
                if isdirectory(path . '/' . addon)
                    let found = path . '/' . addon
                    break
                endif
            endfor
        endif
        " If we found it, then let's see if we can go deeper
        "
        " For example, we may have found component_name/include
        " but what if that directory only has a single directory
        " in it, and that subdirectory only has a single directory
        " in it, etc... ?  This can happen when you're segmenting
        " by namespace like this:
        "
        "    component_name/include/org/vim/CoolClass.h
        "
        " You may find yourself always typing '' from the
        " 'include' directory just to go into 'org/vim' so let's
        " just eliminate the need to hit the ''.
        if found != ''
            let tempfrom = found
            let globbed = globpath(tempfrom, '*')
            while len(split(globbed, "\n")) == 1
                let tempfrom = globbed
                let globbed = globpath(tempfrom, '*')
            endwhile
            let found = scala#SanitizeDirForFuzzyFinder(tempfrom) . '/'
        else
            let found = from
        endif

        return found
    endfunction

    "
    " GetTestDirForFuzzyFinder()
    "
    " Now overload GetDirForFuzzyFinder() specifically for the test directory (I'm
    " really only interested in going down into test/src 90% of the time, so let's
    " hit that 90% and leave the other 10% to couple of extra keystrokes)
    "
    function! scala#GetTestDirForFuzzyFinder(from)
        return scala#GetDirForFuzzyFinder(a:from, 'src/test/scala/')
    endfunction

    "
    " GetMainDirForFuzzyFinder()
    "
    " Now overload GetDirForFuzzyFinder() specifically for the main directory.
    "
    function! scala#GetMainDirForFuzzyFinder(from)
        return scala#GetDirForFuzzyFinder(a:from, 'src/main/scala/')
    endfunction

    "
    " GetRootDirForFuzzyFinder()
    "
    " Now overload GetDirForFuzzyFinder() specifically for the root directory.
    "
    function! scala#GetRootDirForFuzzyFinder(from)
        return scala#GetDirForFuzzyFinder(a:from, 'src/../')
    endfunction

    " If you want to disable the default key mappings, write the following line in
    " your ~/.vimrc
    "     let g:scala_use_default_keymappings = 0
    if get(g:, 'scala_use_default_keymappings', 1)
      nnoremap <buffer> <silent> <Leader>ft :FufFile <c-r>=scala#GetTestDirForFuzzyFinder('%:p:h')<cr><cr>
      nnoremap <buffer> <silent> <Leader>fs :FufFile <c-r>=scala#GetMainDirForFuzzyFinder('%:p:h')<cr><cr>
      nnoremap <buffer> <silent> <Leader>fr :FufFile <c-r>=scala#GetRootDirForFuzzyFinder('%:p:h')<cr><cr>
    endif
endif

function! s:CreateOrExpression(keywords)
  return '('.join(a:keywords, '|').')'
endfunction

function! s:NextSection(backwards)
  if a:backwards
    let dir = '?'
  else
    let dir = '/'
  endif
  let keywords = [ 'def', 'class', 'trait', 'object' ]
  let keywordsOrExpression = s:CreateOrExpression(keywords)

  let modifiers = [ 'public', 'private', 'private\[\w*\]', 'protected', 'abstract', 'case', 'override', 'implicit', 'final', 'sealed']
  let modifierOrExpression = s:CreateOrExpression(modifiers)

  let regex = '^ *('.modifierOrExpression.' )* *'.keywordsOrExpression."\r"
  execute 'silent normal! ' . dir . '\v'.regex
endfunction

noremap <script> <buffer> <silent> ]] :call <SID>NextSection(0)<cr>
noremap <script> <buffer> <silent> [[ :call <SID>NextSection(1)<cr>

" vim:set sw=2 sts=2 ts=8 et:
