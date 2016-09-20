This directory contains tests for various Vim features.

If it makes sense, add a new test method to an already existing file.  You may
want to separate it from other tests with comment lines.

The numbered tests are older, we have switched to named tests.  Don't add any
more numbered tests.

And then you can choose between a new style test, which is a Vim script, or an
old style test, which uses Normal mode commands.  Use a new style test if you
can.  Use an old style test when it needs to run without the +eval feature.


TO ADD A NEW STYLE TEST:

1) Create a test_<subject>.vim file.
2) Add test_<subject>.vim to NEW_TESTS in Make_all.mak in alphabetical order.
3) Use make test_<subject>.res to run a single test in src/testdir/.
   Use make test_<subject>  to run a single test in src/.
4) Also add an entry in src/Makefile.

What you can use (see test_assert.vim for an example):
- Call assert_equal(), assert_true(), assert_false(), etc.
- Use try/catch to check for exceptions.
- Use alloc_fail() to have memory allocation fail. This makes it possible
  to check memory allocation failures are handled gracefully.  You need to
  change the source code to add an ID to the allocation.  Update LAST_ID_USED
  above alloc_id() to the highest ID used.
- Use disable_char_avail_for_testing(1) if char_avail() must return FALSE for
  a while.  E.g. to trigger the CursorMovedI autocommand event.
  See test_cursor_func.vim for an example
- If the bug that is being tested isn't fixed yet, you can throw an exception
  so that it's clear this still needs work.  E.g.:
	  throw "Skipped: Bug with <c-e> and popupmenu not fixed yet"
- See the start of runtest.vim for more help.


TO ADD AN OLD STYLE TEST:

1) Create test_<subject>.in and test_<subject>.ok files.
2) Add test_<subject>.out to SCRIPTS_ALL in Make_all.mak in alphabetical order.
3) Use make test_<subject>.out to run a single test in src/testdir/.
   Use make test_<subject>  to run a single test in src/.
4) Also add an entry in src/Makefile.

Keep in mind that the files are used as if everything was typed:
- To add comments use:   :"  (that's an Ex command comment)
- A line break is like pressing Enter.  If that happens on the last line
  you'll hear a beep!
