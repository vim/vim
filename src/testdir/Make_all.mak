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
	test4.out \
	test5.out \
	test7.out \
	test8.out \
	test14.out \
	test15.out \
	test19.out \
	test20.out \
	test28.out \
	test29.out \
	test31.out \
	test33.out \
	test34.out \
	test36.out \
	test37.out \
	test38.out \
	test39.out \
	test40.out \
	test41.out \
	test42.out \
	test43.out \
	test44.out \
	test45.out \
	test48.out \
	test53.out \
	test55.out \
	test56.out \
	test57.out \
	test60.out \
	test64.out \
	test66.out \
	test68.out \
	test69.out \
	test70.out \
	test73.out \
	test79.out \
	test80.out \
	test88.out \
	test94.out \
	test95.out \
	test99.out \
	test108.out \
	test_autoformat_join.out \
	test_changelist.out \
	test_close_count.out \
	test_erasebackword.out \
	test_eval.out \
	test_fixeol.out \
	test_insertcount.out \
	test_listchars.out \
	test_search_mbyte.out \
	test_wordcount.out


# Tests that run on most systems, but not on Amiga.
SCRIPTS_MORE1 = \
	test11.out \
	test52.out \
	test85.out \
	test86.out \
	test87.out


# Tests that run on most systems, but not on Amiga and DOS/Windows.
SCRIPTS_MORE2 = \
	test12.out \
	test25.out \
	test49.out


# Tests that run on most systems, but not MingW and Cygwin.
SCRIPTS_MORE3 = \
	test54.out


# Tests that run on most systems, but not on VMS
SCRIPTS_MORE4 = \
	test17.out \
	test30.out \
	test32.out \
	test59.out \
	test72.out \
	test78.out \
	test83.out


# Tests specifically for MS-Windows.
SCRIPTS_WIN32 = test50.out


# Tests for the GUI.
SCRIPTS_GUI =


# Tests using runtest.vim.vim.
# Keep test_alot*.res as the last one, sort the others.
NEW_TESTS = test_arabic.res \
	    test_arglist.res \
	    test_assert.res \
	    test_autochdir.res \
	    test_autocmd.res \
	    test_backspace_opt.res \
	    test_breakindent.res \
	    test_bufwintabinfo.res \
	    test_cdo.res \
	    test_channel.res \
	    test_charsearch.res \
	    test_cindent.res \
	    test_clientserver.res \
	    test_cmdline.res \
	    test_command_count.res \
	    test_crypt.res \
	    test_cscope.res \
	    test_curswant.res \
	    test_diffmode.res \
	    test_digraph.res \
	    test_display.res \
	    test_edit.res \
	    test_farsi.res \
	    test_file_size.res \
	    test_fnameescape.res \
	    test_fold.res \
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
	    test_increment.res \
	    test_increment_dbcs.res \
	    test_job_fails.res \
	    test_json.res \
	    test_langmap.res \
	    test_let.res \
	    test_lineending.res \
	    test_listlbr.res \
	    test_listlbr_utf8.res \
	    test_lua.res \
	    test_makeencoding.res \
	    test_man.res \
	    test_marks.res \
	    test_matchadd_conceal.res \
	    test_mksession.res \
	    test_mksession_utf8.res \
	    test_nested_function.res \
	    test_netbeans.res \
	    test_normal.res \
	    test_number.res \
	    test_options.res \
	    test_packadd.res \
	    test_paste.res \
	    test_perl.res \
	    test_preview.res \
	    test_profile.res \
	    test_python2.res \
	    test_python3.res \
	    test_pyx2.res \
	    test_pyx3.res \
	    test_quickfix.res \
	    test_quotestar.res \
	    test_retab.res \
	    test_registers.res \
	    test_ruby.res \
	    test_scrollbind.res \
	    test_search.res \
	    test_signs.res \
	    test_smartindent.res \
	    test_spell.res \
	    test_startup.res \
	    test_startup_utf8.res \
	    test_stat.res \
	    test_substitute.res \
	    test_syntax.res \
	    test_system.res \
	    test_tcl.res \
	    test_terminal.res \
	    test_textobjects.res \
	    test_undo.res \
	    test_usercommands.res \
	    test_viminfo.res \
	    test_vimscript.res \
	    test_visual.res \
	    test_window_id.res \
	    test_writefile.res \
	    test_alot_latin.res \
	    test_alot_utf8.res \
	    test_alot.res


# Explicit dependencies.
test49.out: test49.vim

test60.out: test60.vim

test_options.res test_alot.res: opt_test.vim
