# AGENTS.md - writing Vim tests

How to write tests in `src/testdir` that match the existing ones.  Read
the top-level `AGENTS.md` first; `README.txt` is the guide for humans.
This file applies to every test.  `AGENTS_AREAS.md` has the non-obvious
facts and pitfalls per area; before writing a test, read the section
for the area you work on there (not the whole file), and the existing
tests next to the one you add.  Its sections:

- Data types and builtins
- Vim9 language
- Legacy script
- Normal, Visual and Ex commands
- Registers, undo, mappings, menus, user commands
- Insert mode, completion, command line
- Search, substitute, tags
- Text properties, folds, signs, matches
- Windows, buffers, tab pages, argument list
- Display, scrolling, screen positions
- Vim9 classes, enums, interfaces
- Variants, restrictions, implementations
- Autocommands, callbacks, fresh Vim, startup
- Indenting and formatting
- Files: writing, swap, recovery, encryption, sessions, viminfo
- Spell checking
- Filetypes, runtime files, packages, plugins
- Options
- Terminal, jobs, timers, channels, client-server
- Terminal codes, keys, mouse, GUI, MS-Windows
- Language interfaces
- Other areas

# Core

## How tests run

- `runtest.vim` runs each `test_<subject>.vim` in a fresh Vim (`-u NONE`)
  and calls every `Test_*` function, in **alphabetical order**, not file
  order.  A test fails when it adds to `v:errors` or throws.  It already
  sources `util/setup.vim`, `util/shared.vim` (incl. `view_util.vim`) and
  `util/check.vim`: `Check*`, `WaitFor()`, `WaitForAssert()`, `RunVim()`,
  `GetVimCommand()`, `Screenline()`, `Standby()`/`Resume()` are available.
- `Test_AA_<what>` / `Test_00_<what>` runs first, `Test_xxx_<what>` last;
  add a comment saying why.  `SetUp()` / `TearDown()` run around every
  test.  A `Test_` function takes no arguments; name helpers otherwise.
- After each test the runner wipes extra buffers/windows and swap files.
  It does **not** restore options, mappings, autocommands, highlight,
  globals, functions, `'runtimepath'` or files: each test cleans up its
  own.  Leftover `X*` files are deleted silently after the last test of
  the file, so a forgotten file never fails but is seen by later tests.
- `throw 'Skipped: <reason>'` reports a skip (the `Check*` commands do
  this).  `Test_nocatch_<what>()` runs without the runner's `try`; it
  skips by setting `g:skipped_reason` and returning (ignored elsewhere).
  Don't use `return` to skip: the test is reported as passed.
- The runner's `SwapExists` autocommand fails the test on an unexpected
  swap file.  A test expecting one does `autocmd! SwapExists` and adds
  its own, or sets `g:ignoreSwapExists` (a String is the answer).
- Quitting Vim in a test is reported as `Test caused Vim to exit`.
- `g:test_is_flaky = 1` retries the test (set by every screendump test);
  never use it to hide a real failure.
- The `TestTimeout()` timer exists, so `timer_info()` starts with one
  entry.

Running (from `src/testdir`; always via `make`, which adds flags without
which a child Vim can hang):

    make test_<subject>
    TEST_FILTER=<pattern> make test_<subject>
    TEST_NO_RETRY=yes make test_<subject>   # no retries while developing

Read the result at the end of `messages` (`Executed ...`, then `N FAILED:`
and the errors).  `make test_<subject>` deletes `test_<subject>.res`,
`messages` and `test.log` first, prints `messages` and fails when a test
failed.  `make test_<subject>.res` does nothing while the `.res` file
(written when all tests pass) is newer than the test file and `../vim`,
so it does not re-run after changing `TEST_FILTER` or a runtime or
`util/` file, and it appends to old `messages` and `test.log`.

- The test Vim is still "starting" (`v:vim_did_enter` 0): `OptionSet`
  doesn't fire and `'autochdir'` does nothing; use
  `test_override('starting', 1)` (reset to 0) and `test_autochdir()`.
  A `Test_` runs at function depth 2 and Ex nesting 5 (matters for
  `'maxfuncdepth'` limits).
