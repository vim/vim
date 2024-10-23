" Vim filetype plugin file
" Language:	Vivado mss file
" Maintainer:	The Vim Project <https://github.com/vim/vim>
" Last Change:	2024 Oct 22
" Maintainer:	Wu, Zhenyu <wuzhenyu@ustc.edu>

if exists("b:did_ftplugin")
	finish
endif
let b:did_ftplugin = 1

let b:match_words = '\<BEGIN\>:\<END\>'
