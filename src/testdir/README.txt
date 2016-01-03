This directory contains tests for various Vim features.

If it makes sense, try to add a new test to an already existing file.  You may
want to separate it from other tests in that file using :" (that's an Ex
command comment).

The numbered tests are older, we have switched to named tests.
And the newest way of testing is to use assert functions, see test_assert.vim
for an example.

To add a new test:
1) Create test_<subject>.in and test_<subject>.ok files.
2) Add them to all Makefiles (Make*) in alphabetical order (search for an
   existing test_file.out to see where to add the new one).
3) Use make test_<subject>.out to run a single test file in src/testdir/.
4) Also add an entry in src/Makefile.

Keep in mind that the files are used as if everything was typed.
A line break is like pressing Enter.  If that happens on the last line you'll
hear a beep.
