" This Vim script deletes all the menus, so that they can be redefined.
" Warning: This also deletes all menus defined by the user!
"
" Maintainer:	Bram Moolenaar <Bram@vim.org>
" Last Change:	2019 Sep 11

aunmenu *
tlunmenu *

silent! unlet did_install_default_menus
silent! unlet did_install_syntax_menu
if exists("did_menu_trans")
  menutrans clear
  unlet did_menu_trans
endif

silent! unlet find_help_dialog

silent! unlet menutrans_help_dialog
silent! unlet menutrans_path_dialog
silent! unlet menutrans_tags_dialog
silent! unlet menutrans_textwidth_dialog
silent! unlet menutrans_fileformat_dialog
silent! unlet menutrans_fileformat_choices
silent! unlet menutrans_no_file
silent! unlet menutrans_set_lang_to
silent! unlet menutrans_spell_change_ARG_to
silent! unlet menutrans_spell_add_ARG_to_word_list
silent! unlet menutrans_spell_ignore_ARG

" vim: set sw=2 :