- The runner starts Vim with `--noplugin`, sets `$HOME` to
  `testdir/XfakeHOME` (real one in `$ORIGHOME`; `$XDG_CONFIG_HOME` still
  points at the real config: unset or redirect it before writing
  there), `encoding=utf-8` (original in `g:orig_encoding`), `v:testing`
  and `test_override('alloc_lines', 1)` (`'ALL'` doesn't reset it; a
  child Vim doesn't have it).
- `g:testfunc` is the running test (`'Test_x()'`); `SetUp()` may branch
  on it.  `$TEST_SKIP_PAT` skips matching tests (mention it above a
  known-slow test).  A setup test that sorts first (`Test_AAA_setup`)
  must be included in `TEST_FILTER`.
- A script-level `throw 'Skipped: ...'` before the first function skips
  the whole file.  In a file that starts with `vim9script`, tests must be
  `def g:Test_x()` (a plain `def` is script-local and never run).
- `expand('<stack>')` in a test starts with `script .../runtest.vim[N]..
  function RunTheTest[N]..Test_x`; compare only the tail of stacks
  (`actual[-len(expect):]`).

## Where to put a test

- Add to the existing file for the area, next to tests of the same
  feature.  A new `test_<subject>.vim` goes into `Make_all.mak` in both
  `NEW_TESTS` and `NEW_TESTS_RES`, in alphabetical order (or is sourced
  from `test_alot.vim` if quick and free of side effects).
- Starting a file from another one: rewrite the copied comments.

- Files sourced from `test_alot*.vim` are in `NEW_TESTS` only: latin1
  ones in `test_alot_latin.vim` (file starts with `set encoding=latin1`
  and `scriptencoding latin1`), UTF-8 ones in `test_alot_utf8.vim`.  New
  `test_vim9_*.vim` files go in `TEST_VIM9`/`TEST_VIM9_RES`.  A test too
  slow for CI is in `NEW_TESTS` only, with a comment.
- Give a test its own small file (comment why) when it needs another
  `'encoding'` (`set encoding=cp932` + `scriptencoding cp932`), sets
  script-level state that affects other tests, or makes valgrind report
  false leaks (`fork()` failures; Vim9 `job_start()` tests go in
  `test_vim9_fails.vim`).
- Consistency tests read the sources: a new Ex command needs a line in
  `runtime/doc/index.txt` (`test_cmd_lists`), a modifier an entry in
  `cmdmod_info_tab` (`test_cmdmods`), a builtin an entry in
  `global_functions[]` (ASCII order), in `builtin.txt` (USAGE and
  `*name()*`) and `usr_41.txt` `*function-list*`, with argument counts
  and names matching `getinfo('function', name)` (`test_function_lists`),
  a `+feature` a sorted entry in `features[]` (`test_version`).
- `test_codestyle.vim` fails on trailing white space or a space before a
  Tab in testdir, C and help files (`codestyle: ignore` on the line).
- Fuzzer PoCs go in `crash/<name>` with a line in `test_crash.vim`
  (ASan-only ones in `Test_crash1_3()`).  `test_gettext_makefile_in*.vim`
  are input data and `test_bench_regexp.vim` a benchmark, not tests.
- Old-style tests `test10.in` and `test20.in`-`test29.in` (`SCRIPTS_TINY`
  in `Make_all.mak`) work without `+eval`: `dotest.in` runs the Normal
  mode commands between `STARTTEST` and `ENDTEST`, the test writes
  `test.out`, and it must equal `testNN.ok`.  Every `make` runs them;
  `make tiny` runs only them.  Write new tests as `test_*.vim`; add an
  old-style test only for behavior that must be checked in a build
  without `+eval`.  A change that alters their output must update the
  `.ok` file.  Run one with `make testNN.out` (delete `testNN.out` first:
  it is not re-run while it is newer than the `.in` file and `../vim`);
  a failure writes `testNN.failed` and a line in `test.log`.
  `test77a.in` is only used on VMS.

## File layout

```vim
" Tests for <subject>

source util/screendump.vim          " only for screendumps / terminal
import './util/vim9.vim' as v9      " only for v9.Check* helpers

CheckFeature quickfix               " only if the whole file needs it

" One-line description of what is tested
func Test_subject_what()
  ...
endfunc

" vim: shiftwidth=2 sts=2 expandtab
```

- Two-space indent, spaces only; keep the file's modeline (some use
  `{{{1` markers and `fdm=marker`).
