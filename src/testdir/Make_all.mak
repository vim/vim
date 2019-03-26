#
# Common Makefile, defines the list of tests to run.
#

# Options for protecting the tests against undesirable interaction with the
# environment
NO_PLUGINS = --noplugin --not-a-term
NO_INITS = -U NONE $(NO_PLUGINS)

# The first script creates small.vim.
SCRIPTS_FIRST = \
	test1.out

# Tests that run on all systems.
SCRIPTS_ALL = \
	test3.out \
	test14.out \
	test29.out \
	test37.out \
	test39.out \
	test42.out \
	test44.out \
	test48.out \
	test64.out \
	test69.out \
	test70.out \
	test88.out \
	test94.out \
	test95.out \
	test99.out \
	test108.out \
	test_eval.out


# Tests that run on most systems, but not on Amiga.
SCRIPTS_MORE1 = \
	test11.out \
	test52.out \
	test86.out \
	test87.out


# Tests that run on most systems, but not on Amiga and DOS/Windows.
SCRIPTS_MORE2 = \
	test49.out


# Tests that run on most systems, but not on VMS
SCRIPTS_MORE4 = \
	test17.out \
	test30.out \
	test59.out \
	test72.out \


# Tests specifically for MS-Windows.
SCRIPTS_WIN32 =


# Tests for the GUI.
SCRIPTS_GUI =

# Individual tests, including the ones part of test_alot.
# Please keep sorted up to test_alot.
NEW_TESTS = \
	test_arabic \
	test_arglist \
	test_assert \
	test_assign \
	test_autochdir \
	test_autocmd \
	test_autoload \
	test_backspace_opt \
	test_backup \
	test_behave \
	test_blob \
	test_blockedit \
	test_breakindent \
	test_bufline \
	test_bufwintabinfo \
	test_cd \
	test_cdo \
	test_changedtick \
	test_changelist \
	test_channel \
	test_charsearch \
	test_charsearch_utf8 \
	test_cindent \
	test_clientserver \
	test_close_count \
	test_cmdline \
	test_command_count \
	test_comparators \
	test_compiler \
	test_conceal \
	test_crypt \
	test_cscope \
	test_cursor_func \
	test_curswant \
	test_delete \
	test_diffmode \
	test_digraph \
	test_display \
	test_edit \
	test_erasebackword \
	test_escaped_glob \
	test_eval_stuff \
	test_ex_equal \
	test_ex_undo \
	test_ex_z \
	test_exec_while_if \
	test_execute_func \
	test_exists \
	test_exists_autocmd \
	test_exit \
	test_expand \
	test_expand_dllpath \
	test_expand_func \
	test_expr \
	test_expr_utf8 \
	test_feedkeys \
	test_file_perm \
	test_file_size \
	test_filechanged \
	test_fileformat \
	test_filetype \
	test_filter_cmd \
	test_filter_map \
	test_find_complete \
	test_findfile \
	test_fixeol \
	test_float_func \
	test_fnameescape \
	test_fnamemodify \
	test_fold \
	test_functions \
	test_ga \
	test_getcwd \
	test_getvar \
	test_gf \
	test_glob2regpat \
	test_global \
	test_gn \
	test_goto \
	test_gui \
	test_gui_init \
	test_hardcopy \
	test_help \
	test_help_tagjump \
	test_hide \
	test_highlight \
	test_history \
	test_hlsearch \
	test_iminsert \
	test_increment \
	test_increment_dbcs \
	test_ins_complete \
	test_job_fails \
	test_join \
	test_json \
	test_jumplist \
	test_jumps \
	test_lambda \
	test_langmap \
	test_largefile \
	test_let \
	test_lineending \
	test_lispwords \
	test_listchars \
	test_listdict \
	test_listlbr \
	test_listlbr_utf8 \
	test_lua \
	test_makeencoding \
	test_man \
	test_maparg \
	test_mapping \
	test_marks \
	test_match \
	test_matchadd_conceal \
	test_matchadd_conceal_utf8 \
	test_memory_usage \
	test_menu \
	test_messages \
	test_mksession \
	test_mksession_utf8 \
	test_modeline \
	test_move \
	test_nested_function \
	test_netbeans \
	test_normal \
	test_number \
	test_options \
	test_packadd \
	test_partial \
	test_paste \
	test_perl \
	test_plus_arg_edit \
	test_popup \
	test_preview \
	test_profile \
	test_prompt_buffer \
	test_put \
	test_python2 \
	test_python3 \
	test_pyx2 \
	test_pyx3 \
	test_quickfix \
	test_quotestar \
	test_recover \
	test_regex_char_classes \
	test_regexp_latin \
	test_regexp_utf8 \
	test_registers \
	test_reltime \
	test_rename \
	test_restricted \
	test_retab \
	test_ruby \
	test_scriptnames \
	test_scroll_opt \
	test_scrollbind \
	test_search \
	test_searchpos \
	test_set \
	test_sha256 \
	test_shortpathname \
	test_signals \
	test_signs \
	test_smartindent \
	test_sort \
	test_source \
	test_source_utf8 \
	test_spell \
	test_startup \
	test_startup_utf8 \
	test_stat \
	test_statusline \
	test_substitute \
	test_suspend \
	test_swap \
	test_syn_attr \
	test_syntax \
	test_system \
	test_tab \
	test_tabline \
	test_tabpage \
	test_tagcase \
	test_tagjump \
	test_taglist \
	test_tcl \
	test_termencoding \
	test_terminal \
	test_terminal_fail \
	test_textformat \
	test_textobjects \
	test_textprop \
	test_timers \
	test_true_false \
	test_undo \
	test_unlet \
	test_user_func \
	test_usercommands \
	test_utf8 \
	test_utf8_comparisons \
	test_vartabs \
	test_viminfo \
	test_vimscript \
	test_virtualedit \
	test_visual \
	test_winbar \
	test_winbuf_close \
	test_window_cmd \
	test_window_id \
	test_windows_home \
	test_wnext \
	test_wordcount \
	test_writefile \
	test_xxd \
	test_alot_latin \
	test_alot_utf8 \
	test_alot


