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
	test85.out \
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


# Tests using runtest.vim
# Keep test_alot*.res as the last one, sort the others.
# test_largefile.res is omitted, it uses too much resources to run on CI.
NEW_TESTS = test_arabic.res \
	    test_arglist.res \
	    test_assert.res \
	    test_autochdir.res \
	    test_autocmd.res \
	    test_autoload.res \
	    test_backspace_opt.res \
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
	    test_farsi.res \
	    test_file_size.res \
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
	    test_retab.res \
	    test_ruby.res \
	    test_scrollbind.res \
	    test_search.res \
	    test_shortpathname.res \
	    test_signs.res \
	    test_smartindent.res \
	    test_spell.res \
	    test_startup.res \
	    test_stat.res \
	    test_substitute.res \
	    test_swap.res \
	    test_syntax.res \
	    test_system.res \
	    test_tab.res \
	    test_tcl.res \
	    test_terminal.res \
	    test_terminal_fail.res \
	    test_textformat.res \
	    test_textobjects.res \
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


# Explicit dependencies.
test49.out: test49.vim

test_options.res test_alot.res: opt_test.vim
