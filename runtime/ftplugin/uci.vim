" Vim ftplugin file
" Language:	OpenWrt Unified Configuration Interface
" Maintainer:	Colin Caine <complaints@cmcaine.co.uk>
" Upstream:	https://github.com/cmcaine/vim-uci
" Last Change:	2024 Apr 4
"
" For more information on uci, see https://openwrt.org/docs/guide-user/base-system/uci

if exists("b:did_ftplugin")
  finish
endif
let b:did_ftplugin = 1

" UCI files are indented with tabs.
setl noet
setl commentstring=#\ %s

let b:undo_ftplugin = "setlocal et< cms<"
