Tests for syntax highlighting plugins
=====================================

Summary: Files in the "input" directory are edited by Vim with syntax
highlighting enabled.  Screendumps are generated and compared with the
expected screendumps in the "dumps" directory.  This will uncover any
character attributes that differ.

The dumps are normally 20 screen lines tall.  Without any further setup
a screendump is made at the top of the file (using _00.dump) and another
screendump is made if there are more lines (using _01.dump), and so on.

When the screendumps are OK an empty "done/{name}" file is created.  This
avoids running the test again until "make clean" is used.  Thus you can run
"make test", see one test fail, try to fix the problem, then run "make test"
again to only repeat the failing test.

When a screendump differs it is stored in the "failed" directory.  This allows
for comparing it with the expected screendump, using a command like:

	let fname = '{name}_00.dump'
	call term_dumpdiff('failed/' .. fname, 'dumps/' .. fname)


Creating a syntax plugin test
-----------------------------

Create a source file in the language you want to test in the "input"
directory.  Use the filetype name as the base and a file name extension
matching the filetype.  Let's use Java as an example.  The file would then be
"input/java.java".

Make sure to include some interesting constructs with plenty of complicated
highlighting.  Optionally, pre-configure the testing environment by including
setup commands at the top of the input file.  The format for these lines is:

	VIM_TEST_SETUP {command}

where {command} is any valid Ex command, which extends to the end of the line.
The first 20 lines of the input file are ALWAYS scanned for setup commands and
these will be executed before the syntax highlighting is enabled.  Typically,
these lines would be included as comments so as not to introduce any syntax
errors in the input file but this is not required.

Continuing the Java example:

	// VIM_TEST_SETUP let g:java_space_errors = 1
	// VIM_TEST_SETUP let g:java_minlines = 5
	class Test { }

As an alternative, setup commands can be included in an external Vim script
file in the "input/setup" directory.  This script file must have the same base
name as the input file.

So, the equivalent example configuration using this method would be to create
an "input/setup/java.vim" script file with the following lines:

	let g:java_space_errors = 1
	let g:java_minlines = 5

Both inline setup commands and setup scripts may be used at the same time, the
script file will be sourced before any VIM_TEST_SETUP commands are executed.

Every line of a source file must not be longer than 1425 (19 x 75) characters.

If there is no further setup required, you can now run all tests:

	make test

Or you can run the tests for a filetype only by passing its name as another
target, e.g. "java", before "test":

	make java test

Or you can run a test or two by passing their filenames as extra targets, e.g.
"java_string.java" and "java_numbers.java", before "test", after listing all
available syntax tests for Java:

	ls testdir/input/java*
	make java_string.java java_numbers.java test

(Some interactive shells may attempt to perform word completion on arbitrary
command arguments when you press certain keys, e.g. Tab or Ctrl-i.)

As an alternative, you can specify a subset of test filenames for running as
a regular expression and assign it to a VIM_SYNTAX_TEST_FILTER environment
variable; e.g. to run all tests whose base names contain "fold", use any of:

	make test -e 'VIM_SYNTAX_TEST_FILTER = fold.*\..\+'
	make test VIM_SYNTAX_TEST_FILTER='fold.*\..\+'
	VIM_SYNTAX_TEST_FILTER='fold.*\..\+' make test

Consider quoting the variable value to avoid any interpretation by the shell.

Both Make targets and the variable may be used at the same time, the target
names will be tried for matching before the variable value.

The first time testing "input/java.java" will fail with an error for a missing
screendump.  The newly created screendumps will be "failed/java_00.dump",
"failed/java_01.dump", etc.  You can inspect each with:

	call term_dumpload('failed/java_00.dump')
	call term_dumpload('failed/java_01.dump')
	...

If they look OK, move them to the "dumps" directory:

	:!mv failed/java_00.dump dumps
	:!mv failed/java_01.dump dumps
	...

If you now run the test again, it will succeed.


Adjusting a syntax plugin test
------------------------------

If you make changes to the syntax plugin, you should add code to the input
file to see the effect of these changes.  So that the effect of the changes
are covered by the test.  You can follow these steps:

1. Edit the syntax plugin somewhere in your personal setup.  Use a file
   somewhere to try out the changes.
