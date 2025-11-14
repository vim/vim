"------------------------------------------------------------------------------
"  Description: Vim Ada omnicompletion file
"     Language:	Ada (2022)
"    Copyright: Copyright (C) 2006 … 2022 Martin Krischik
"   Maintainer:	Doug Kearns <dougkearns@gmail.com> (Vim)
"		Martin Krischik <krischik@users.sourceforge.net> (Upstream)
"               Bartek Jasicki <thindil@laeran.pl>
"	   URL: https://github.com/krischik/vim-ada
"      Version: 5.5.0
"      History: 24.05.2006 MK Unified Headers
"		26.05.2006 MK improved search for begin of word.
"		16.07.2006 MK Ada-Mode as vim-ball
"		15.10.2006 MK Bram's suggestion for runtime integration
"		05.11.2006 MK Bram suggested not to use include protection for
"			      autoload
"		05.11.2006 MK Bram suggested against using setlocal omnifunc 
"		05.11.2006 MK Bram suggested to save on spaces
"		28.08.2022 MK Merge Ada 2012 changes from thindil
"		01.09.2022 MK Use GitHub and dein to publish new versions
"		25.10.2022 MK Add Alire compiler support
"               21.08.2023 MK Release 5.5.0
"	 Usage: Use dein to install
"    Help Page: ft-ada-omni
"------------------------------------------------------------------------------

if version < 700
   finish
endif

" Section: adacomplete#Complete () {{{1
"
" This function is used for the 'omnifunc' option.
"
function! adacomplete#Complete (findstart, base)
   if a:findstart == 1
      return ada#User_Complete (a:findstart, a:base)
   else
      "
      " look up matches
      "
      if exists ("g:ada_omni_with_keywords")
	 call ada#User_Complete (a:findstart, a:base)
      endif
      "
      "  search tag file for matches
      "
      let l:Pattern  = '^' . a:base . '.*$'
      let l:Tag_List = taglist (l:Pattern)
      "
      " add symbols
      "
      for Tag_Item in l:Tag_List
	 if !has_key(l:Tag_Item, 'language') || l:Tag_Item['language'] == 'Ada'
	    "
	    " Tag created by ctags
	    "
	    let l:Info	= 'Symbol                : ' . l:Tag_Item['name']  . "\n"
	    let l:Info .= 'Of type               : ' . g:ada#Ctags_Kinds[l:Tag_Item['kind']][1]  . "\n"
	    let l:Info .= 'Defined in File       : ' . l:Tag_Item['filename'] . "\n"

	    if has_key( l:Tag_Item, 'package')
	       let l:Info .= 'Package               : ' . l:Tag_Item['package'] . "\n"
	       let l:Menu  = l:Tag_Item['package']
	    elseif has_key( l:Tag_Item, 'separate')
	       let l:Info .= 'Separate from Package : ' . l:Tag_Item['separate'] . "\n"
	       let l:Menu  = l:Tag_Item['separate']
	    elseif has_key( l:Tag_Item, 'packspec')
	       let l:Info .= 'Package Specification : ' . l:Tag_Item['packspec'] . "\n"
	       let l:Menu  = l:Tag_Item['packspec']
	    elseif has_key( l:Tag_Item, 'type')
	       let l:Info .= 'Datetype              : ' . l:Tag_Item['type'] . "\n"
	       let l:Menu  = l:Tag_Item['type']
	    else
	       let l:Menu  = l:Tag_Item['filename']
	    endif

	    let l:Definition = trim(l:Tag_Item['cmd'], "/^")
	    let l:Definition = trim(l:Definition, "$/;")
	    let l:Definition = trim(l:Definition)
	    let l:Info .= 'Definition            : ' . l:Definition . "\n"

	    let l:Match_Item = {
	       \ 'word':  l:Tag_Item['name'],
	       \ 'abbr':  l:Definition,
	       \ 'menu':  l:Menu,
	       \ 'info':  l:Info,
	       \ 'kind':  l:Tag_Item['kind'],
	       \ 'icase': 1}
	 endif
	 if complete_add (l:Match_Item) == 0
	    return []
	 endif
	 if complete_check ()
	    return []
	 endif
      endfor
      return []
   endif
endfunction adacomplete#Complete

finish " }}}1

"------------------------------------------------------------------------------
"   Vim is Charityware - see ":help license" or uganda.txt for licence details.
"------------------------------------------------------------------------------
" vim: set textwidth=78 wrap tabstop=8 shiftwidth=3 softtabstop=3 noexpandtab :
" vim: set filetype=vim fileencoding=utf-8 fileformat=unix foldmethod=marker :
" vim: set spell spelllang=en_gb :