- Name `Test_<area>_<what>`, unique in the file.  For a reported issue,
  write `" Test for issue #12345: <summary>` and use the reporter's
  reproducer.  A crash regression says `" The following used to crash
  Vim`.  A test that only finds a memory error under ASan/valgrind says
  so.

## Legacy or Vim9

- Most files use legacy `func`/`endfunc` with `call`/`let`; no `l:`
  prefix.  Use `def`/`enddef` in `test_vim9_*.vim` or where neighbours do.
- **GC tests are legacy functions.**  Never call
  `test_garbagecollect_now()` with a `:def` function on the stack (its
  values are not marked).  Put it at script level inside `lines` run by
  `v9.CheckSourceSuccess(lines)` from a `func` (see
  `Test_enum_garbagecollect()`).
- A `:def` function is compiled when first called, so a `Check*` inside
  it is too late when the body uses a function a build may lack: check
  in a legacy wrapper that calls a `def Run_Test_<what>()`.
- In `def` and `vim9script` code, call global functions with `g:`
  (`g:RunVimInTerminal()`, `g:WaitForAssert()`, `g:Screenline()`, and
  `func`/`def` defined in a legacy test file), else `E117`.  Helpers only
  the file uses: `def s:Name()`.
- In `:def`, `has('lit')` and `exists_compiled()` are folded at compile
  time and a false branch is not compiled, so `if !has('channel') |
  CheckFeature channel | else ... | endif` works inside a `def` too.
- In `lines` without `vim9script`, `def Name()` defines a global
  function: `delfunc g:Name` before the next case (else `E122`).
- A test whose `def` body would not reach its assert (the callee
  aborts) must be a legacy `func`.  A `def` test with deliberately
  unreachable code: `test_override('unreachable', 1)`, `defcompile
  Test_x`, reset, all at script level.
- `feedkeys(keys, 'c')` runs the keys without script context (legacy
  syntax, no `s:`).

## Guards (`util/check.vim`)

At the start of a test, or at the top of the file if all tests need it
(then not again inside tests):

    CheckFeature x  CheckNotFeature x  CheckOption x  CheckFunction x
    CheckCommand x  CheckExecutable x  CheckUnix  CheckLinux  CheckMSWindows
    CheckNotMSWindows  CheckNotMac  CheckScreendump  CheckRunVimInTerminal
    CheckGui  CheckNotGui  CheckCanRunGui  CheckNotRoot  CheckEnglish
    CheckNotAsan  CheckNotValgrind  CheckAnyOf Feature:a Feature:b  CheckAllOf

Platform variants go in separate tests with a guard rather than large
`if has('win32')` blocks.  When a platform gives another valid result,
accept both with a comment rather than skipping.

Also `CheckX11`, `CheckX11BasedGui`, `CheckClipboardInTerminal`,
`CheckTwoClipboards`, `CheckNotBSD`, `CheckNotOpenBSD`, `CheckNotMacM1`,
`CheckPowerShell`, `CheckEnv X`, `CheckAsan`, `CheckGithubActions`,
`CheckImageBackend`, `CheckIPv6`; `CheckPython(name)` is a function;
`util/window_manager.vim` has `CheckXServer`, `CheckWaylandCompositor`.

- The guards are user commands: a test that clears all user commands
  deletes them first and then calls the function form (`call
  CheckRunVimInTerminal()`).
