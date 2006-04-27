" Created	: Wed 26 Apr 2006 01:20:53 AM CDT
" Modified	: Thu 27 Apr 2006 03:29:13 AM CDT
" Author	: Gautam Iyer <gi1242@users.sourceforge.net>
" Description	: ftplugin for mrxvtrc

if exists("b:did_ftplugin")
  finish
endif
let b:did_ftplugin = 1

let b:undo_ftplugin = "setl com< cms< fo<"

setlocal comments=:! commentstring=!\ %s formatoptions-=t formatoptions+=croql