2. Go to the directory where you have the Vim code checked out and replace the
   syntax plugin.  Run the tests: "make test".  Usually the tests will still
   pass, but if you fixed syntax highlighting that was already visible in the
   input file, carefully check that the changes in the screendump are
   intentional:

	let fname = '{name}_00.dump'
	call term_dumpdiff('failed/' .. fname, 'dumps/' .. fname)

   Fix the syntax plugin until the result is good.
2. Edit the input file for your language to add the items you have improved.
   (TODO: how to add another screendump?).
   Run the tests and you should get failures.  Like with the previous step,
   carefully check that the new screendumps in the "failed" directory are
   good.  Update the syntax plugin and the input file until the highlighting
   is good and you can see the effect of the syntax plugin improvements.  Then
   move the screendumps from the "failed" to the "dumps" directory.  Now "make
   test" should succeed.
3. Prepare a pull request with the modified files:
	- syntax plugin:    syntax/{name}.vim
	- Vim setup file:   syntax/testdir/input/setup/{name}.vim (if any)
	- test input file:  syntax/testdir/input/{name}.{ext}
	- test dump files:  syntax/testdir/dumps/{name}_00.dump
			    syntax/testdir/dumps/{name}_01.dump (if any)
			    ...

As an extra check you can temporarily put back the old syntax plugin and
verify that the tests fail.  Then you know your changes are covered by the
test.


Viewing generated screendumps (local)
-------------------------------------

You may also wish to look at the whole batch of failed screendumps after
running "make test".  Source the "viewdumps.vim" script for this task:

	[VIMRUNTIME=../..] \
	../../src/vim --clean -S testdir/viewdumps.vim \
				[testdir/dumps/java_*.dump ...]

By default, all screendumps found in the "failed" directory will be added to
the argument list and then the first one will be loaded.  Loaded screendumps
that bear filenames of screendumps found in the "dumps" directory will be
rendering the contents of any such pair of files and the difference between
them (:help term_dumpdiff()); otherwise, they will be rendering own contents
(:help term_dumpload()).  Remember to execute :edit when occasionally you see
raw file contents instead of rendered.

At any time, you can add, list, and abandon other screendumps:

	:$argedit testdir/dumps/java_*.dump
	:args
	:qall

The listing of argument commands can be found under :help buffer-list.


Viewing generated screendumps (from a CI-uploaded artifact)
-----------------------------------------------------------

After you have downloaded an artifact archive containing failed screendumps
and extracted its files in a temporary directory, you need to set up a "dumps"
directory by creating a symlink:

	cd /path/to/fork
	ln -s $(pwd)/runtime/syntax/testdir/dumps \
				/tmp/runtime/syntax/testdir/dumps

You can now examine the extracted screendumps:

	./src/vim --clean -S runtime/syntax/testdir/viewdumps.vim \
				/tmp/runtime/syntax/testdir/failed/*.dump


Viewing generated screendumps (submitted for a pull request)
------------------------------------------------------------

Note: There is also a "git difftool" extension described in
      src/testdir/commondumps.vim

First, you need to check out the topic branch with the proposed changes and
write down a difference list between the HEAD commit (index) and its parent
commit with respect to the changed "dumps" filenames:

	cd /path/to/fork
	git switch prs/1234
	git diff-index --relative=runtime/syntax/testdir/dumps/ \
				--name-only prs/1234~1 > /tmp/filelist

Then, you need to check out the master branch, change the current working
directory to reconcile relative filepaths written in the filenames list, copy
in the "failed" directory the old "dumps" files, whose names are on the same
list, and follow it by checking out the topic branch:

	git switch master
	cd runtime/syntax/testdir/dumps
	cp -t ../failed $(cat /tmp/filelist)
	git switch prs/1234

Make note of any missing new screendumps.  Please remember about the
introduced INVERTED relation between "dumps" and "failed", i.e. the files to
be committed are in "dumps" already and their old versions are in "failed".
Therefore, you need to copy the missing new screendumps from "dumps" to
"failed":

	cp -t ../failed foo_10.dump foo_11.dump foo_12.dump

After you have changed the current working directory to its parent directory,
you can now examine the screendumps from the "failed" directory (note that new
screendumps will be shown with no difference between their versions):

	cd ..
	../../../src/vim --clean -S viewdumps.vim


TODO: test syncing by jumping around