- To test the error a build without the feature gives, check it before
  the file-level guard: `if !has('x') | call assert_fails(..., 'E942:') |
  endif`.  A file-local guard is an `s:` function that throws, wrapped in
  a `command`.  A test that exists only when an option exists:
  `if exists('+opt') | execute join(['func Test_x()', ...], "\n") | endif`.

## Assertions

- `assert_equal(expected, actual)`, expected first.  Also
  `assert_notequal`, `assert_true/false`, `assert_match/notmatch`,
  `assert_inrange`, `assert_report`, `assert_beeps/nobeep` (a command
  string), `assert_equalfile`, `assert_exception` (in `catch`).
- **Assert the full error message**, with the names, types and values Vim
  fills in, not only the number: `assert_fails('call F(1)', 'E1013:
  Argument 1: type mismatch, expected string but got number')`.  An
  `E1013:` alone also passes when another argument or code path fails,
  and the full text shows the reader what is expected.  The same goes
  for the error of the `v9.Check*Failure()` helpers.  Messages are in
  English (`runtest.vim` does `lang mess C`); cut only a part that
  varies (a path, a system error text).
- A list `['E734:', 'E734:']` checks the first and last error; arg 3 is
  a message, arg 4 the line in a sourced script, arg 5 a pattern for the
  script/function name.  Without an error it accepts any error and may
  test nothing.
- Pass a message argument when one assert runs in a loop.
- Assert exact results (`getline(1, '$')`, return values, option values,
  `split(execute('cmd'), "\n")`), not "did not crash".  For a Dict,
  compare the whole Dict when the key set matters.
- Functions that fail through a return value (`-1`): assert it and that
  the state is unchanged.  After any error, check nothing was changed or
  half-assigned.
- **Make each check able to fail.**  Clear the register before a yank
  (`let @" = ''`), use the same variable for a file name that is written
  and checked, use input that the flag under test changes, input in the
  wrong order for a sort.  `assert_equal(x, x)` on the same variable, or
  expected == input, tests nothing.
- Use `assert_fails()` in new tests, not `silent!` + `v:errmsg`.
- A message without an error number: assert only in English
  (`CheckEnglish` or `if v:lang =~ '^\(C\|[Ee]n\)'`).
- Typed keys: `feedkeys("...", 'xt')`; `"\<Esc>"`, `"\<CR>"`.  Command
  line contents: `feedkeys(":cmd\<C-B>\"\<CR>", 'xt')` then
  `assert_equal('"cmd', @:)`.
- Async state: `WaitForAssert({-> assert_equal(x, F())})` or
  `WaitFor({-> cond})` (returns ms waited), never a plain `sleep`.
- Errors inside a heredoc can't go in an `assert_fails()` string; use
  `try` + `assert_report('No exception thrown')` + `catch /E730:/` +
  `catch | call assert_report('Caught: ' .. v:exception)`.
- **Testing a check helper**: an `assert_*()` returns 1 on failure and
  appends to `v:errors`.  Check the return value and `v:errors[0]` right
  after the call, then `remove(v:errors, 0)`.  A comparison gives `0`,
  not `v:false`, so the message says `got 0`.
- `test_override()`, `test_alloc_fail()`, `test_feedinput()`,
  `test_setmouse()`, `test_settime()`, `test_null_*()`,
  `test_refcount()`, `test_garbagecollect_now()` simulate conditions.
  `test_alloc_fail(GetAllocId('name'), 0, 0)` right before the operation,
  then `assert_fails(..., 'E342:')`; new ids go before `aid_last` in
  `src/alloc.h`.  Reset overrides (`test_override('ALL', 0)`).