# Test targets that use runtest.vim.
# Keep test_alot*.res as the last one, sort the others.
# test_largefile.res is omitted, it uses too much resources to run on CI.
NEW_TESTS_RES = \
	test_arabic.res \
	test_arglist.res \
	test_assert.res \
	test_autochdir.res \
	test_autocmd.res \
	test_autoload.res \
	test_backspace_opt.res \
	test_blob.res \
	test_blockedit.res \
	test_breakindent.res \
	test_bufwintabinfo.res \
	test_cdo.res \
	test_changelist.res \
	test_channel.res \
	test_charsearch.res \
	test_cindent.res \
	test_clientserver.res \
	test_close_count.res \
	test_cmdline.res \
	test_command_count.res \
	test_comparators.res \
	test_conceal.res \
	test_crypt.res \
	test_cscope.res \
	test_curswant.res \
	test_diffmode.res \
	test_digraph.res \
	test_display.res \
	test_edit.res \
	test_erasebackword.res \
	test_escaped_glob.res \
	test_eval_stuff.res \
	test_exec_while_if.res \
	test_exists.res \
	test_exists_autocmd.res \
	test_exit.res \
	test_file_size.res \
	test_filechanged.res \
	test_find_complete.res \
	test_fixeol.res \
	test_fnameescape.res \
	test_fold.res \
	test_getcwd.res \
	test_getvar.res \
	test_gf.res \
	test_gn.res \
	test_gui.res \
	test_gui_init.res \
	test_hardcopy.res \
	test_help.res \
	test_hide.res \
	test_highlight.res \
	test_history.res \
	test_hlsearch.res \
	test_iminsert.res \
	test_increment.res \
	test_increment_dbcs.res \
	test_ins_complete.res \
	test_job_fails.res \
	test_json.res \
	test_jumplist.res \
	test_langmap.res \
	test_let.res \
	test_lineending.res \
	test_listchars.res \
	test_listdict.res \
	test_listlbr.res \
	test_lua.res \
	test_makeencoding.res \
	test_man.res \
	test_maparg.res \
	test_marks.res \
	test_matchadd_conceal.res \
	test_memory_usage.res \
	test_mksession.res \
	test_nested_function.res \
	test_netbeans.res \
	test_normal.res \
	test_number.res \
	test_options.res \
	test_packadd.res \
	test_paste.res \
	test_perl.res \
	test_plus_arg_edit.res \
	test_preview.res \
	test_profile.res \
	test_prompt_buffer.res \
	test_python2.res \
	test_python3.res \
	test_pyx2.res \
	test_pyx3.res \
	test_quickfix.res \
	test_quotestar.res \
	test_regex_char_classes.res \
	test_registers.res \
	test_restricted.res \
	test_retab.res \
	test_ruby.res \
	test_scriptnames.res \
	test_scrollbind.res \
	test_search.res \
	test_shortpathname.res \
	test_signals.res \
	test_signs.res \
	test_smartindent.res \
	test_source.res \
	test_spell.res \
	test_startup.res \
	test_stat.res \
	test_substitute.res \
	test_swap.res \
	test_syntax.res \
	test_system.res \
	test_tab.res \
	test_tcl.res \
	test_termencoding.res \
	test_terminal.res \
	test_terminal_fail.res \
	test_textformat.res \
	test_textobjects.res \
	test_textprop.res \
	test_undo.res \
	test_user_func.res \
	test_usercommands.res \
	test_vartabs.res \
	test_viminfo.res \
	test_vimscript.res \
	test_visual.res \
	test_winbar.res \
	test_winbuf_close.res \
	test_window_id.res \
	test_windows_home.res \
	test_wordcount.res \
	test_writefile.res \
	test_xxd.res \
	test_alot_latin.res \
	test_alot_utf8.res \
	test_alot.res
