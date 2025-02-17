" Last Change:	17 Feb 2025

" Compare 2 versions
" @param v1 number[2]
" @param v2 number[2]
" @return number 1 if v1 > v2, 0 if v1 == v2, -1 if v1 < v2
func! s:compare_ver(v1, v2)
	if a:v1[0] > a:v2[0]
		return 1
	endif
	if a:v1[0] < a:v2[0]
		return -1
	endif
	return a:v1[1] == a:v2[1] ? 0 : (a:v1[1] > a:v2[1] ? 1 : -1)
endfunc

func! lua#IncludeExpr(fname)
	if !exists("g:lua_version")
		let g:lua_version = 5
	endif
	if !exists("g:lua_subversion")
		let g:lua_subversion = 3
	endif
	let l:fname = tr(a:fname, '.', '/')
	if s:compare_ver([g:lua_version, g:lua_subversion], [5, 3]) >= 0
		let l:paths = [ './' ]
	endif
	let l:paths = s:compare_ver(v1, v2) >= 0 ?  [ l:fname.'.lua', l:fname.'/init.lua' ] : [ l:fname.'.lua' ]
	for l:path in l:paths
		if filereadable(l:path)
            return l:path
        endif
	endfor
endfunc