- `assert_fails('F()', ...)` without `call` always fails with `E492`:
  write `'call F()'`.  A String error is a substring match; List items
  are regexes (`['', 'pat']` checks only the last error, `'E684:\|E1012:'`
  accepts either order; escape `[`, `.`, `*`, `~` and `\` in a full
  message).  An error turned into an exception is
  `Vim(cmd):E121: ...`: catch `/E121:/`, not `/^E121/`.
- An assert can run inside code Vim evaluates later (`set
  tabline=%{assert_equal(...)}` + `redrawstatus!`, `win_execute(id, 'call
  assert_...')`).  Prove such a callback ran with a flag it sets.
- `AssertException(cmds, pat)` (shared.vim) for errors without an E
  number (interpreters); give a pattern that can't match `''`.
- Delays: after an error message the next redraw waits ~1 s (`silent!`
  avoids it); the W10 read-only warning waits 1 s (`test_settime(1)`).
- `test_srand_seed(n)` makes `rand()` repeatable (reset with no arg).
  `test_alloc_fail(id, after, 0)` skips `after` allocations first.
  `test_refcount()`: -1 for Number/String/Float and null values, 0 for an
  unstored container literal, 1 when held in one variable.
- In method form the piped value becomes the first ("expected")
  argument: `F(x)->assert_equal(want)` works, but a failure message
  shows the two values swapped.
- Output: `redir => v` around a `feedkeys()` captures a prompt;
  `execute()` inside `:redir` is not captured, `:redir` inside `execute()`
  is `E930`; an error inside a function prints 3 lines; `:messages`
  starts with `Messages maintainer:` when `$LANG` is set
  (`GetMessages()` drops it).

## Vim9 helpers (`util/vim9.vim`)

`import './util/vim9.vim' as v9`.  Legacy, `:def` and Vim9 script code
take different C paths, so test a behavior in each context it applies
to with one helper.  Use `CheckSource*` in new tests (they source a
scratch buffer); the older `CheckScript*`/`CheckDef*`/`CheckLegacyAndVim9*`
write a temp file; keep them only where neighbours do.

| Helper | Runs `lines` as |
|--------|-----------------|
| `CheckSourceLegacyAndVim9Success(lines)` / `...Failure(lines, err)` | legacy func, `:def`, Vim9 script; `err` string or `[legacy, def, script]` |
| `CheckSourceDefAndScriptSuccess(lines)` / `...Failure(lines, err, lnum)` | `:def` and Vim9 script (`vim9script` prepended); compile error; `err` or `[def, script]` |
| `CheckSourceDefExecAndScriptFailure(lines, err, lnum)` | same, runtime error in `:def` |
| `CheckSourceDefSuccess` / `CheckSourceDefFailure` / `CheckSourceDefExecFailure` | `:def` only |
| `CheckSourceSuccess(lines)` / `CheckSourceFailure(lines, err, lnum)` / `CheckSourceFailureList` | sourced as is (start with `vim9script` for Vim9) |

- Use the widest helper that fits.  `CheckSourceSuccess` + `vim9script`
  for script-level `def`, `class`, `enum`, `import`.  When contexts give
  different errors, pass the list, one message per line.
- `lnum` is the line in `lines`; for an error inside a function it is
  counted from the function's first body line.
- In `CheckLegacyAndVim9*` lines: `VAR` declares, `LET` assigns,
  `TRUE`/`FALSE`, `LSTART a LMIDDLE expr LEND` is a lambda, `#"` a
  comment, use `call`.  The words are replaced everywhere on the line,
  also inside strings.
- `assert_fails('...')` inside `lines` runs outside the `:def` function,
  so `:def` locals are unknown there (`E121`); use a global or a
  `...Failure` helper.
- The helpers run in a scratch window that is wiped: check buffer- and
  window-local state inside `lines`.
- Compile errors must actually be compiled: end `lines` with
  `defcompile` or a call.  `test_override('defcompile', 1)` compiles at
  definition.
- A wrong type usually gives `E1013: Argument N: type mismatch` when
  compiled and `E1174`/`E1210`/`E1206`/... at runtime in Vim9 script;
  test typed values and values through `any`.

Heredocs: `let lines =<< trim END` (reassign with another `let`; in a
`def`, `var lines =<< trim END` once, then `lines =<< trim END`).
`=<< trim eval END` expands `{expr}`.  Use another marker when the text
contains `END`.  Heredoc text confuses syntax highlighting of the code
after it; some files keep such tests at the end.

- Also: `CheckSourceLegacySuccess/Failure` (legacy function only),
  `CheckSourceDefCompileSuccess` (compile, don't run; e.g. a return type
  only checked at runtime), `CheckSourceScriptSuccess` (no `vim9script`),
  `CheckSourceTrans{Legacy,Def,Vim9}Success` (one translated context).
- An incomplete expression at the end of a `def` body continues past
  `lines` (other error number, lnum past the end): use separate def and
  script checks.  An error after a `silent!` statement is not seen by
  the helpers: write a file, `source` it in `try`, match `v:throwpoint`.
- A relative `import './X.vim'` in `lines` resolves against the cwd.
  In `=<< trim eval`, `{{`/`}}` are literal braces.
- Many expressions with shared setup: a `header` list and a loop over
  `[expr, err]` rows, checked as `var x = {expr}` and inside a `def`.

## Files, buffers, windows and state

- Temp files start with `X`: `writefile(lines, 'Xname', 'D')`,
  `mkdir('Xdir', 'R')` (`'pR'` for parents) delete them when the function
  ends.  Never write outside `src/testdir`.
- `new` / `bwipe!` a buffer; `%bw!` or `only` when a test opens many.
- Save and restore every option, `'runtimepath'`/`'packpath'`,
  mapping, autocommand group (use a unique `augroup` name, then
  `au! Name` / `augroup! Name`), highlight group, sign, match, global
  variable and function the test creates.  `set opt&` restores the
  default.
- Window size matters for display, scrolling and cursor results: create
  a fixed-size window (`10new`, `vert 80new`) or compute the expected
  value from `winwidth(0)`.
- Global-local options: test `:set`, `:setlocal`, `:setglobal`, empty
  local value, and that `:split` copies it.
- `'D'`/`'R'` remember the full path, so cleanup works after `:cd`.
  `mkdir(d, 'D')` deletes only an empty directory: use `'R'`.  On
  MS-Windows a directory can't be deleted while it is a cwd or holds a
  swap file.
- `$HOME` is cached: after `unlet $HOME`, `expand('~')` is unchanged; set
  a value and restore it.  `set compatible` resets many options: undo
  with `set nocompatible viminfo+=nviminfo`.
- `writefile(l, f, 'b')` writes no final newline; `"\n"` in an item is
  written as NUL; append a blob (`writefile(0z00, f, 'a')`) for exact
  bytes.  `readfile(f, 'b')` keeps the `\r` of CRLF.  In buffer text a
  NUL is stored as NL (`setline(1, "a\nb")`).
- State that outlives a test and isn't reset by the runner: registers
  and `@.`, marks `A-Z0-9` and the jumplist (`delmarks A-Z0-9`,
  `clearjumps`), tab pages (`tabonly!`), user digraphs, `setcellwidths()`,
  `v:colornames`, loaded spell files, global prop types, interpreter
  globals, the last search/substitute pattern (can't be cleared: use a
  fresh Vim or a test that sorts first).  `set undofile` writes
  `.<name>.un~`, which is not an `X` file: delete it.

## Data tables

When many values get the same checks, loop over a table and pass the
row name as the assert message (a `def` test):

```vim
  for [Val, emptyExp, typenameExp] in [[null_blob, 1, 'blob'], [null_list, 1, 'list<any>']]
    assert_equal(emptyExp, empty(Val))
    assert_equal(typenameExp, typename(Val))
  endfor
```

Add rows for optional features with `if has(...)`.  For combinations of
settings, a local helper with boolean parameters called once per
combination.

## Screendump and terminal tests

```vim
func Test_feature_display()
  CheckScreendump
  let lines =<< trim END
    call setline(1, ['one', 'two'])
    set number
  END
  call writefile(lines, 'XTest_feature', 'D')
  let buf = RunVimInTerminal('-S XTest_feature', {'rows': 10})
  call VerifyScreenDump(buf, 'Test_feature_1', {})
  call term_sendkeys(buf, ":set nonumber\<CR>")
  call VerifyScreenDump(buf, 'Test_feature_2', {})
  call StopVimInTerminal(buf)
endfunc
```

- Needs `source util/screendump.vim`.  `RunVimInTerminal` uses
  `--clean` (loads `defaults.vim`).  Use `CheckRunVimInTerminal` when no
  dump is taken.
- A missing reference writes `failed/<name>.dump` and fails; review it
  (`term_dumpload()`), move it into `dumps/`, and check it in.  A
  `dumps/<name>.vim` filter is applied to both dumps to mask varying
  parts.  The `wait` option raises the 1000 ms limit.
- Wait for the screen after each key (`WaitForAssert` on
  `term_getline(buf, N)`) when a later step depends on it.  Prefer
  asserting text with `term_getline()` when attributes don't matter.
- Text with a path wraps differently with a long cwd; avoid paths or
  skip when `getcwd()` is too long.
- For a child Vim without a terminal: `RunVim([], cmds, '--clean')` or
  `system(GetVimCommand() .. ' -es -X -S Xscript')`, check
  `v:shell_error` and files the child wrote.
- `RunVimInTerminal()` options: `rows` (≤ 20), `cols` (≤ 78; default
  20x75), `wait_for_ruler: 0` (no ruler, `-e -s`, or a child that exits
  by itself: then wait for `term_getstatus(buf) == 'finished'`),
  `statusoff: N` (`'cmdheight'` > 1), `cmd: 'sh'`, `no_clean`, `env`,
  `keep_t_u7`.  Call it from a full-width window.  The child loads no
  plugins (`packadd`/`runtime` in its script); `set clipboard=` for
  Visual-highlight dumps.  A child stuck at `-- More --`:
  `job_stop(term_getjob(buf))`.
- `GetVimCommandClean()` uses `--clean`, `GetVimCommandCleanTerm()` adds
  `-v`, `GetVimProg()` is the bare program (for `-u`, `$VIMINIT` tests).
  A child reports through a file (`writefile(v:errors, 'Xresult')`,
  `json_encode(v:event)`) or its exit code (`cq! len(v:errors)`); one
  that calls `test_garbagecollect_now()` sets `v:testing = 1`.
  `RunVim([], [], '-s Xkeys')` runs typed commands outside any script.
- In a child: poll state without leaving a mode with a `<Cmd>` mapping
  that writes a file; send raw bytes with
  `ch_sendraw(term_getjob(buf)->job_getchannel(), seq)`; after `:qall!`
  check `job_info(job).termsig` is empty (no crash).

## Memory errors and leaks

Error paths leak easily, and such tests pass in a normal build.  Run
them under valgrind or ASan (see the top-level `AGENTS.md`).  A file
that tests a type holding references usually has a `TearDown()` that
calls `test_garbagecollect_now()`.  `test_refcount()` before and after
an operation checks it does not leak or drop a reference.

`RunningWithValgrind()`, `RunningAsan()`, `ValgrindOrAsan()` skip part of
a test; huge loops (65535 iterations) use `CheckNotAsan` and
`CheckNotValgrind`.  A leak that only shows at exit goes in a `Test_z_`
test.  Python reports from pymalloc are not Vim leaks.

# Checklist

1. Right file, next to related tests, one-line comment; a new file is in
   `Make_all.mak`.
2. Guards present, not duplicated; skips throw `Skipped:`.
3. Exact results and full error messages; each check can fail.
4. Every applicable context (legacy, `:def`, Vim9 script; script level,
   methods, lambdas; typed and `any`), with empty, null and error
   cases; compile errors forced with `defcompile`.
5. A new or changed builtin has a wrong-type case per argument in
   `test_vim9_builtin.vim`.
6. Variants through one helper; function options set every way;
   interactive state checked mid-sequence; completion via `@:`.
7. No dependence on test order; everything changed is restored (use
   `:defer` or `try`/`finally`); `X` files with `'D'`/`'R'`.
8. `make test_<subject>` passes, the test fails without the fix, and
   memory-related tests were run under valgrind or ASan.
