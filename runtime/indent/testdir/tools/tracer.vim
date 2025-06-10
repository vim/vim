vim9script

# Whenever indent plugins contain "search*()" lines explicitly annotated with
# "VIM_INDENT_TEST_TRACE_(START|END)" comment markers; this script can then be
# used as shown to measure and record elapsed time for such decorated calls.
#
# Usage:
#	cd runtime/indent
#	vim -u NONE -S testdir/tools/tracer.vim \
#		html.vim javascript.vim \
#		../autoload/python.vim ../autoload/dist/vimindent.vim
#	git diff
#	make clean test
#	vim testdir/00-TRACE_LOG.fail

def GenerateTempletForTracing(fname: string, vname: string): list<string>
  #### ONLY INSTRUMENT "search*()"es FOR INDENT TESTS.

  const templet: list<string> =<< trim eval END

  if getcwd() =~# '\<runtime/indent$'

  def! g:IndentTestTrace(id: string, start: list<number>, result: any): any
    const end: list<number> = reltime(start)

    if !has_key(g:indent_test_trace_times, id)
      g:indent_test_trace_times[id] = []
    endif

    g:indent_test_trace_times[id]
	->add(reltimefloat(end))
    return result
  enddef

  def! g:IndentTestInitTracing()
    # Possibly use a later "{fname}", cf. ":runtime indent/foo.vim".
    autocmd_add([{{
	replace: true,
	group:	'tracing',
	event:	'QuitPre',
	bufnr:	bufnr(),
	cmd:	'g:IndentTestWriteTraceTimes()',
      }}])
    g:indent_test_trace_times = {{}}
  enddef

  def! g:IndentTestWriteTraceTimes()
    # Anticipate usage by multiple languages.
    const token: string = printf('%02x', (rand() % 26))
    writefile(['" {fname}:',
	    "let {vname}_" .. token .. " = " .. string(g:indent_test_trace_times),
	    "let {vname}_" .. token .. "_summary = " .. string(g:indent_test_trace_times
		->items()
		->reduce((outer: dict<dict<any>>, times: list<any>) =>
		    extend({{[times[0]]: times[1]
			      ->copy()
			      ->reduce((inner: dict<any>, v: float) =>
				  extend({{
				      min: inner.min < v ? inner.min : v,
				      max: inner.max > v ? inner.max : v,
				      sum: (inner.sum + v),
				      avg: ((inner.sum + v) / inner.count),
				    }},
				    inner,
				    "keep"),
				  {{
				      min: v:numbermax - 0.0,
				      max: v:numbermin + 0.0,
				      sum: 0.0,
				      avg: 0.0,
				      count: len(times[1]),
				  }})}},
			    outer),
			{{}}))],
	(!empty($VIM_INDENT_TEST_LOG) && filewritable($VIM_INDENT_TEST_LOG))
	    ? $VIM_INDENT_TEST_LOG
	    : "testdir/00-TRACE_LOG.fail",
	"a")
  enddef

  call g:IndentTestInitTracing()

  else

  def! g:IndentTestTrace(_: string, _: list<number>, result: any): any
    return result
  enddef

  endif

  END
  return templet
enddef

def InstrumentMarkedEntry(): bool
  const marker_start: string = 'VIM_INDENT_TEST_TRACE_START'
  const start: number = search('\C\<' .. marker_start .. '\>', 'ceW')

  if start == 0
    return false
  endif

  const marker_end: string = 'VIM_INDENT_TEST_TRACE_END'
  const end: number = search('\C\<' .. marker_end .. '\>', 'ceW')

  if end == 0
    return false
  endif

  const tracee: list<string> = matchlist(
    getline(start + 1),
    '\(^.\+\)\(\<search\%(pair\)\=\%(pos\)\=\s*(.*$\)')

  if empty(get(tracee, 1, '')) || empty(get(tracee, 2, ''))
    return false
  endif

  const end_line: string = getline(end)
  const tracer: string = printf('%sg:IndentTestTrace("%s", reltime(), %s',
      tracee[1],
      strpart(end_line, (stridx(end_line, marker_end) + strlen(marker_end) + 1)),
      tracee[2])

  if (end - start) > 1
    setline((start + 1), tracer)
    setline((end - 1), getline(end - 1) .. ')')
  else
    setline((start + 1), tracer .. ')')
  endif

  return true
enddef

def ProcessIndentPluginCmdlineArgs()
  const names: list<string> = range(char2nr('a'), char2nr('z'))
      ->map((_: number, n: number) => nr2char(n, true))
  var entries: number = 0
  var next: number = 0

  for fname: string in argv(-1)
    if filereadable(fname) && filewritable(fname)
      execute 'new ' .. fname
      call cursor(1, 1)

      while InstrumentMarkedEntry()
	entries += 1
      endwhile

      if entries > 0
	append(1, GenerateTempletForTracing(fname, get(names, next, names[-1])))
	wq
      endif

      entries = 0
      next += 1
    endif
  endfor
enddef

if empty(system('git status --porcelain=v1'))
  ProcessIndentPluginCmdlineArgs()
endif

quitall

# vim:fdm=syntax:sw=2:ts=8:noet:nosta:
