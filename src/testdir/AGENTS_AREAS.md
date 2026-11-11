# AGENTS_AREAS.md - Vim test pitfalls by area

The non-obvious facts and pitfalls for each area of `src/testdir`.
Read `AGENTS.md` in this directory first; the general rules there (test
every context, every edge, restore state, check errors with full
messages) are not repeated.  Read only the section for the area you
work on, e.g. `grep -n '^## ' AGENTS_AREAS.md` and then that range.

## Data types and builtins

- **One test per operation**, `Test_<type>_<op>`, with `" Test for
  <func>()`; separate success and error tests when errors are many.
  Cover: empty, one item, negative and out-of-range index, nested,
  null (`test_null_list()` / `null_list`), locked, items of each type,
  `v:numbermax`/`v:numbermin`, multibyte and composing characters,
  the method form (`x->func()`), and wrong argument types (legacy:
  `E729`/`E730`/`E731`/`E745`/`E728`).
- `test_vim9_builtin.vim` has one `def Test_<func>()` per builtin, in
  alphabetical order: one wrong-type line per argument with
  `v9.CheckSourceDefAndScriptFailure(['f(...)'], ['E1013: ...', 'E1210: ...'])`,
  the return type (`var f: float = abs(-2)` → `E1012`), and
  runtime-only errors with `CheckSourceDefExecAndScriptFailure`.  It has
  no file-level guard.  Behaviour tests go in the area file
  (`test_functions.vim` when none exists).  Update both.
