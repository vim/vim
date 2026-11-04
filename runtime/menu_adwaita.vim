" Vim support file to define the main menu of the header bar (GTK 4 with
" libadwaita), like the main menu of GNOME applications.
" This file is sourced by menu.vim, which takes care of 'cpoptions'.
"
" Maintainer:	The Vim Project <https://github.com/vim/vim>
" Last Change:	2026 Nov 03

" Ask before throwing away the changes in the current buffer.  A buffer without
" a file name is emptied, there is no file to read again.
func! s:DiscardChanges()
  let has_name = bufname('%') != ''
  let msg = has_name ? gettext("Discard all changes and read the file again?")
        \ : gettext("Discard all changes?")
  if &modified && confirm(msg, gettext("&Cancel\n&Discard Changes"), 1,
        \ "Warning") != 2
    return
  endif
  if has_name
    edit!
  else
    enew!
  endif
endfunc

if has("job")
  an 1.10 ]HeaderBar.&New\ Window		:call job_start([v:progpath, "-g"])<CR>
  an 1.15 ]HeaderBar.-SEP1-			<Nop>
endif
an <silent> 1.20 ]HeaderBar.&Save<Tab>:w	:if expand("%") == ""<Bar>browse confirm w<Bar>else<Bar>confirm w<Bar>endif<CR>
an 1.30 ]HeaderBar.Save\ &As\.\.\.<Tab>:sav	:browse confirm saveas<CR>
an <silent> 1.40 ]HeaderBar.&Discard\ Changes<Tab>:e!	:call <SID>DiscardChanges()<CR>
an 1.45 ]HeaderBar.-SEP2-			<Nop>
an 1.48 ]HeaderBar.&Find\.\.\.		:promptfind<CR>
an 1.50 ]HeaderBar.Find\ and\ Rep&lace\.\.\.	:promptrepl<CR>
if has("printer")
  an 1.60 ]HeaderBar.&Print			:hardcopy<CR>
endif
an 1.65 ]HeaderBar.-SEP3-			<Nop>
an <silent> 1.70 ]HeaderBar.F&ullscreen	:if &go =~# 's'<Bar>set go-=s<Bar>else<Bar>set go+=s<Bar>endif<CR>
an 1.75 ]HeaderBar.-SEP4-			<Nop>
" The menus of Vim are added after this, see main_menu_create_popup_cb() in
" src/gui_gtk4_adw.c.
