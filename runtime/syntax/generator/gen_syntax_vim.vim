" Vim syntax file generator
" Language: Vim script
" Maintainer: Hirohito Higashi (h_east)
" Last Change: 2025 Mar 09

let s:keepcpo= &cpo
set cpo&vim

language C
let s:log_write_dir = getcwd() . '/'

function s:parse_vim_option(opt, missing_opt, term_out_code)
	try
		let file_name = $VIM_SRCDIR . '/optiondefs.h'
		let item = {}

		new
		exec 'read ' . file_name
		norm! gg
		exec '/^.*\s*options\[\]\s*=\s*$/+1;/^\s*#\s*define\s*p_term(/-1yank a'
		exec '/^#define\s\+p_term(/+1;/^};$/-1yank b'
		%delete _

		put a
		" workaround for 'shortname'
		g/^#\s*ifdef\s*SHORT_FNAME\>/j
		g/^#/d
		g/^\s*{\s*"\w\+"\%(\s*,\s*[^,]*\)\{2}[^,]$/j
		g/^\s*{\s*"\w\+"\s*,.*$/j
		g!/^\s*{\s*"\w\+"\s*,.*$/d

		for line in getline(1, line('$'))
			let list = matchlist(line, '^\s*{\s*"\(\w\+\)"\s*,\s*\%("\(\w\+\)"\|NULL\)\s*,\s*\%([^,]*\(P_BOOL\)[^,]*\|[^,]*\)\s*,\s*\([^,]*NULL\)\?.*')
			let item.name = list[1]
			let item.short_name = list[2]
			let item.is_bool = empty(list[3]) ? 0 : 1
			if empty(list[4])
				call add(a:opt, copy(item))
			else
				call add(a:missing_opt, copy(item))
			endif
		endfor
		if empty(a:opt)
			throw 'opt is empty'
		endif
		if empty(a:missing_opt)
			throw 'missing_opt is empty'
		endif

		%delete _
		put b
		g!/^\s*p_term(\s*"\w\+"\s*,.*$/d

		for line in getline(1, line('$'))
			let list = matchlist(line, '^\s*p_term(\s*"\(\w\+\)"\s*,')
			let item.name = list[1]
			call add(a:term_out_code, copy(item))
		endfor
		quit!
		if empty(a:term_out_code)
			throw 'term_out_code is empty'
		endif
	catch /.*/
		call s:err_gen('')
		throw 'exit'
	endtry
endfunc

function s:append_syn_vimopt(lnum, str_info, opt_list, prefix, bool_only)
	let ret_lnum = a:lnum
	let str = a:str_info.start

	for o in a:opt_list
		if !a:bool_only || o.is_bool
			if !empty(o.short_name)
				let str .= ' ' . a:prefix . o.short_name
			endif
			let str .= ' ' . a:prefix . o.name
			if len(str) > s:line_break_len
				if !empty(a:str_info.end)
					let str .= ' ' . a:str_info.end
				endif
				call append(ret_lnum, str)
				let str = a:str_info.start
				let ret_lnum += 1
			endif
		endif
	endfor
	if str !=# a:str_info.start
		if !empty(a:str_info.end)
			let str .= ' ' . a:str_info.end
		endif
		call append(ret_lnum, str)
		let ret_lnum += 1
	endif
	return ret_lnum
endfunc

" ------------------------------------------------------------------------------
function s:parse_vim_command(cmd)
	try
		let file_name = $VIM_SRCDIR . '/ex_cmds.h'
		let item = {}

		new
		exec 'read ' . file_name
		norm! gg
		exec '/^}\?\s*cmdnames\[\]\s*=\s*$/+1;/^};/-1yank'
		%delete _
		put
		g!/^EXCMD(/d

		let lcmd = {}
		for key in range(char2nr('a'), char2nr('z'))
			let lcmd[nr2char(key)] = []
		endfor
		let lcmd['~'] = []

		for line in getline(1, line('$'))
			let list = matchlist(line, '^EXCMD(\w\+\s*,\s*"\(\a\w*\)"\s*,')
			if !empty(list)
				" Small ascii character or other.
				let key = (list[1][:0] =~# '\l') ? list[1][:0] : '~'
				call add(lcmd[key], list[1])
			endif
		endfor
		quit!

		for key in sort(keys(lcmd))
			for my in range(len(lcmd[key]))
				let omit_idx = 0
				if my > 0
					let omit_idx = (key =~# '\l') ? 1 : 0
					for idx in range(1, strlen(lcmd[key][my]))
						let matched = 0
						for pre in range(my - 1, 0, -1)
							" Avoiding conflicts shortened command and special commands
							" - weird abbreviations for delete. (See :help :d)
							" - k{char} is used as mark. (See :help :k)
							" - :s commsnds repeat. (See :help :substitute-repeat)
							if lcmd[key][my][:idx] ==# lcmd[key][pre][:idx] ||
							\	(key ==# 'd' &&
							\		lcmd[key][my][:idx] =~# '^d\%[elete][lp]$')
							\	|| (key ==# 'k' &&
							\		lcmd[key][my][:idx] =~# '^k[a-zA-Z]$')
							\	|| (key ==# 's' &&
							\		lcmd[key][my][:idx] =~# '^s\%(c\%([^sr][^ip]\=\)\=$\|g\|i[^mlg]\=$\|I\|r[^e]\=$\)')
								let matched = 1
								let omit_idx = idx + 1
								break
							endif
						endfor
						if !matched
							break
						endif
					endfor
				endif

				let item.name = lcmd[key][my]
				let item.type = s:get_vim_command_type(item.name)
				if omit_idx + 1 < strlen(item.name)
					let item.omit_idx = omit_idx
					let item.syn_str = item.name[:omit_idx] . '[' . 
					\		item.name[omit_idx+1:] . ']'
				else
					let item.omit_idx = -1
					let item.syn_str = item.name
				endif
				call add(a:cmd, copy(item))
			endfor
		endfor

		" Add weird abbreviations for delete. (See :help :d)
		for i in ['l', 'p']
			let str = 'delete'
			let item.name = str . i
			let item.type = s:get_vim_command_type(item.name)
			let item.omit_idx = -1
			for x in range(strlen(str))
				let item.syn_str = str[:x] . i
				if item.syn_str !=# "del"
					call add(a:cmd, copy(item))
				endif
			endfor
		endfor

		" Required for original behavior
		let item.name = 'a'		" append
		let item.type = 0
		let item.omit_idx = -1
		let item.syn_str = item.name
		call add(a:cmd, copy(item))
		let item.name = 'i'		" insert
		let item.syn_str = item.name
		call add(a:cmd, copy(item))

		let no_shorten_in_vim9 =<< trim EOL
			final
			def
			enddef
			class
			endclass
			enum
			endenum
			interface
			endinterface
			abstract
			public
			static
			this
			var
			type
		EOL

		call map(a:cmd, {_, v ->
			\ index(no_shorten_in_vim9, v.name) != -1 ?
			\		extend(copy(v), {'omit_idx': -1, 'syn_str': v.name}) :
			"\ ":fina" means ":finally" in legacy script, for backwards compatibility.
			"\ (From Vim source code find_ex_command() in ex_docmd.c)
			\ v.name ==# 'finally' ?
			\		extend(copy(v), {'omit_idx': 3, 'syn_str': 'fina[lly]'}) :
			"\ :ho must not be recognized as :horizontal.
			\ v.name ==# 'horizontal' ?
			\		extend(copy(v), {'omit_idx': 2, 'syn_str': 'hor[izontal]'}) :
			\ v
			\ })

		if empty(a:cmd)
			throw 'cmd is empty'
		endif
	catch /.*/
		call s:err_gen('')
		throw 'exit'
	endtry
endfunc

function s:get_vim_command_type(cmd_name)
	" Return value:
	"   0: normal
	"   1: (Reserved)
	"   2: abbrev (without un)
	"   3: menu
	"   4: map
	"   5: mapclear
	"   6: unmap
	"   7: abclear
	"   99: (Exclude registration of "syn keyword")
	let ab_prefix   = '^[ci]\?'
	let menu_prefix = '^\%([acinostvx]\?\|tl\)'
	let map_prefix  = '^[acilnostvx]\?'
	let exclude_list =<< trim EOL
		2match
		3match
		Next
		Print
		X
		abstract
		append
		augroup
		augroup
		autocmd
		behave
		call
		catch
		class
		debuggreedy
		def
		delcommand
		doautoall
		doautocmd
		echo
		echoconsole
		echoerr
		echohl
		echomsg
		echon
		echowindow
		elseif
		endclass
		enddef
		endenum
		endfunction
		endinterface
		enum
		execute
		export
		final
		for
		function
		if
		interface
		insert
		let
		loadkeymap
		map
		mapclear
		match
		noremap
		new
		normal
		popup
		public
		redir
		return
		set
		setglobal
		setlocal
		sleep
		smagic
		snomagic
		static
		substitute
		syntax
		this
		throw
		type
		unlet
		unmap
		var
		vim9script
		while
	EOL
	" Required for original behavior
	" \	'global', 'vglobal'

	if index(exclude_list, a:cmd_name) != -1
		let ret = 99
	elseif a:cmd_name =~# '^\%(\%(un\)\?abbreviate\|noreabbrev\|\l\%(nore\|un\)\?abbrev\)$'
		let ret = 2
	elseif a:cmd_name =~# ab_prefix . 'abclear$'
		let ret = 7
	elseif a:cmd_name =~# menu_prefix . '\%(nore\|un\)\?menu$'
		let ret = 3
	elseif a:cmd_name =~# map_prefix . '\%(nore\)\?map$'
		let ret = 4
	elseif a:cmd_name =~# map_prefix . 'mapclear$'
		let ret = 5
	elseif a:cmd_name =~# map_prefix . 'unmap$'
		let ret = 6
	else
		let ret = 0
	endif
	return ret
endfunc

function s:append_syn_vimcmd(lnum, str_info, cmd_list, type)
	let ret_lnum = a:lnum
	let str = a:str_info.start

	for o in a:cmd_list
		if o.type == a:type
			let str .= ' ' . o.syn_str
			if len(str) > s:line_break_len
				if !empty(a:str_info.end)
					let str .= ' ' . a:str_info.end
				endif
				call append(ret_lnum, str)
				let str = a:str_info.start
				let ret_lnum += 1
			endif
		endif
	endfor
	if str !=# a:str_info.start
		if !empty(a:str_info.end)
			let str .= ' ' . a:str_info.end
		endif
		call append(ret_lnum, str)
		let ret_lnum += 1
	endif
	return ret_lnum
endfunc

" ------------------------------------------------------------------------------
function s:parse_vim_event(li)
	try
		let file_name = $VIM_SRCDIR . '/autocmd.c'
		let item = {}

		new
		exec 'read ' . file_name
		norm! gg
		exec '/^static keyvalue_T event_tab\[NUM_EVENTS] = {$/+1;/^};$/-1yank'
		%delete _

		put
		g!/^\s*KEYVALUE_ENTRY(/d

		for line in getline(1, line('$'))
			let list = matchlist(line, '^\s*KEYVALUE_ENTRY(-\?EVENT_\w\+,\s*"\(\w\+\)"')
			let item.name = list[1]
			call add(a:li, copy(item))
		endfor

		quit!

		if empty(a:li)
			throw 'event is empty'
		endif
	catch /.*/
		call s:err_gen('')
		throw 'exit'
	endtry
endfunc

" ------------------------------------------------------------------------------
function s:parse_vim_function(li)
	try
		let file_name = $VIM_SRCDIR . '/evalfunc.c'
		let item = {}

		new
		exec 'read ' . file_name
		norm! gg
		exec '/^static\s\+funcentry_T\s\+global_functions\[\]\s*=\s*$/+1;/^};/-1yank'
		%delete _

		put
		g!/^\s*{\s*"\w\+"\s*,.*$/d
		g/^\s*{\s*"test"\s*,.*$/d
		g@//\s*obsolete@d
		g@/\*\s*obsolete\s*\*/@d

		for line in getline(1, line('$'))
			let list = matchlist(line, '^\s*{\s*"\(\w\+\)"\s*,')
			let item.name = list[1]
			call add(a:li, copy(item))
		endfor

		quit!

		if empty(a:li)
			throw 'function is empty'
		endif
	catch /.*/
		call s:err_gen('')
		throw 'exit'
	endtry
endfunc

" ------------------------------------------------------------------------------
function s:parse_vim_hlgroup(li)
	try
		let file_name = $VIM_SRCDIR . '/highlight.c'
		let item = {}

		new
		exec 'read ' . file_name
		call cursor(1, 1)
		exec '/^static\s\+char\s\+\*(highlight_init_both\[\])\s*=\%(\s*{\)\?$/+1;/^\s*};/-1yank a'
		exec '/^static\s\+char\s\+\*(highlight_init_light\[\])\s*=\%(\s*{\)\?$/+1;/^\s*};/-1yank b'
		exec '/^set_normal_colors(\%(void\)\?)$/+1;/^}$/-1yank d'
		%delete _
		put a
		for line in getline(1, line('$'))
			let list = matchlist(line, '^\s*\%(CENT(\)\?"\%(default\s\+link\s\+\)\?\(\a\+\).*",.*')
			if !empty(list)
				let item.name = list[1]
				let item.type = 'both'
				call add(a:li, copy(item))
			endif
		endfor

		%delete _
		put b
		for line in getline(1, line('$'))
			let list = matchlist(line, '^\s*\%(CENT(\)\?"\%(default\s\+link\s\+\)\?\(\a\+\).*",.*')
			if !empty(list)
				let item.name = list[1]
				let item.type = 'light'
				call add(a:li, copy(item))
			endif
		endfor

		%delete _
		put d
		for line in getline(1, line('$'))
			let list = matchlist(line, '^\s*if\s*(set_group_colors(.*"\(\a\+\)",')
			if !empty(list) && list[1] !=# 'Normal'
				let item.name = list[1]
				let item.type = 'gui'
				call add(a:li, copy(item))
			endif
		endfor

		let item.name = 'CursorIM'
		let item.type = 'gui'
		call add(a:li, copy(item))

		" The following highlight groups cannot be extracted from highlight.c
		" (TODO: extract from HIGHLIGHT_INIT ?)
		let item.name = 'LineNrAbove'
		let item.type = 'both'
		call add(a:li, copy(item))

		let item.name = 'LineNrBelow'
		let item.type = 'both'
		call add(a:li, copy(item))

		" "Conceal" is an option and cannot be used as keyword, so remove it.
		" (Separately specified as 'syn match' in vim.vim.base).
		call filter(a:li, {idx, val -> val.name !=# 'Conceal'})

		quit!

		if empty(a:li)
			throw 'hlgroup is empty'
		endif
	catch /.*/
		call s:err_gen('')
		throw 'exit'
	endtry
endfunc

" ------------------------------------------------------------------------------
function s:parse_vim_complete_name(li)
	try
		let file_name = $VIM_SRCDIR . '/usercmd.c'
		let item = {}

		new
		exec 'read ' . file_name
		norm! gg
		exec '/^static keyvalue_T command_complete_tab\[] =$/+1;/^};$/-1yank'
		%delete _

		put
		g!/^\s*KEYVALUE_ENTRY(/d
		g/"custom\(list\)\?"/d

		for line in getline(1, line('$'))
			let list = matchlist(line, '^\s*KEYVALUE_ENTRY(EXPAND_\w\+,\s*"\(\w\+\)"')
			let item.name = list[1]
			call add(a:li, copy(item))
		endfor

		quit!

		if empty(a:li)
			throw 'complete_name is empty'
		endif
	catch /.*/
		call s:err_gen('')
		throw 'exit'
	endtry
endfunc

" ------------------------------------------------------------------------------
function s:parse_vim_addr_name(li)
	try
		let file_name = $VIM_SRCDIR . '/usercmd.c'
		let item = {}

		new
		exec 'read ' . file_name
		norm! gg
		exec '/^static addrtype_T addr_type_complete_tab\[] =$/+1;/^};$/-1yank'
		%delete _

		put
		g!/^\s*ADDRTYPE_ENTRY(/d

		for line in getline(1, line('$'))
			let list = matchlist(line, '^\s*ADDRTYPE_ENTRY(ADDR_\w\+,\s*"\(\w\+\)",\s*"\(.*\)"')
			let item.name = list[1]
			call add(a:li, copy(item))
			let item.name = list[2]
			call add(a:li, copy(item))
		endfor

		" '?' is not in 'iskeyword' and cannot be used as keyword, so remove it.
		" (Separately specified as 'syn match' in vim.vim.base).
		call filter(a:li, {idx, val -> val.name !=# '?'})

		quit!

		if empty(a:li)
			throw 'addr_name is empty'
		endif
	catch /.*/
		call s:err_gen('')
		throw 'exit'
	endtry
endfunc

" ------------------------------------------------------------------------------
function s:parse_vim_var(li)
	try
		let file_name = $VIM_SRCDIR . '/evalvars.c'
		let item = {}

		new
		exec 'read ' . file_name
		norm! gg
		exec '/^} vimvars\[VV_LEN] =\n{$/+1;/^};$/-1yank'
		%delete _

		put
		g!/^\s*{VV_NAME(/d

		for line in getline(1, line('$'))
			let list = matchlist(line, '^\s*{VV_NAME("\(\w\+\)"')
			let item.name = list[1]
			call add(a:li, copy(item))
		endfor

		quit!

		if empty(a:li)
			throw 'var is empty'
		endif
	catch /.*/
		call s:err_gen('')
		throw 'exit'
	endtry
endfunc

" ------------------------------------------------------------------------------
function s:append_syn_any(lnum, str_info, li)
	let ret_lnum = a:lnum
	let str = a:str_info.start

	for o in a:li
		let str .= ' ' . o.name
		if len(str) > s:line_break_len
			if !empty(a:str_info.end)
				let str .= ' ' . a:str_info.end
			endif
			call append(ret_lnum, str)
			let str = a:str_info.start
			let ret_lnum += 1
		endif
	endfor
	if str !=# a:str_info.start
		if !empty(a:str_info.end)
			let str .= ' ' . a:str_info.end
		endif
		call append(ret_lnum, str)
		let ret_lnum += 1
	endif
	return ret_lnum
endfunc

" ------------------------------------------------------------------------------
function s:update_syntax_vim_file(vim_info)
	try
		function! s:search_and_check(kword, base_fname, str_info)
			let a:str_info.start = ''
			let a:str_info.end = ''

			let pattern = '^" GEN_SYN_VIM: ' . a:kword . '\s*,'
			let lnum = search(pattern)
			if lnum == 0
				throw 'Search pattern ''' . pattern . ''' not found in ' .
				\		a:base_fname
			endif
			let li = matchlist(getline(lnum), pattern . '\s*START_STR\s*=\s*''\(.\{-}\)''\s*,\s*END_STR\s*=\s*''\(.\{-}\)''')
			if empty(li)
				throw 'Bad str_info line:' . getline(lnum)
			endif
			let a:str_info.start = li[1]
			let a:str_info.end = li[2]
			return lnum
		endfunc

		let target_fname = 'vim.vim.rc'
		let base_fname = 'vim.vim.base'
		let str_info = {}
		let str_info.start = ''
		let str_info.end = ''

		new
		exec 'edit ' . target_fname
		%d _
		exec 'read ' . base_fname
		1delete _
		call cursor(1, 1)

		" vimCommand
		let li = a:vim_info.cmd
		" vimCommand - normal
		let lnum = s:search_and_check('vimCommand normal', base_fname, str_info)
		let lnum = s:append_syn_vimcmd(lnum, str_info, li, 0)

		" vimOption
		let kword = 'vimOption'
		let li = a:vim_info.opt
		" vimOption - normal
		let lnum = s:search_and_check(kword . ' normal', base_fname, str_info)
		let lnum = s:append_syn_vimopt(lnum, str_info, li, '', 0)
		" vimOption - turn-off
		let lnum = s:search_and_check(kword . ' turn-off', base_fname, str_info)
		let lnum = s:append_syn_vimopt(lnum, str_info, li, 'no', 1)
		" vimOption - invertible
		let lnum = s:search_and_check(kword . ' invertible', base_fname, str_info)
		let lnum = s:append_syn_vimopt(lnum, str_info, li, 'inv', 1)
		" vimOption - term output code
		let li = a:vim_info.term_out_code
		let lnum = s:search_and_check(kword . ' term output code', base_fname, str_info)
		let lnum = s:append_syn_any(lnum, str_info, li)

		" vimOption - normal variable
		let li = a:vim_info.opt
		let lnum = s:search_and_check(kword . ' normal variable', base_fname, str_info)
		let lnum = s:append_syn_vimopt(lnum, str_info, li, '', 0)
		" vimOption - term output code variable
		let li = a:vim_info.term_out_code
		let lnum = s:search_and_check(kword . ' term output code variable', base_fname, str_info)
		let lnum = s:append_syn_any(lnum, str_info, li)

		" Missing vimOption
		let li = a:vim_info.missing_opt
		let lnum = s:search_and_check('Missing vimOption', base_fname, str_info)
		let lnum = s:append_syn_vimopt(lnum, str_info, li, '', 0)
		let lnum = s:append_syn_vimopt(lnum, str_info, li, 'no', 1)
		let lnum = s:append_syn_vimopt(lnum, str_info, li, 'inv', 1)

		" vimAutoEvent
		let li = a:vim_info.event
		let lnum = s:search_and_check('vimAutoEvent', base_fname, str_info)
		let lnum = s:append_syn_any(lnum, str_info, li)

		" vimHLGroup
		let li = a:vim_info.hlgroup
		let lnum = s:search_and_check('vimHLGroup', base_fname, str_info)
		let lnum = s:append_syn_any(lnum, str_info, li)

		" vimFuncName
		let li = a:vim_info.func
		let lnum = s:search_and_check('vimFuncName', base_fname, str_info)
		let lnum = s:append_syn_any(lnum, str_info, li)

		" vimVarName
		let li = a:vim_info.var
		let lnum = s:search_and_check('vimVarName', base_fname, str_info)
		let lnum = s:append_syn_any(lnum, str_info, li)

		" vimUserAttrbCmplt
		let li = a:vim_info.compl_name
		let lnum = s:search_and_check('vimUserCmdAttrCmplt', base_fname, str_info)
		let lnum = s:append_syn_any(lnum, str_info, li)

		" vimUserAttrbAddr
		let li = a:vim_info.addr_name
		let lnum = s:search_and_check('vimUserCmdAttrAddr', base_fname, str_info)
		let lnum = s:append_syn_any(lnum, str_info, li)

		" vimCommand - abbrev
		let kword = 'vimCommand'
		let li = a:vim_info.cmd
		let lnum = s:search_and_check(kword . ' abbrev', base_fname, str_info)
		let lnum = s:append_syn_vimcmd(lnum, str_info, li, 2)
		let lnum = s:search_and_check(kword . ' abclear', base_fname, str_info)
		let lnum = s:append_syn_vimcmd(lnum, str_info, li, 7)
		" vimCommand - map
		let lnum = s:search_and_check(kword . ' map', base_fname, str_info)
		let lnum = s:append_syn_vimcmd(lnum, str_info, li, 4)
		let lnum = s:search_and_check(kword . ' mapclear', base_fname, str_info)
		let lnum = s:append_syn_vimcmd(lnum, str_info, li, 5)
		let lnum = s:search_and_check(kword . ' unmap', base_fname, str_info)
		let lnum = s:append_syn_vimcmd(lnum, str_info, li, 6)
		" vimCommand - menu
		let lnum = s:search_and_check(kword . ' menu', base_fname, str_info)
		let lnum = s:append_syn_vimcmd(lnum, str_info, li, 3)

		update
		quit!

	catch /.*/
		call s:err_gen('')
		throw 'exit'
	endtry
endfunc

" ------------------------------------------------------------------------------
function s:check_help_doc(vim_info)
	try
		new
		let cwd_save = getcwd()
		cd ../../../runtime/doc

		let exclude_cmd =<< trim END
			deletel
			deletep
			a
			i
		END

		let nocheck_shorten_excmd_list =<< trim END
			bufdo
			cfdo
			cstag
			debug
			defer
			eval
			intro
			lfdo
			luado
			luafile
			ownsyntax
			py3do
			pydo
			pyxdo
			pyxfile
			rundo
			smile
			syntime
			windo
			wundo
		END

		" Check the Ex-command is listed in index.txt
		split index.txt
		for vimcmd in a:vim_info.cmd
			if index(exclude_cmd, vimcmd.name) != -1
				continue
			endif
			norm! gg
			let find_ptn = '^|:' . vimcmd.name . '|\s\+'
			let lnum = search(find_ptn, 'eW')
			if lnum == 0
				call s:err_sanity($'Ex-cmd ":{vimcmd.name}" is not found in index.txt.')
			elseif search(find_ptn, 'eW') > 0
				call s:err_sanity($'Ex-cmd ":{vimcmd.name}" is duplicated in index.txt.')
			else
				let doc_syn_str = substitute(getline(lnum), find_ptn . ':\(\S\+\)\s\+.*', '\1', '')
				if doc_syn_str !=# vimcmd.syn_str
					call s:err_sanity($'Ex-cmd "{vimcmd.name}" short name differ in index.txt. expect: "{vimcmd.syn_str}", but: "{doc_syn_str}"')
				endif
			endif
		endfor
		quit!

		" Check the existence of the help tag for Ex-command.
		set wildignore=version*.txt,todo.txt,usr_*.txt
		for vimcmd in a:vim_info.cmd
			if index(exclude_cmd, vimcmd.name) != -1
				continue
			endif
			let find_ptn = '\s\*:' . vimcmd.name . '\*\_s'
			exec "silent! vimgrep /" . find_ptn . "/gj *.txt"
			let qfl = getqflist()
			if empty(qfl)
				call s:err_sanity($'Help tag for Ex-cmd ":{vimcmd.name}" not found.')
			elseif len(qfl) > 1
				call s:err_sanity($'Help tag for Ex-cmd ":{vimcmd.name}" is duplicated.')
			elseif index(nocheck_shorten_excmd_list, vimcmd.name) ==# -1
				" Check the existence of the shorten Ex-command notation.
				cc
				norm! 2k
				let end_lnum = qfl[0].lnum + 10
				let find_ptn = '^:.*\<' . vimcmd.syn_str->escape('[]')
				let lnum = search(find_ptn, 'W', end_lnum)
				if lnum == 0
					if vimcmd.omit_idx != -1
						" Check the existence of the shorten help tag for Ex-command.
						cc
						norm! k
						let end_lnum = qfl[0].lnum + 10
						let find_ptn = '\s\*:' . vimcmd.name[:vimcmd.omit_idx] . '\*\_s'
						let lnum = search(find_ptn, 'W', end_lnum)
					else
						let lnum = 1
					endif
					if lnum == 0
						call s:err_sanity($'Shorten help tag "{vimcmd.name[:vimcmd.omit_idx]}" for Ex-cmd "{vimcmd.name}" not found.')
					endif
				endif
			endif
		endfor
	catch /.*/
		call s:err_gen('')
		throw 'exit'
	finally
		call s:err_gen('Ex-cmd documentation consistency check completed.')
		exec 'cd ' . cwd_save
		set wildignore&
	endtry
endfunc

" ------------------------------------------------------------------------------
function s:err_gen(arg)
	call s:write_error(a:arg, s:log_write_dir .. 'generator.err')
endfunc

function s:err_sanity(arg)
	call s:write_error(a:arg, s:log_write_dir .. 'sanity_check.err')
endfunc

function s:write_error(arg, fname)
	let li = []
	if !empty(v:throwpoint)
		call add(li, v:throwpoint)
	endif
	if !empty(v:exception)
		call add(li, v:exception)
	endif
	if type(a:arg) == type([])
		call extend(li, a:arg)
	elseif type(a:arg) == type("")
		if !empty(a:arg)
			call add(li, a:arg)
		endif
	endif
	if !empty(li)
		call writefile(li, a:fname, 'a')
	else
		call writefile(['UNKNOWN'], a:fname, 'a')
	endif
endfunc

" ------------------------------------------------------------------------------
try
	let s:line_break_len = 768
	let s:vim_info = {}
	let s:vim_info.opt = []
	let s:vim_info.missing_opt = []
	let s:vim_info.term_out_code = []
	let s:vim_info.cmd = []
	let s:vim_info.event = []
	let s:vim_info.func = []
	let s:vim_info.hlgroup = []
	let s:vim_info.compl_name = []
	let s:vim_info.addr_name = []
	let s:vim_info.var = []

	set lazyredraw
	if !$CHECK_HELP_DOC
		silent call s:parse_vim_option(s:vim_info.opt, s:vim_info.missing_opt,
		\						s:vim_info.term_out_code)
		silent call s:parse_vim_command(s:vim_info.cmd)
		silent call s:parse_vim_event(s:vim_info.event)
		silent call s:parse_vim_function(s:vim_info.func)
		silent call s:parse_vim_hlgroup(s:vim_info.hlgroup)
		silent call s:parse_vim_complete_name(s:vim_info.compl_name)
		silent call s:parse_vim_addr_name(s:vim_info.addr_name)
		silent call s:parse_vim_var(s:vim_info.var)

		call s:update_syntax_vim_file(s:vim_info)
	else
		silent call s:parse_vim_command(s:vim_info.cmd)
		silent call s:check_help_doc(s:vim_info)
	endif
	set nolazyredraw

finally
	quitall!
endtry

" ---------------------------------------------------------------------
let &cpo = s:keepcpo
unlet s:keepcpo
" vim:ts=2 sw=2