- **Lists/Dicts**: `:lockvar {depth}` for depths 0..4 (`:def` locals
  can't be locked: drop that context with a comment); scope dicts
  (`a:`: `E742`); changing a list while `for`/`map()`/`filter()`
  iterate it; self-reference with `string()`, `==`, `deepcopy()`,
  `json_encode()` and GC; `copy()` vs `deepcopy()` checked with `is`;
  comparison stops at 1000 levels.
- **`filter()`/`map()`/`mapnew()`/`foreach()`** share argument handling:
  test a change on all four × List/Dict/Blob/String × string expr,
  funcref, lambda, `def`.  `filter()`/`map()` change containers in place
  (use `copy()`), not Strings; `mapnew()` works on a locked value;
  `foreach()` ignores the callback result.  `lockvar 1`: `map()` works,
  `filter()` gives `E741`.  Strings split into characters with their
  composing marks.
- **Blobs**: `0zDEADBEEF`, dotted form, odd digit count (`E973`), byte
  range (`E1239`), `blob2list`/`list2blob`, `blob2str`/`str2blob` with
  each encoding spelling, `readblob()` offset/size.
- **Text formats** (`test_json.vim`): keep text and value as pairs
  (`s:jsonN`/`s:varN`, plus an `x` variant when decoding differs) so the
  encode and decode tests can't drift; the same cases for `js_*`.  Cut a
  valid text after every token (`E491` for each prefix), junk after a
  value (`E488`), depth > `'maxfuncdepth'` (`E132`), lone surrogates,
  `set encoding=latin1`.  NaN never equals itself: use `isnan()`.
- **Floats**: compare `string(result)` (6 digits; shows `inf`, `nan`,
  `-0.0`; `assert_equal` can't tell `0.0` from `-0.0`).  Inputs: `±0.0`,
  `±1.0/0.0`, `0.0/0.0`, out-of-domain (`sqrt(-1.0)`), String (`E808`).
  `float2nr()` clamps around 2^63.
- **Ranking functions** (`matchfuzzy()`): one case per scoring rule
  with input in the opposite order; equal scores keep input order;
  `matchfuzzypos()` checks exact scores (a scoring change must update
  them).  Look up size limits in the source (`FUZZY_MATCH_MAX_LEN` 1024)
  and test below, at and above, also multibyte.
- **`printf()`**: each conversion with each type, `*` width, positional
  args (`E1500`–`E1503`), too few/many args (`E766`/`E767`).
- **Functions that read input** (`input()`, `getchar()`, `confirm()`):
  `feedkeys()` the answer first (`getchar()` with mode `''`), or run the
  call from the command line in the same `feedkeys()`.  `confirm()`
  needs `feedkeys(..., 'L')`, `CheckUnix`, `CheckNotGui`.
- **A builtin calling user code** while working on a list: changing the
  input from there must give `E741`, not corrupt it.
- **Sandbox**: a function or command that changes state outside the
  buffer must give `E48`; add it to `Sandbox_tests()` in
  `test_excmd.vim`.
- **A fake provider** for an external service: callbacks that record
  their args in globals (`v:clipproviders["test"]`, `set clipmethod=test`,
  `CheckFeature clipboard_provider`).
- A new builtin: compare the whole `getinfo('function', name)` dict.
  `add()`, `has()`, `len()`, `exists_compiled()` are compiled specially:
  test at script level, in `:def` and in method form.  Some builtins
  return `-1` in legacy but give `E966` in Vim9.
- Run user code in the middle of a builtin with a list of objects whose
  class has `def string(): string`.  `sandbox function F()` / `sandbox
  let F = {-> ...}` always runs sandboxed (test `E48`).
- Legacy facts: `1/0` is `v:numbermax`, `0/0` `v:numbermin`, `x % 0` 0
  (Vim9 `E1154`); `sort(l, 'n')` sorts Strings as 0 (`'N'` for numeric
  strings); `:const` skips items with refcount > 1; a dict function copied
  to another dict rebinds `self`; in `:for`, removing the next item skips
  it; `{TRUE}` arguments treat `"foo"` as false, `{non-zero-arg}` any
  non-empty String as true.
- Blobs: slice assignment keeps the length (`E972`), `b[len(b)] = n`
  appends, comparing with a non-Blob `E977`.  `b:changedtick` starts at
  2, `undo` adds 2.  Dicts grow differently above 1000 entries (use
  ~1500 keys).  `copy()` drops only the outer declared type.
- Tuples (also legacy: `(1,)`, `()`): immutable (`E1532`, slice
  `E1533`) but inner containers are not; per-context errors (legacy
  `E1524`, def `E1013`, script `E1211`).
- Case-insensitive comparisons: `ſ` (U+017F) and `K` (U+212A) fold to
  ASCII of a different byte length; write strings with `\xNN` in single
  quotes and build the expression with `eval(printf(...))`.  Create
  strings for a `set encoding=latin1` test while still in utf-8.

## Vim9 language

- Statements run through `ex_eval.c`/`ex_docmd.c` at script level and
  `vim9compile.c`/`vim9cmds.c` in `:def`: test both, plus a `def` in a
  script.
- **Skipped code**: put the construct in `if false`, an `elseif` after
  a taken branch, `for x in []`, and a `def` only `defcompile`d; it must
  have no effect and no error.  Code after `return`/`throw`/`break`/
  `continue` gives `E1095`.
- **Comments/white space**: `# x` after a space works; `cmd# x`
  (`E1144`), `"yes"# x`, `#{` (`E1170`), missing space around operators
  (`E1004: White space required before and after '*' at "*2"`; loop over
  operators); the same line in legacy where `#` is not a comment.
- **Line continuation**: operator at line end and next line start,
  comments between lines.
- **Known and `any` types** take different compiler paths: literals,
  typed variables, and globals (`g:anint`); type errors are compile
  errors for typed operands and runtime errors for `any` (`['E1036:',
  'E1030:']`).  Short-circuit: `false && Fails()` must not run.
- **Script reloading**: `writefile()`, `source`, change, `source` again;
  removed functions are gone (`E117`), and after `finish` old functions
  using script variables give `E1149`.  `vim9script noclear`.
- **Assignments**: every target (local, `s:`, `g:`, `b:`, `w:`, `t:`,
  `$VAR`, `&opt`/`&l:`/`&g:`, `@a`, `v:` (read-only: `E46`), list/dict
  item, slice, member); every declaration form (typed, inferred,
  default value of each type, `final` (contents may change), `const`
  (`E741`)); a null value keeps the declared type; compound operators
  on each target; unpacking with `_`, rest, too few/many.
- **Types and aliases**: every place a type appears (args, return,
  `func()`, `list<>`/`dict<>`/`tuple<>`, nested, members, interfaces,
  generics, casts, `for`, across scripts); check with `typename()`.
  A type is not a value: script level `E1403`, `:def` `E1407`.
- **Functions**: arguments (typed, default, `...rest`, `_`, `v:none` for
  a default) with `E118`/`E119`/`E1013` compiled and through `any`;
  missing `return` (`E1027`); closures (after return, shared, in a loop:
  loop variable vs a `var` in the body), legacy interop (`E1189`,
  `E1248`).  An error in a `def` aborts it even with `silent!`; check a
  following assignment didn't run.  Line numbers of errors: source a file
  in `try` and match `v:throwpoint` (`', line 3$'`).
- **Generic functions**: every kind of function (global, script, nested,
  lambda, method, abstract, interface, enum, imported) × every way to
  call (direct, recursive, `function('Fn<number>')`, string name,
  `call()`, callback); wrong type-arg count (`E1556`/`E1557`/`E1559`).
- **Import/export**: every import form (relative, absolute, `import/` in
  `'rtp'`, `autoload`, symlink); every place an imported name is
  evaluated later (mappings `<SID>`/`<ScriptCmd>`, completion functions,
  `'operatorfunc'`, each `*expr` option; set `debug=throw` so errors are
  not silently ignored); `import autoload` must not load the script
  until used (check a global it sets); `E1048`/`E1049`.
- **Classes, enums, interfaces**: see "Vim9 classes, enums, interfaces".
- **`defer`**: record call order in a list; deferred functions run in
  reverse, also when the function throws; `E118`/`E119` when `:defer`
  runs.  They also run on `:qa!`: test in a child Vim with files
  written with `'D'` (gone afterwards) and one appended to.
- **Ex commands in Vim9**: options Vim9 ignores (`'magic'`, `'gdefault'`,
  `'cpo'`, `'ignorecase'` for `==`): loop over values, same result.
  `` `=expr` `` and `\=` in a `:def` find locals; unknown name is
  `E1001`.  Every command modifier, in `:def` and at script level.
- **`:scriptversion`** applies to lines after it; restore it for the
  tests that follow.  Recursion limits: `repeat('(', 1002)` → `E1169`.
- **Where something was set**: `execute('verbose nmap <F3>')` and match
  only `line N` (the file name differs between helpers).
- `vim9script` sets `'cpo'` to the Vim default while sourcing.
- `legacy`/`vim9cmd` test one statement in the other syntax; the
  innermost wins.  `legacy set indentexpr=...` records the syntax used
  later.  `defcompile Test_x` at file level compiles at load.
- Per-context error pairs: unknown command `['E476:', 'E492:']`; locked
  list item `E1119`/`E741`, dict item `E1121`/`E741`; `if [1]`
  `E1012`/`E745`.  Value-only line `E1207`; a range needs a colon
  (`E1050`); commands can't be shortened (`E1065`); one-letter commands
  `a c i k o t x` capture `x = 1`; the alternate file is `%%`.
- Null vs empty: only `null_*` values `== null`; an uninitialized `var
  l: list<T>` is empty (`add()` works), one set to `null_list` stays null
  (`E1130`/`E1134`).  Vim9 string index counts characters (legacy:
  bytes); out of range gives `''`.
- A script-level `var` in an `if`/`for`/`try`/`{}` block is
  block-local.  A misparsed `{` block swallows the rest of the test
  file: test `{` blocks inside `lines`.
- `silent! Func()` hides the error but `Func` still aborts; `silent!`
  inside the `def` continues.  A failed compile gives `E1091` on the
  next call and `E1191` for callers.  A lambda must accept every
  argument a builtin passes (`E1106`); `_` can't be read (`E1181`).
- Generics: type arguments are never inferred (`E1559`).
  `test_override('autoload', 1)` loads `import autoload` scripts at once.
  An import name is not a value (`E1094`/`E1236`).  Reloading a script
  that exports a class gives `E1041`.
- Type aliases: Vim9 script level only (`E1393` legacy, `E1399` in a
  def), defined before use (`E1010`); only `type()`, `typename()`,
  `string()`, `instanceof()` accept one (`E1403`/`E1407`); errors show
  the resolved type.
- Non-aborting compile errors: `v:errmsg = ''`, `defcompile`,
  `assert_equal('', v:errmsg)`.

## Legacy script

- `:let`: curly-brace names, registers, `&t_xx` (`E113` until set),
  each option type with each wrong operator (`.=` on a Number: `E734`;
  value kept), `a:`/`a:000` changed through each path (`E46`, `E461`,
  `E742`), `execute('let a b')` listing compared exactly.
- Heredoc edge cases: empty, marker with extra indent or trailing space,
  text like `endfunc`, `trim` with an unindented marker, inside `if 0`;
  `eval` with an invalid `{expr}`.  Errors that need a whole file
  (`E990`, `E221`, `E1145`): `writefile()` + `assert_fails('source X')`.
- Control flow (`test_vimscript.vim`, `test_trycatch.vim`):
  `source util/script_util.vim` for `XpathINIT`/`Xpath 'a'` (compare
  `g:Xpath`), `XloopINIT`/`Xloop`/`XloopNEXT`, `ExecAsScript Func`
  (`:return` → `:finish`), `DeleteTheScript()`.  `RunInNewVim(test,
  verify)` for errors/interrupts that end script processing;
  `interrupt()` throws `Vim:Interrupt`.  Put `call assert_report('should
  not get here')` on lines that must not run.  `verbose=14` exception
  messages need `CheckEnglish`.
- Legacy functions: `range`, `dict`/`self`, `a:firstline`, `:delfunction`
  of a running function, `execute('function Name')` listing exact.
- Sourcing (`test_source.vim`): put lines in a buffer and `:source`
  (also a range; variables must be set on lines inside it); the script
  ID stays the same when re-sourcing a buffer; `vim9cmd source ++clear`;
  `<slnum>`/`<sflnum>`; `SourceCmd` replaces sourcing (no `SourcePre`);
  `E22` nesting in a terminal Vim; `E167`/`E168`/`E984` outside a script;
  line lengths around the 250-byte read buffer, looped.
- Reports (`:profile`, `--startuptime`): write the code once with
  placeholders for `func`/`def`, substitute, run in a child Vim (the
  report is written on exit), check the line count, then each line;
  times and counts with patterns.  Exclude paused/unmatched functions.
- Debugger: in a terminal, send one command at a time and compare the
  bottom lines (`s:RunDbgCmd()` in `test_debugger.vim`); or with
  `debuggreedy` + `feedkeys(":call F()\<CR>quit\<CR>", 'xt')` (quit
  throws `Vim:Interrupt`).  Always `breakdel *` and `0debuggreedy`.
  Long cwd breaks expected lines (`CheckCWD`).
- `test_trycatch.vim` defines its own `Xpath` (takes an expression).
  Run a control-flow construct three ways: `execute(lines)`,
  `execute(join(lines, '|'))`, `execute(join(lines, "\n"))`.
- Nesting limit 50: `E579` (if), `E585` (loops), `E601` (try), `E1058`
  (functions).  Unfinished blocks need a sourced file: `E171`, `E170`,
  `E600`.  `silent! call F()` also swallows an exception `F` throws.  An
  uncaught throw in a called function gives `['E605:', 'E605:']`; one in
  a deferred function is not propagated (`E605` in `v:errmsg`).  A
  failing default argument without `abort` still runs the body.
- Legacy closures: `test_garbagecollect_now()` between creating and
  calling.  `:profile dump` writes in the test Vim itself (`E750`
  without `start`).  A backward range gives `E493` in a script (`:silent`
  swaps it).  `:source! file` runs Normal-mode commands.
- Ex mode: `"Qlet s:x =<< END\<CR>" .. keys .. "\<CR>END\<CR>visual\<CR>"`
  reads the typed line; with `gQ` use `"\<C-B>\"\<CR>"` and `@:[1:]`;
  every `feedkeys()` must leave Ex mode.  An uncaught error in Ex mode
  adds 1 to the exit code.

## Normal, Visual and Ex commands

- `normal!` (no mappings) by default; `exe "normal! z\<CR>"` for keys.
- Check `getcurpos()` (`[0, lnum, col, off, curswant]`),
  `winsaveview()`, `virtcol('.')`, `mode()`, not just the line.
- Failing commands: `assert_beeps('normal! zq')`, and the buffer is
  unchanged.  After a failure in Visual mode or with a pending count,
  `exe "normal \<C-C>"`.
- Counts before, after and on both (`"ay4y`, `>2>`); very large counts
  must not overflow.
- Each motion alone and with `d`/`c`/`y`, Visual mode, count, at buffer
  start/end, and with `'wrap'`/`'virtualedit'`/`'rightleft'`/folds.
- Visual: loop `for mode in ['v', 'V', "\<C-V>"]`; check
  `getregion()`/`getregionpos()` while active and via `'<`/`'>`.
- Increment: each `'nrformats'` value, leading zeros, `-0`, 64-bit wrap.
- Text objects: `ib` is `i(`, `iB` is `i{`; when copying a test for
  another delimiter change every key.  Cursor before, on each delimiter,
  in an empty object; `selection=exclusive`; `V` becomes `v`.
- `'virtualedit'`: compare all of `getcurpos()` (`off` is cells into a
  Tab or past the end); edits inside a Tab split it into spaces (count
  from `'ts'`); each option value; `getpos("'>")` after `V` is
  `v:maxcol`.  Test a global-local option through a probe whose result
  depends on the effective value.
- Ex ranges: past the end (`E16`), too large (`E1247`, guard with
  `v:sizeoflong`), line 0, counts beyond the buffer.  A command in
  `if 0` must have no side effect.  Text lock (`CTRL-R =`, `CTRL-\ e`):
  `E565`.  A command compiled out gives `E319` (test only when missing).
- `:sort`/`:uniq`: rows `{'name', 'cmd', 'input', 'expected'}` in a
  loop, also checking that already sorted/unique text leaves
  `'modified'` unset.  Each flag and combination with input it changes.
  Marks on removed lines are deleted; one `undo` restores.  `l` uses
  `strcoll()` and then ignores `i` (undocumented).  Locale-dependent
  results: branch on `v:collate`.  `sort()` with each `how`, stable
  order, and a compare function returning a value outside int range.
- Visual blockwise: short lines, `$`, Tabs, double-width chars cut by
  the block edge, empty lines; `I`/`A`/`c`/`r`/`>`.  `p`/`P` over a
  selection with each register type.  Huge counts must not overflow.
- Counts are capped at 999999999.  `K` without a program:
  `com! -nargs=* Kprog let g:args = <q-args>` and `set keywordprg=:Kprog`.
  `feedkeys("\<CursorHold>", 'xt')` fires `CursorHold` directly.
  Plugin mappings need `normal`, not `normal!`.
- `v_p` puts the replaced text in `@"` and `@-`, `v_P` doesn't.  Typing
  `:` ends Visual mode (use `:<C-U>` and `gv`).  A failed text object in
  Visual mode keeps Visual mode active.
- `:uniq` deletes marks on the last N lines of the range and leaves the
  others at their line numbers.  `:append` in `if 0` still reads lines up
  to `.`.  `exists(':cmd')`: 2 full, 1 abbreviation, 3 ambiguous.  Check
  `gf` targets with `expand('<cfile>')` and an explicit `'isfname'`.
- Interrupt a long command in a child: `cnoremap ; <Cmd>sleep 10<CR>`,
  send it, `TermWait()`, `"\<C-C>"`, wait for `Interrupted`.

## Registers, undo, mappings, menus, user commands

- Registers are global and filled by other tests: errors for an empty
  register are tested first (`Test_aaa_empty_reg_test`).  Cover named,
  append (`"A`), unnamed, `"0`, `"1`..`"9` (shift), `"-`, `"_`, `"=`,
  `"/`, read-only (`E354`), clipboard.  Check `getreg(r, 1, 1)`,
  `getregtype()`, `getreginfo()` (which `setreg()` must restore exactly).
  With `set report=0`, yank messages land in `v:statusmsg`.
- **Undo**: a test function is one undo block.  Close it after setup and
  after each change with `set ul&` (or `let &g:undolevels =
  &g:undolevels`); `CTRL-G u` in Insert mode.  Compare the whole
  `undotree()`.  Time-based steps: `test_settime()`, reset with
  `test_settime(0)`.  Undo files: round trip, `E822`, `E823`, and a
  real file truncated at many sizes (`E825`).
- **Mappings**: every mode; listing via
  `execute('nmap ,f')->trim()->split("\n")` with the flags (`*`, `&`,
  `@`); clear default mappings first (`imapclear`).  Save/restore with
  `maparg(lhs, mode, 0, 1)` + `mapset()`.  Compare `maparg()` with a full
  dict (sid from `expand('<sfile>')` `<SNR>`, `lnum` from `<sflnum>`).
  After `mapset()`, also type the keys.  `maplist()` count must agree
  with the `:map` listings after every `mapclear` variant.  Ambiguous
  prefixes: short `timeoutlen`, `feedkeys(keys, 'xt!')`, a timer to end
  the wait (`CheckFeature timers`, flaky).  `mapset()` mode strings that
  no `:map` can make: `E1276`.
- **Menus** work without the GUI (`CheckFeature menu`; `CheckNotGui`
  when the listing or mouse matter).  Same item in every mode, run with
  `:emenu {mode}`; compare `menu_info()` with a full dict (`:amenu` stores
  a different rhs per mode).  `aunmenu *` / `tunmenu *` before listing.
  Runtime menus: source `menu.vim`, end with `delmenu.vim`.  PopUp with
  the mouse: `mouse=a mousemodel=popup`, `test_setmouse()`, and a second
  click from an `<expr>` mapping.
- **User commands**: a helper that stores `a:000`, one escaping case per
  line for `<args>`/`<q-args>`/`<f-args>`/`<line1>`/`<count>`/`<bang>`/
  `<reg>`/`<mods>`; each `-addr` with `%`; warnings via `v:warningmsg`
  (reset it); listing compared exactly.
- Each `feedkeys(..., 'xt')` is its own undo step (typed keys sync
  undo); feed without `t` to test `:undojoin`.  Fill a buffer without
  undo: `set ul=-1`, `setline()`, `set ul=100`.  An unset local
  `'undolevels'` reads `-123456`.  Undo messages: `message clear`, then
  `split(execute('message'), "\n")[-1]`.
- A Ctrl-key mapping has `lhsraw` and `lhsrawalt`; look it up both ways.
  Test keys whose bytes contain 0x80 (`…`) or 0x9b (`ě`), and
  `"\<*C-M>"` (unsimplified).  `<Cmd>` errors: nested `E1136`, no `<CR>`
  `E1255`.  Mapping timeouts in a terminal: `term_wait(buf, ms)` around
  `'timeoutlen'`.
- Function options store `s:F` expanded: expect `expand('<SID>') .. 'F'`.
- `:command`: redefining from another script `E174`; `-complete` without
  `-nargs` is a warning (`E1208` in `v:warningmsg`) in legacy, an error
  in Vim9; a `{ }` body uses Vim9 syntax; `customlist` returning a
  non-List `E1303`; `delcommand -buffer` of another's `E1237`.

## Insert mode, completion, command line

- `feedkeys()` flags: `x` executes until typeahead is empty (ends
  Insert mode as with `<Esc>`); `t` as typed (matters for undo, folds,
  abbreviations, completion); `!` with `x` stays in Insert mode and
  doesn't set `ex_normal_busy` (add a key after a final `<Esc>`, e.g.
  `"\<Esc>0"`); `n` no remap; `i` insert before typeahead.
  `test_edit.vim` uses `'tnix'`.
- **State mid-sequence**: a buffer-local `<F5>` mapping to
  `<C-R>=Func()<CR>` (or `<Cmd>`) that saves `complete_info()`,
  `pumvisible()`, `getline('.')`, `mode()` in a global and returns `''`.
- `test_override('char_avail', 1)` makes code run that is skipped while
  typed keys wait (`'incsearch'`, `showcmd`, completion); reset to 0.
- **Options that take a function** (`completefunc`, `omnifunc`,
  `tagfunc`, `findfunc`, `operatorfunc`, ...): every way to set it in a
  `CheckLegacyAndVim9Success` block: name, `g:` name, `function()` with
  args, funcref, `string(funcref)`, lambda and its string, `s:Func`,
  `<SID>Func`, Vim9 `def` without `s:`; invalid (`E700`); `:setlocal`
  / `:setglobal` (`E764`); survives GC.  The callback appends its args
  to a global list.
- **Data-providing functions** (`'tagfunc'` etc.): record the args each
  caller passes; return each kind of value (list, `[]`, `v:null` =
  fallback, wrong type `E987`, `throw`) at every entry point (jump and
  completion); one small function per side effect (wipe buffer `E987`,
  close window `E1299`, recursion `E986`).  Loop with `for &tagfunc in
  [...]`.  Clean up in `finally`.
- File-name/tag/dictionary completion picks up `src/testdir` files: `cd`
  into an `X` dir (and `cd ..` at the end) or use unusual names.
- `Insert` mode is refused in the sandbox (`E48`), under textlock
  (`E565`, catch with `/^Vim\%((\a\+)\)\=:E565/`), in a non-modifiable
  buffer (`E21`).  `<Del>` etc. need their termcap entry (`let &t_kD =
  "\<Esc>[3;*~"`).
- Popup menu: control the matches with `complete(col, list)` from a
  `<C-R>=` mapping, or a `'completefunc'` returning dicts.  Record
  `pum_getpos()` from a mapping; dumps for size, scrollbar, highlights.
- Command-line completion: `feedkeys(":b SRF\<Tab>\<C-B>\"\<CR>", 'tx')`
  then `@:`; `<C-A>` before `<C-B>` lists all matches.
  `getcompletion(pat, type)` / `'cmdline'` type does not exercise
  wildmenu or keys.  Mid-line state via `\<C-R>=Func()\<CR>`
  (`getcmdline()`, `getcmdpos()`, `getcmdtype()`).  A feature that
  applies to every completion type gets a test per type, off and on.
- **Command-line window**: open, act and close in one `feedkeys(...,
  'x!')`; call helpers from inside.  With `'x'` the window closes itself
  when the keys run out; if it didn't open, a trailing `:q\<CR>` quits
  Vim.  `"<CR>"` in a double-quoted string is literal text, use
  `"\<CR>"`.  Window commands inside give `E11`, a second window
  `E1292`.  `RunVimInTerminal` loads `defaults.vim`: clear the
  `vimHints` augroup first in dump tests.
- Digraphs: `norm! o<C-K>{chars}` helpers; user digraphs can't be
  removed, so don't assume none exist.  `:digraph` layout depends on
  width: match entries.  `:loadkeymap` reads the rest of the file
  (`E105` outside one, inside `if 0` it eats `endif`).
- Right-to-left/shaping: check `getline()` (code points) and
  `ScreenLines()` (shaped, right-aligned) separately.  `set arabic`
  sets `'rl'`, `'arabicshape'`, `'keymap'`, `'delcombine'`; `set
  arabic&` resets only some: reset `keymap=` etc.
- `CursorMovedI`/`TextChangedI` (and `wildtrigger()`) don't fire while
  typeahead is pending: needs `test_override('char_avail', 1)`.
- An `<expr>` mapping runs under textlock: an easy way to test `E565`;
  a completion function that changes text or window must also give
  `E565`.
- Capture the command line after `<Tab>` without running it:
  `"\<C-R>=execute('let x = getcmdline()')\<CR>\<C-C>"`.  Record history
  recall with `cnoremap <F2> <C-\>eSave()<CR>` (`histdel()` first).
  `test_cmdline.vim`'s `SetUp()` maps `<F4>` to save the wildmenu line in
  `g:Sline`.
- Completion features must also work through `'complete'` function
  sources (`F`, `FName`; escape `,` and spaces).
- Check state while in Insert mode: `timer_start(100, 'Cb')` then
  `feedkeys('a', 'tx!')`; the callback asserts and feeds `"\<Esc>"`.
  Prompt buffers are tested in a terminal (`'x'` ends Insert mode); the
  callback must `set nomodified`.

## Search, substitute, tags

- `'incsearch'` needs `test_override('char_avail', 1)` and
  `CheckOption incsearch`; test every command with incremental matching
  (`/`, `?`, `:s`, `:g`, `:v`, `:sort`, `:uniq`, `:vimgrep`).
- Test both regexp engines (`\%#=1`, `\%#=2` or `'regexpengine'`).
- Timeouts: grow input until clearly slower than the timeout, wide
  `assert_inrange()`, scale with `get(g:, 'run_nr', 1)`, flaky,
  `CheckFeature reltime`, maybe `CheckNotAsan`.
- Hard-to-write input files go in `samples/`.
- Search count: `execute(':unsilent :norm! n')` (execute() is silent),
  `set shortmess-=S` (restore `+=S`); `searchcount()` compared as whole
  dicts; `recompute: 0` returns the last *shown* values.
- File search: fixed tree with `mkdir(..., 'pR')`, `cd` into it, set
  `'shellslash'`; each `'path'` form (`,,`, `.`, `**N`, `;` with stop
  dir); upward-search results may include entries above the test dir.
- Substitute: table `[input, cmd, expected]` with the cmd as message;
  same cases for `:s` and `substitute()`; each replacement item with
  `'magic'` on/off; answer `c` prompts in the same `feedkeys()`; `\=`
  that changes windows/buffers must not crash.
- Tags: write the tags file with `"\t"`, keep it sorted when the header
  says so, use line-number addresses unless patterns are tested, `set
  tags&`.  Compare `gettagstack()` with a growing expected list.
- Pattern errors differ per engine (old `E61`/`E62`/`E64`, NFA
  `E871`/`E866`/`E867`): set `re=1`, then `re=2`.  Regexp match tables
  (`tl`) take rows `[engines, pat, text, match, sub1, ...]`.
  `test_override('nfa_fail', 1)` forces the backtracking fallback.
- Deterministic timeouts: a skip expression that sleeps.
  `searchcount().incomplete` is 1 for a timeout, 2 past `maxcount`.
- `\=Func()` with an error replaces matches with `0`; an exception
  caught in `try` changes nothing.
- A tag command runs in the sandbox (`E12` for changing files); `E435`
  is only a message (`v:statusmsg`).  `setlocal tc=` is valid,
  `setglobal tc=` is `E474`.  URL names `E1576` with `'tagsecure'`.
- `glob()` in `sandbox` doesn't use the shell.  `fnamemodify(':S')` and
  `shellescape()` depend on `'shell'` (set `shell=sh`).

## Text properties, folds, signs, matches

- Shared helpers create the types/props and return the expected list;
  after each edit adjust the expected list by exactly what the edit
  should do and compare the whole `prop_list()`.  Check the item that
  starts/ends at the edit (`start_incl`/`end_incl`), undo/redo (close
  undo blocks with `set ul&`), `line2byte()` unchanged for each
  `fileformat` and a buffer with hundreds of lines.
- Virtual text positions need screendumps; cursor movement and
  `screenpos()` without dumps where possible.
- **Lines a property spans**: one piece per line with `start`/`end`
  flags; after removing part of it check the neighbours' flags.  The
  same change made several ways must give the same result: loop over
  the edit commands, rebuild the buffer, pass the command as message.
  A case that doesn't work yet: leave it out with `" TODO:`, never
  assert a value you think is wrong, and no dead code after `return`.
- **Folds**: compare `map(range(1, line('$')), 'foldlevel(v:val)')`
  (or `foldclosed`) for the whole buffer; each `'foldmethod'`;
  `foldtextresult()` instead of a dump; `redraw` before checking
  `'foldexpr'` folds; an error in the expression gives level 0.
- **Expressions set from a modeline run in the sandbox** (`'foldexpr'`,
  `'foldtext'`, `'indentexpr'`, `'statusline'`, ...): needs `set modeline
  modelineexpr nomodelinestrict` (else `E992`); let the function write
  a file and check it is unchanged, then set the option with `:setlocal`
  and check it changes.
- **Signs** are global: start and end with `sign_unplace('*')` and
  `sign_undefine()`; test command and function forms; list functions
  return one result per item; ignore `E255` (no icon support).
- **Matches**: compare a matched cell's `screenattr()` with a plain
  cell, not with a number.  Automatic IDs start at 1000 per window and
  an explicit ID raises the next one to at least ID + 100: keep the
  returned ID instead of hard-coding it.  IDs 1–3 reserved (`E798`),
  `E799`, `E801`.  `getmatches()`/`clearmatches()`/`setmatches()`
  round trip; compare with `getmatches(winid)` afterwards.  End with
  `clearmatches()`.
- Prop types are global unless created with `bufnr`: `silent! call
  prop_type_delete(name)` before adding (`E969`).  `prop_list()` dicts
  include `type_bufnr`.  Virtual text IDs are negative and not reused.
  Code that only runs when the buffer has props needs a prop somewhere.
- No syntax re-run: `syntime on`, `syntime clear`, change, `redraw`,
  `assert_notmatch('Group', execute('syntime report'))`.

## Windows, buffers, tab pages, argument list

- Compare a full layout snapshot (`winlayout()` + `getwininfo()` sizes)
  before and after a failing command.  Derive "how many windows fit"
  from `&lines`/`&columns` (comment the arithmetic); one more gives
  `E36`.  Use window IDs, not numbers, when windows change.
- Changing `&lines`/`&columns`: set `t_WS=` (restore it) and restore
  the size.
- Window commands from textlock (`E565`; `==` on an empty line doesn't
  evaluate `'indentexpr'`), the command-line window (`E11`), and
  autocommands (`E1312`).
- Buffer numbers grow during a run: `%bw!` and use `bufnr()` results.
  Modified buffer: `E89`/`E37`; `:confirm` answered with
  `feedkeys('c', 'L')` (`CheckFeature dialog_con`, `CheckNotGui`).
  Autocommands that wipe the buffer being closed (`E1546`).  Fail the
  same allocation id before each caller.
- Argument list is not restored by the runner: start with `args a |
  %argd`, end with `%argdelete` and `%bw!`; check `argidx()` after every
  command.  `:all`/`:args` lock the list (`E1156` from autocommands).
  `assert_fails()` doesn't work in the command-line window on the
  MS-Windows console; use `try`/`catch`.
- Counts and addresses (tab pages, windows, buffers): each form
  (`3cmd`, `cmd 3`, `.`, `.+1`, `-`, `$`, `0`, `#`) through a
  one-line helper with the command as message; rebuild the state before
  each group; out of range `E16`.
- Fill the screen: `assert_fails('execute "split|"->repeat(&lines)',
  'E36:')`, then try every window-opening command.  Layout of another tab
  page: `winlayout(tp)` + `getwininfo(win_getid(nr, tp))`.  Window order:
  save `win_getid()` per window, compare with `windo` order.  The text
  tabline takes a row in the terminal but not in the GUI.
- `win_splitmove()` keeps the window ID (`E957`, `E855`, `E242`).  After
  a failed `newwin_wvars` allocation end with `tabnew | tabonly`.
- Tab page counts: before the command `E16`, as an argument `E475`.
  Autocommand order per command: a marker command `command -nargs=1 -bar
  C call add(s:li, '=== ' .. <q-args>) | <args>`.
- cwd scope: `execute('verbose pwd')` shows `[global]`/`[tabpage]`/
  `[window]`.  `:confirm :b` with `dialog_con_gui` opens a file browser:
  also `CheckNotFeature dialog_con_gui`.
- `'winfixbuf'` (`E1513`): per command test the refused and the allowed
  form (`!`, a command that opens a window, the same buffer); `:split`
  doesn't copy it; reuse the helpers in `test_winfixbuf.vim`.

## Display, scrolling, screen positions

- Prefer screen text over dumps: `NewWindow(h, w)` / `CloseWindow()`
  (`view_util.vim`, sets `'winfix*'`), `ScreenLines([r1, r2], width)`
  (does `redraw!`), `Screenline(row)`, `screenstring()`,
  `screenattr()`, `screenchar()`; compare joined with `"\n"`.  Dumps
  only for highlighting, borders, scrollbars, overlap.
- `'breakindent'` caches the indent: change an option it depends on and
  `redraw!` before reading the screen.  `_vartabs` twin tests must give
  the same screen.
- With `'nowrap'` and a leftcol the line is drawn by another path: test
  scrolled too.
- Multi-item options (`'listchars'`, `'fillchars'`): each item alone and
  with the items it overrides; `E474` (no colon), `E1511` (wrong
  character count), `E1512` (control or double-width character, also
  written `\\x01`); the option keeps its value after an error.  Give
  every window different characters in one dump to show which value
  was used.
- Concealing (`CheckFeature conceal`): the cursor line is not concealed
  unless `'concealcursor'` includes the mode: put the cursor on another
  line or set `cocu=n`.  Check screen and buffer columns
  (`test_setmouse()` + `<LeftMouse>` + `getcurpos()`); `'concealcursor'`
  per mode needs dumps (`a`, `/e`, `v`).  Put a column-number comment
  above an expected screen string.  Use `setlocal` for `cole`/`cocu`.
- Scrolling: check screen and text position together (`wincol()`,
  `winline()`, `col('.')`) and `winsaveview()` (`topline`, `skipcol`,
  `leftcol`); a dump after every key for step-by-step display changes;
  cases with `'number'`, `'showbreak'`, double-width at the wrap,
  zero-width window, odd/even heights, with and without `'ruler'`.
- Position functions: use one buffer where bytes, characters and cells
  differ (`['', "01\tà4è678", 'Ⅵ', '012345678']`); `'v'` only has a
  value in Visual mode (read it from a `vnoremap <expr>`); out-of-range
  values are clamped (negative `E475`).  `screenpos()`: expect values
  computed from `win_screenpos(winid)`; Tab (`curscol` = last cell) and
  double-width (`endcol` + 1).  `curswant` after `$` is `v:maxcol`.
- Highlight groups are global: save with `HighlightArgs(name)` and
  restore with `hi clear Name` then `exe save` (both `exe save` and
  `hlset()` only set the attributes they name).  `hlset(hlget())` must
  change nothing.  Guard colors with `IsColorable()`.  `gui -f` can't be
  undone: such a test runs last.
- Status line and other format options: read the row with
  `screenstring()` after `redraw!` (row `&lines - 1` is only the bottom
  window's; compute `win_screenpos(0)[0] + winheight(0)`); one item at a
  time with its help quoted; `g:statusline_winid` is the window being
  drawn; an uncaught error clears the option; sandbox via `sandbox
  setlocal stl=%!F()` + `redrawstatus` → `E48`.  Click regions with
  `test_setmouse()`.  Heights after equalizing may differ by one:
  `assert_inrange()`.
- Messages: `execute()` returns a leading `"\n"`; `GetMessages()`;
  `test_ignore_error(pat)` / `('RESET')`.  `-- More --` and hit-enter
  need a terminal; leave the prompt before `StopVimInTerminal()` or the
  `:qa!` is eaten.
- A new screen element must be tested against everything else that
  draws (cmdline, wildmenu, popups, tabline, ruler, vsplit, small
  screen, mouse).  To get values out of a child Vim, let it append them
  to a file and compare after stopping it.
- Syntax engine (`syntax.c`; language syntax files are tested in
  `runtime/syntax/testdir`): one-letter group names and an expected
  string with one character per column, checked with
  `AssertHighlightGroups()`; loop `re=0/1/2`; `'redrawtime'` limits with
  `assert_inrange()`.
- Popup windows: `redraw` then `popup_getpos()`; the runner fails a test
  that leaves a popup open: `popup_clear()`.  Filters record keys in a
  global.  Mouse: `map <F3> :call test_setmouse(5, 2)<CR>` and feed
  `"\<F3>\<LeftMouse>"`.  Loop over `latin1` and `utf-8`.
- `:disassemble`: define the function at script level, match
  `execute('disass s:Name')` with one `..`-joined pattern (`\d` for
  instruction numbers, `<SNR>\d*_`, each line ending `\_s*`, escape
  `[`); also `assert_notmatch()` what an optimization removes, and call
  the function to check the result.
- Files comparing `screenattr()` start with `if !has('gui_running') &&
  has('unix') | set term=ansi | endif`.  Reference attribute without a
  dump: `matchadd('Visual', '.')`, `redraw`, read the cell, clear.
  `HighlightArgs()`/`IsColorable()` live in `test_highlight.vim` only.
  `:hi` output has terminal-dependent color numbers: normalize them.
  `hi Normal ctermbg=N` sets `'background'`.
- `'hlsearch'` shows only after `redraw`.  Call `wincol()` before
  `ScreenLines()` of wrapped lines (updates `skipcol`).  To show an
  option change doesn't clear the screen: `echon` a marker and check it
  is still there after `redraw`.  Setting a `P_CURSWANT` option (even
  `let &ts = &ts`) recomputes `curswant`.
- `screenchars()` on the second cell of a double-width char is `[0]`;
  illegal UTF-8 shows as `<xx>`.  DBCS drawing: loop `cp932`, `cp936`,
  `cp949`, `cp950` in a `RunVim()` child.
- Click regions `%[Func]text%[]`: the callback gets `{button, nclicks,
  minwid, winid, area}` (`winid` 0 for the tabline/tabpanel).  WinBar:
  `amenu 1.10 WinBar.X ...` (current window only).
- Popups: `popup_locate(row, col)`; `popup_getpos()` is `{}` after
  close; `win_execute()` of a window command in a popup `E994`; menus
  pass a 1-based index, `-1` for cancel; `popup_clear(1)` forces.
  Preview window: `wincmd P` without one `E441`.
- Images and exact escape output: run Vim on a pty with `job_start(...,
  #{pty: 1, out_mode: 'raw', out_cb: ...})` and wait for the sequence.

## Vim9 classes, enums, interfaces

- `def Test_*()`; each case a full `vim9script` passed to
  `v9.CheckSourceSuccess()` / `CheckSourceFailure(lines, err, lnum)`;
  always pass `lnum`, full message with names.  Start with the syntax
  errors.
- Test a rule from every place: script level, `:def`, object method,
  class method, lambda/closure in a method, subclass, through `any`
  (one commented case each).  Inheritance three levels deep; errors name
  the defining class.
- Methods as funcrefs and callbacks (timers: `g:WaitForAssert()`);
  `null_object`; `empty()`/`len()` methods.
- Enums: values are singletons (`deepcopy(Color.Green) is
  Color.Green`); `Color.values` is locked; `E1421`–`E1427`.
- Interfaces: contract errors (`E1348`, `E1349`, `E1382`, `E1383`,
  `E1367`) are reported on the `endclass` line.
- Force compiling methods with `defcompile`, `defcompile C` or
  `defcompile C.Method`.  The default `new()` takes every object
  variable (also protected and inherited) as an optional argument;
  `v:none` keeps the initializer.
- Write errors: `E1335` not writable, `E1409` final/const, `E1333`
  protected (names the defining class), `E1360` null object; `:lockvar`
  of a member `E1391`/`E1392`.  `string()` gives `object of C {a: 1}`,
  `class C`, `enum Car.Honda {name: 'Honda', ordinal: 0}`.
- Enums: the same misuse gives other errors at script level and in a
  `def` (`E1421`/`E1012`, `E1424`/`E1012`); `Foo.new()` `E1325`.
- Interfaces can't have static (`E1378`), protected (`E1379`/`E1380`) or
  public (`E1387`) members.  An untyped class variable gets the
  interface type at `new()`, so `E1382` appears only when `new()` runs.

## Variants, restrictions, implementations

- **Command variants** sharing code (quickfix `c`/`l`, global/local):
  one helper `X<what>Tests(cchar)` called per variant.  In
  `test_quickfix.vim` use `s:setup_commands(cchar)` wrappers (`Xgetexpr`,
  `g:Xgetlist`); variant-specific behaviour in `if a:cchar == 'l'`
  (`E776`).  Free lists with `setqflist([], 'f')`, `setloclist(0, [],
  'f')`.
- **A restriction for many commands** (`'winfixbuf'`): one test per
  command, alphabetical, same shape (reset, setup helper, fails and
  changed nothing, the `!` way around).  Cover every entry point (Ex,
  Normal, mouse, quickfix, `:*do`, interfaces).
- **Several implementations** (diff internal/external/`diffexpr`, regexp
  engines, crypt methods): a `Common_<what>()` body called from one
  `Test_` per implementation; for dumps pass the implementation name as
  the extra `VerifyScreenDump()` argument.  Many cases in one terminal
  session: `set autoread`, rewrite files, `:checktime`.  Without dumps:
  `diff_hlID()`, `diff_filler()` (set `diffopt+=foldcolumn:0`).
  Cleanup: `diffoff!`, `set diffopt& diffexpr=`.  An event that may fire
  once or twice: `assert_inrange(1, 2, n)` with a comment.
- Quickfix: the default `'errorformat'` differs per platform (set it);
  the title is `':'` plus the typed command with its indentation (use
  `exe`); every file name creates a buffer (`%bwipe!`);
  `getloclist(0, {'all': 0})` has an extra `filewinid`;
  `QuickFixCmdPre` fires even when the command fails, `Post` doesn't;
  `%v` counts a Tab as 8.  `:grep` tests grep the test file itself.
- Compiler `'errorformat'` without the tool: `cgetfile` sample output and
  check `getqflist()` fields.
- In a loop over implementations, `return` when `VerifyScreenDump()`
  fails so `failed/` isn't overwritten.  `diff(l1, l2, {output:
  'indices'})` tests the algorithm without windows.

## Autocommands, callbacks, fresh Vim, startup

- Hostile autocommands (wipe the buffer, free the list being filled,
  close/split windows, `interrupt()`): assert the error or completion
  and that state is consistent (`bufexists()`, `win_findbuf()`).
  Errors: `E1312`, `E937`, `E812`, `E218` (nesting).
- Record events in a list and compare the whole sequence.  `++once`,
  `++nested`, `E983`.  `v:event` is read-only (`E46`, `E742`).
  `autocmd_get()` compared as a whole list.
- Cleanup that always runs: `defer CleanUpGroup()` right after creating
  the group; `defer test_override('starting', 0)`.
- `CursorHold`, `VimEnter`, `SafeState` don't fire in a test function:
  use a terminal Vim whose handler appends to a file.
  `test_override('starting', 1)` for behavior after startup.
- Callbacks (`listener_add()` etc.): store the args, compare exactly;
  `listener_flush()`; merged vs separate changes; the args list is
  locked (`E741`); adding a listener from a callback (`E1569`); remove
  listeners at the end.
- **Fresh Vim**: some crashes reproduce only in a new instance:
  `RunVim([], [], '-e -s -S Xscript -c qa')`.  Asserts in the child
  don't reach the test: write `v:errors` to a file, quit, and check it
  is `[]` (inside `if RunVim(...)`).
- **Startup**: `RunVim(before, after, args)` (`before` via `--cmd`,
  `after` via `-S`); add `set viminfo+=nviminfo nomore`.
  `RunVimPiped()` feeds stdin.  Argument errors: `system(GetVimCommand()
  .. ' ' .. opt)`, `v:shell_error`, exact lines (`CheckUnix`,
  `CheckNotGui`).  `GetVimProg()` for custom args.  Restore env vars
  set with `setenv()` (`setenv(name, v:null)`).  Load order: each
  plugin appends to a global that the child writes out.
- `WinScrolled`/`WinResized`/`TextChanged` need a terminal child that
  writes `json_encode(v:event)` to a file.  `CursorHoldI` can fire in the
  test Vim with `updatetime=20` and a timer feeding `<Esc>`.  `OptionSet`:
  queue the expected rows in a list, the handler removes one per call.
- `v:exiting` is `v:null` until `VimLeavePre`.  `FileChangedShell`
  records `v:fcs_reason` and sets `v:fcs_choice`; without a handler
  answer with `feedkeys('L', 'L')` before `:checktime`.  A change needs a
  different mtime (`sleep 10m` with `has('nanotime')`, else `sleep 2`).
- `listener_add()` callbacks run on `listener_flush()` or a redraw; a
  change above a pending one flushes it first.  Store `deepcopy(a:l)`.
- Startup: `-es` hides output (add `-V1`); test `$VIMINIT`/`$EXINIT`/
  `'exrc'` with `GetVimProg()` (no `-u NONE`); program names (`view`,
  `rvim`, ...) by symlinking the binary in an `X` dir.
- Restricted mode: child with `-Z`, or `$SHELL` named `false`/`nologin`;
  blocked actions `E145`, interpreter commands `E981` (sandbox is `E48`).
- Shell: a fake shell script that logs `$*` shows the exact command;
  shell option defaults depend on `'shell'` at startup (test in a child
  with `--cmd "set shell=X"`); a List command to `system()` runs without
  a shell; on MS-Windows output has `\r`.
- Signals (`CheckUnix`, probe with `system('kill -l')`): `silent !kill
  -s WINCH ` .. getpid(); after TERM the child is gone (wait for
  `finished`, then `:recover`).  Suspend: run Vim from `term_start('/bin/
  sh')` and resume with `fg`.
- Injection through a buffer name: `bufadd('X|call setline(1,
  "PWNED")')` then run the command; through a file name:
  `X$(touch${IFS}Xmarker)` and assert the marker is absent.

## Indenting and formatting

- Indent scripts are tested in `runtime/indent/testdir`.
- Input heredoc, `setlocal` options (always `ts`, `sw`), command,
  expected heredoc, compare `getline(1, '$')`.  Use `[CODE]` as marker.
  Heredoc Tabs must be literal Tab characters (`\t` is not expanded);
  never `:retab` the file.
- Each flag with several values; re-indenting (`=`) and indenting while
  typing (`o`, `cinkeys`) take different paths.
- `'formatoptions'`: one test per flag with only that flag and
  `'textwidth'`; multibyte flags with double-width chars and `tw=1`/`2`.
- Shifting and tab stops: use widths that don't divide each other
  (`sw=5 ts=7 vts=19,17,11`), `expandtab`, expected text built with
  `repeat(' ', N)`; each entry point (Ex, Normal, Visual, Insert)
  × plain / `'shiftround'` / `'vartabstop'`.  Set all related options on
  one line per case (`set ts=8 sts=0 vts=6 vsts=`), type a different
  letter per case.  `'vartabstop'` errors: trailing comma `E475`, empty
  item `E474`.

## Files: writing, swap, recovery, encryption, sessions, viminfo

- Compare written bytes: `readfile(f, 'B')` with a blob literal
  (`0z6E6F656F6C0A`).  A failed conversion (`E513`) leaves the file
  unchanged.  `set backupskip=` for backups in the temp dir.  Read-only
  files: `setfperm()`, `CheckUnix`, `CheckNotRoot`.  "Changed outside
  Vim" needs mtimes ≥ 2 s apart: rewrite until `getftime()` is ahead.
- **Swap files**: leave one behind like a crash: write, `:preserve`,
  `readblob(swapname(''))`, close the buffer, write the blob back with
  `'D'`.  Answer the prompt with your own `SwapExists` handler setting
  `v:swapchoice` (after `autocmd! SwapExists`); remove it at the end.
  Header fields: PID at bytes 24–27 (little endian; a finished job's PID
  makes Vim delete the swap file; not as root: `IsRoot()`), host at
  `28 + 40`, modified flag at `28 + 80 + 899`.  `swapinfo()` `dirty` is
  a Number.
- **Recovery**: work out endianness and 64-bit from the magic number in
  `SetUp()`; skip other layouts with `Skipped:`.  Damage one field at a
  time from `copy(save_b)`, `:recover`, check the error, `@%` and the
  `???` marker lines.  10000 lines of 100 chars give two pointer-block
  levels.  Copy `samples/` reproducers with
  `writefile(readblob(sample), target, 'D')` (may be a symlink).
- **Patching a file Vim wrote** (undo file): put a marker line
  (`'QQQQQQQQ'`), `:wundo`, find the marker bytes in the blob (assert
  found), splice in crafted bytes with each field commented, `:rundo`,
  check the crafted line is there, then expect the corruption error.
  Byte order makes it platform specific: guard.
- **Malformed binary files**: build the file as a blob (header + bytes
  under test), `writefile(blob, name, 'b')`, check the error for each
  kind of damage (truncated, counts too large, invalid values).
- **Encryption** (`CheckFeature cryptv`; `sodium` for xchacha20): type
  the key with `feedkeys(":X\<CR>foo\<CR>foo\<CR>", 'xt')`; reset `set
  key= cryptmethod&`.  Round trip per method, large enough for several
  blocks, plus decrypting fixed data from an earlier Vim (byte-escape
  string, `xxd -r` hex dump, `samples/`).  `sodium_mlock()` failing
  (`E1230`) is a skip.  Changing the key marks the buffer modified.
- **Sessions/views/viminfo**: round trip, and "destroy" must really
  remove the state (`%bwipe!`, `%argdelete`, `unlet`, `only`) or the
  test passes without restoring anything; also with the item off.  Read
  the written file for what must or must not be there.  Each
  `'sessionoptions'`/`'viewoptions'`/`'viminfo'` item.  Viminfo merge
  uses timestamps: `test_settime(t)` per entry.  Huge lengths: `try` /
  `catch /E342/` and `CheckNotAsan`.  A disabled test keeps its name and
  throws `Skipped:` (renaming hides it).
- `:write` errors: `E45` 'readonly', `E505` r-- file, `E504` `w!` with
  `cpo+=W`, `E212` r-x dir, `E509` no backup, `E139` loaded elsewhere,
  `E140` partial write, `E141` no name, `E676` acwrite, `E17` a
  directory.  `E513` only with `nobackup nowritebackup` (otherwise the
  file is written with ` CONVERSION ERROR`).  A backup is made only when
  overwriting: write twice.  `backupdir=.//` uses `%` path names.
  Copy vs rename: compare the inode (`ls -i`).
- Planted swap file + `v:swapchoice = 'r'` throws `E308`.  Read the
  `E325` text with `feedkeys('e', 'tL')` + `redir` (`Test_nocatch_`);
  `test_override('uptime', 0)` drops `(STILL RUNNING)`.
- Session globals are saved only for names like `g:Mixed_case` with a
  Number/String/Float value; legacy-set string options get a `legacy `
  prefix.  Check mappings/imports from a session in a new Vim.
- Viminfo: `:wviminfo X` merges (`!` overwrites); `:rviminfo` doesn't
  overwrite existing values (`!` does).  Hand-written files start with
  `|1,4`; malformed bar lines are silently ignored.  `:wviminfo` sets
  `'0`.  Globals need `!` in `'viminfo'`.

## Spell checking

- Build a small `.aff`/`.dic` in the test, `:mkspell!`, `set
  spl=Xtest.latin1.spl spell`; `TearDown()` deletes generated files and
  resets `'encoding'`.  Real `en` dictionary: only check what it surely
  contains.
- `spellbadword()` reports a lower-case line start as `'caps'` first:
  clear `'spellcapcheck'`.  A failing `set spell` leaves `'spell'` set
  and later option changes reload the broken file: `set nospell` first.
- `:mkspell` reports most problems as messages, not errors: match
  `execute('mkspell! ...')` output (it names file and line).
- Latin1 text goes in `test_spell.vim` (byte escapes), UTF-8 in
  `test_spell_utf8.vim`.
- Spell files are cached: `set encoding=utf-8` (even unchanged) reloads
  them; needed before rewriting a same-named `.spl`.  Files are found
  through `'rtp'` (`Xtest/spell/Xtest.utf-8.spl`).  Damaged files:
  `Spellfile_Test(content, emsg)`.  `.sug` errors appear only on
  `spellsuggest()`.  Using the real `en` file: add a `SpellFileMissing`
  autocommand flag (the download prompt hangs).

## Filetypes, runtime files, packages, plugins

- Filetype detection (`test_filetype.vim`): file names go in
  `s:GetFilenameChecks()` under the filetype key, keys alphabetical
  (`none` first); list every pattern, paths both as written and under
  another prefix (`/etc/x` and `any/etc/x`); case-sensitive names in
  `s:GetFilenameCaseChecks()`.  Content: `s:GetScriptChecks()`,
  `s:GetScriptEnvChecks()`, `s:GetFalsePositiveChecks()`.  A
  `dist#ft#FT<ext>()` gets `Test_<ext>_file()`: content for each choice,
  the default, and `g:filetype_<ext>`.  Start with `filetype on`, end
  with `filetype off`.
- A file that `:lcd`s and loads runtime files makes `$VIMRUNTIME` and
  `'rtp'` absolute first.
- Runtime files with a maintainer change through the maintainer.
- `:compiler`: check `b:`/`g:current_compiler`, `&l:efm` vs global,
  `verbose set efm`; real tools need `CheckExecutable`, `$LC_ALL=C`,
  `noshellslash`.  Don't hard-code lists that grow: compute from
  `glob('$VIMRUNTIME/compiler/*.vim', 0, 1)`.
- Packages: `SetUp()` creates an absolute `X` tree, points `'packpath'`
  at it and saves `'packpath'`/`'runtimepath'`; `TearDown()` restores
  them and deletes the tree.  Each script sets its own number in a
  global or appends its name to `g:sequence` (shows which ran and in
  which order).  Check `&rtp` placement with `match()` (`[/\\]` for
  paths); running a command twice must not change `&rtp`.
- Completion that cycles: one `feedkeys()` per `<Tab>` count, each
  ending with `')\<C-B>call add(li, '\<CR>`.
- Bundled plugins (`test_plugin_<name>.vim`): `packadd {name}`; skip the
  file when an external program is missing; unit-test script-local
  functions through `<SNR>{sid}_{name}` found in `:scriptnames`; check
  signs, layout, variables and mappings it leaves; mark flaky.
- Load plugins explicitly (`runtime plugin/x.vim`, `packadd x`);
  ftplugin settings need `filetype plugin on` and a `'filetype'`.
  `:packloadall` works once (`!` to repeat).  Names under
  `$XDG_CONFIG_HOME` in `test_filetype.vim` use `s:WhenConfigHome()`.
- Runtime-script security: call the autoload function directly with a
  payload that would set `g:x_injected` or create a marker; set
  external-command globals to `true`/`false` so nothing runs; also check
  normal input still works.  A missing tool: set the plugin's command
  global to `"/"`.
- Unit-test script-local functions via `<SNR>{sid}_{name}` from
  `:scriptnames`, or copy the autoload file and append accessors.
  Generated output (`:TOhtml`) is compared in slices with `samples/`.
  Omni-completion functions are called directly
  (`ccomplete#Complete(1, '')`, then `(0, 'base')`).

## Options

- `util/gen_opt_test.vim` generates a test per option from
  `optiondefs.h` and `options.txt`: add the option to its `test_values`
  dict (`'name': [[valid...], [invalid...]]`) and document it in
  `options.txt`.  Run with `make test_options_all`.
- `test_options.vim`: each invalid value with its error and the value
  kept (add a row to `Test_string_option_revert_on_failure()` for a new
  string option); `+=`/`-=`/`^=` with duplicates; `&g:`/`&l:`/`&opt`,
  `set opt<` and `setlocal opt<` (differ for string vs number/bool);
  value completion via `getcompletion('set display=', 'cmdline')`;
  `sandbox set` → `E48` for secure options; `set opt&` vs `opt&vim`.
- Flag options (`'cpoptions'`, `'shortmess'`, ...): one test per flag
  named after it (`Test_cpo_a`, `Test_cpo_A`, punctuation spelled out:
  `Test_cpo_dollar`); every flag has a test or a comment pointing to it
  or a `" TODO:`.  Set the flag explicitly both ways and assert the two
  different results with input where they differ; don't rely on the
  default value.  `test_override('redraw_flag', 1)` for screen-only
  effects.
- Modelines: save/restore `&modeline` (default depends on `IsRoot()`).
  Open the file you wrote; an "ignored" case also checks a valid line in
  the same format.  `'modelinestrict'` skips disallowed options
  silently (set `nomodelinestrict` to test others; `E520` for itself).
  Version forms with `v:version ± 100`.  Errors: secure options `E520`,
  expression options without `'modelineexpr'` `E992`, unsafe characters
  in `ft`/`syn`/`keymap` `E474`; a new restricted option gets a line in
  the matching list.  `verbose set ts?` → `Last set from modeline line
  N`.  A value from a modeline is unsafe (`nowrap` forces `extends:>`).
- Values set in the sandbox, a modeline or secure mode are evaluated in
  the sandbox later, also after copying to a new buffer.  An unset
  global-local value reads `-1` (Number) or `''` (String).  Not every
  invalid value keeps the old one (`set sw=-1` gives `E487` and sets it
  from `'ts'`).  In a diff window modeline `foldmethod`/`wrap` are
  ignored.  A helper that checks many options skips one with
  `if !exists('+' .. name) | return | endif`.

## Terminal, jobs, timers, channels, client-server

- More screendump details: `VerifyScreenDump(buf, name, #{rows: 4})`
  compares part of the screen; returning to an earlier state reuses that
  dump name; a fourth argument names the variant; variants by parameter
  (`Run_Test_x(arg)` builds the dump names, `Test_x()` calls it per
  value).  Clear a message before a dump (`:silent undo | echo`).
- `Run_shell_in_terminal({})` / `StopShellInTerminal(buf)` (sets
  `g:job`, flaky); `term_start(cmd, opts)` for a command.  Never sleep:
  `WaitForAssert()` on `job_status()`, `getline('$')`, `term_getline()`.
  `TermWait(buf)` waits longer on retries.
- Flaky retries: 5 (10 on macOS, `g:max_run_nr`); a retry stops when
  the same message repeats unless `g:giveup_same_error = 0`.
- Platform commands in a helper (`Get_cat_123_cmd()`); Python helpers
  via `PythonProg()` / `CheckPython()`.
- Each `:terminal ++arg` has a `term_start()` option: test both.
  `term_scrape()`, `term_getattr()`, `term_getcursor()` instead of dumps.
  A running job: `E948` on unload; test `:qall` in a child Vim.  Output
  reaches the buffer only in Terminal-Normal mode (`CTRL-W N`).
  `term_getstatus()` is `running` / `running,normal`.
  `'termwinsize'` forms: `6x40` fixed, `10*50` minimum, zeros follow the
  window, huge values capped.
- Child readiness: the script calls `echoraw(TermNotifyParentCmd(v:true))`
  and the test `WaitForChildNotification()` (not on MS-Windows);
  `WaitForTermCurPosAndLinesToMatch()` before a dump.
- Malformed terminal output must keep `term_getstatus()` `running`.
- Cleanup: `test_override('ALL', 0)`, `Terminal*` autocommands,
  `%bwipe!` hidden terminals.  `gui -f` tests named `Test_zz*_` run
  last (`CheckCanRunGui`, `test_ignore_error('E285:')`).  A CI-only
  failure skips with `'Skipped: FIXME: ...'`.
- Timers (`CheckFeature timers`, flaky): the runner's timer means
  `timer_info()` starts with one entry and `timer_stopall()` stops it
  too.  Measure with `WaitFor()`'s return value and `assert_inrange()`
  (a timer may fire 1 ms early; wider bounds on mac/no `reltime`).  A
  repeating timer stops after three consecutive errors.
  `Standby(msec)` waits in `getchar()` so timers run; `Resume()` ends
  it.  Add a safety timer that feeds `CTRL-C`.
- Channels: the other end is a Python script (`test_channel*.py`; add a
  command to it for new behaviour); skip without Python.  Socket tests
  run a `Ch_<what>(port)` body through `s:run_server()` (`RunServer()`),
  with `_ipv6` (`CheckIPv6`) and `_unix` variants.  Callbacks store into
  `g:Ch_*`; wait for them.  Jobs end with `job_stop()` and a wait for
  `dead`.  Never leave `ch_logfile()` enabled.
- Client-server: start the server with `job_start(GetVimCommand() ..
  ' --servername X', {'stoponexit': 'kill', 'out_io': 'null'})` and wait
  for `serverlist()`; server and test Vim must use the same backend.
  Stop with `remote_send(name, "\<C-\>\<C-N>:qa!\<CR>")`, wait for
  `dead`, fall back to `job_stop(job, 'kill')`.  X11 needs
  `Check_X11_Connection()`.  `remote_peek()`: accept both outcomes.
  Socket dir permissions/symlink: `E1589` (point `$TMPDIR` /
  `$XDG_RUNTIME_DIR` at an `X` dir and restore).
- Terminal files set `let $PROMPT_COMMAND=''`; `test_override(
  'vterm_title', 1)` keeps buffer names stable.  Exact bytes to libvterm:
  `term_start([&shell, &shellcmdflag, 'printf "%s" ' .. shellescape(s)])`.
  Bytes Vim sends to a job: a `sh` script with `stty raw -echo` and
  `dd bs=1 count=N >Xout`.  Terminal API from a child Vim: `set title`,
  `t_ts=\<Esc>]51;`, `t_fs=\x07`, `titlestring` JSON `["call","Tapi_X",
  [...]]`.
- Jobs: a short job may already be `dead` (`'^\%(dead\|run\)$'`); end
  with `job_stop()` in `finally`.  `test_channel_pipe.py` takes `echo`,
  `echoerr`, `double`, `quit`.  `out_io: 'buffer'` adds a `Reading from
  channel output...` line unless `out_msg: 0`.  Option errors `E475`,
  `E920`, `E921`, `E915`.  LSP: a round trip (`ch_evalexpr()`) before
  checking notifications.  Unix socket paths max 108 bytes.
  `ch_logfile('')` stops any log: such tests are `Test_zz_`.
- Timers: alternating errors don't stop a repeating timer;
  `timer_info(id)[0].repeat` counts down.  Low-level input from a timer
  works only on Unix or in the GUI.
- Client-server: names in use get a number (`assert_match()`); don't
  reuse a `channel:<port>`; socketserver ids give `E1564`, X11 `E573`;
  sending to itself needs `feedkeys('', 'x')`; gVim on MS-Windows shows a
  dialog for `--remote-expr` (skip).  A socketserver child needs the test
  Vim to serve too (`remote_startserver()`).

## Terminal codes, keys, mouse, GUI, MS-Windows

- Raw terminal input: `feedkeys(seq, 'Lx!')` (Unix and GUI only;
  `CheckUnix`, `CheckNotGui`).  Save/restore `&term`, `&mouse`,
  `&ttymouse`; `set term=xterm mouse=a`,
  `test_override('no_query_mouse', 1)`, `WaitForResponses()`.
- `util/mouse.vim`: `MouseLeftClick(row, col)`, `MouseLeftRelease()`,
  `MouseLeftDrag()`, `MouseWheelUp()`; loop over `g:Ttymouse_values +
  g:Ttymouse_dec + g:Ttymouse_netterm` with the protocol as message.
- Modified keys: `GetEscCodeCSI27(key, mod)` and `GetEscCodeCSIu()`
  (2 Shift, 3 Alt, 5 Ctrl); run the same checks with both funcrefs.
- Terminal responses need `t_RV` set and
  `test_override('term_props', 1)`; such tests are named
  `Test_xx<nn>_...` to run late.  Save/restore `t_` options.
- GUI (`test_gui.vim`, `CheckCanRunGui`; other files run in the GUI
  with `make GUI_FLAG=-g`): `util/setup_gui.vim` `GUISetUpCommon()` /
  `GUITearDownCommon()` must be called from functions named exactly
  `SetUp()`/`TearDown()` (case sensitive).  `test_gui_event(type, args)`
  then `feedkeys('...', 'Lx!')`.  Toolkit differences: branch on
  `has('gui_gtk')` etc. and skip with `throw g:not_supported .. 'x'`.
- MS-Windows (`test_mswin_event.vim`, `CheckMSWindows`):
  `test_mswin_event()` with virtual key codes (`s:VK`); read with
  `getcharstr(0)`; flush typeahead before each table; console and GUI
  execute keys differently.  Check changes in the Windows CI jobs.
- See how a key was decoded: Insert `CTRL-K` + key inserts its name
  (`<S-Home>`).  `feedkeys(substitute(&t_ku, '\*', '', 'g'), 'Lx!')`
  sends the real code.  A response changes `'ttymouse'`/`'background'`
  only when the user didn't set them (`test_option_not_set()`); compare
  the whole `terminalprops()`.  `t_` options may not exist (`exists('&t_K1')`).
- `Mouse*Code()` helpers return the sequence (join them for
  multi-clicks); the action helpers send it.  Bracketed paste:
  `set term=xterm`, `"\<Esc>[200~text\<Esc>[201~"`.  Modeless selection:
  click codes on the command line, `"\<C-Y>"` copies to `@*`.
- GUI: `test_gui_event()` dict keys per type (mouse `button`/`row`/`col`/
  `multiclick`/`modifiers`; a missing key returns false), process with
  `feedkeys('', 'Lx!')`.  A GUI child: `system(GetVimCommand('X') ..
  ' -g')` with `au GUIEnter * call writefile(...) | qall`.  In the GUI,
  `set guioptions-=e` for a text tabline.
- MS-Windows events: the GUI processes them after `feedkeys("\<Esc>",
  'Lx!')`, the console with `test_mswin_event('key', {'execute':
  v:true})`; mouse moves need `set mousemev`.  `let $HOME = ''`
  recomputes the home directory.
- Wayland: set `$WAYLAND_DISPLAY` in each test; check results with
  `system('wl-paste -n')`.

## Language interfaces

- Put `Test_skip_<lang>()` with the command inside `if 0` before the
  file's `CheckFeature`, so a build without it checks the skipped
  command parses.  Interpreter errors have no E number and differ per
  version (Lua: pick the text from `luaeval('vim.lua_version')`; Tcl: a
  `TclEval()` helper).  Reset interpreter globals (`py3 del x`, `lua x =
  nil`); `TearDown()` runs GC.
- Python: errors arrive as `Vim(py3):<Class>: msg`; the helper `ee(expr)`
  appends `expr:repr(exc)` to the buffer for many cases at once (extend
  its version normalization).  `vim.bindeval()` returns live references,
  `vim.eval()` copies; Python 3 function results are bytes.  py2/py3
  conflict in some builds (`E836`/`E837`): separate files; `set pyx=`
  before `CheckFeature` in pyx files.  `py3eval(expr, {'d': d})` passes
  locals by reference.  Perl: `VIM::Eval("assert_equal(...)")`.

## Other areas

- gettext: `ru_RU/LC_MESSAGES/__PACKAGE__.po/.mo` in testdir,
  `bindtextdomain("__PACKAGE__", getcwd())`, `language messages ru_RU`;
  a missing locale (`E197`) is a skip.
- Help: `:help x`, check `getline('.') =~ '\*tag\*'`, `helpclose`; a fake
  help tree per language (`doc/tags`, `tags-xx`) with `'rtp'`,
  `'helpfile'`, `'helplang'`.  `:hardcopy > X` (needs `postscript`): the
  first line matches `PS-Adobe`; a `'printexpr'` must delete
  `v:fname_in`.
- Input methods without a real IM: recording `'imactivatefunc'` /
  `'imstatusfunc'` with `iminsert=2`.
- Netbeans: `test_netbeans.py` via `s:run_server('Nb_<what>')`; append a
  command name to `Xcmdbuf` to make the server act; traffic is logged
  to `Xnetbeans`.
- xxd: `s:xxd_cmd` comes from `$XXD`, exported only by the testdir
  Makefile (run `make test_xxd`); filter with `%!` or `0r!` and
  compare lines; memory regressions add `2>&1` and
  `assert_notmatch('runtime error')`.
- Memory usage: RSS from `ps -o rss= -p pid` of a `term_start()` Vim,
  wide `assert_inrange()`, `CheckNotAsan`, flaky.
