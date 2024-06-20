" PicoLisp filetype plugin file
" Language: PicoLisp
" Maintainer: nat-418 <93013864+nat-418@users.noreply.github.com>
" Latest Revision: 2024-04-10

if exists("b:did_ftplugin")
  finish
endif
let b:did_ftplugin = 1

setlocal comments=s1:#{,ex:}#,:#
setlocal commentstring=#\ %s

let b:undo_ftplugin = "setl com< cms<"
