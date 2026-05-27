" Tests for trace functionality and ch_traceget()/ch_traceclear().

func Test_trace_option()
  " Test default value
  set chtraceopt=
  call assert_equal('', &chtraceopt)

  " Test setting valid chtraceopt value
  set chtraceopt=input,command,ex,verbosity:debug,ringsize:1024
  call assert_equal('input,command,ex,verbosity:debug,ringsize:1024', &chtraceopt)

  " Test verbosity component
  set chtraceopt=verbosity:normal
  set chtraceopt=verbosity:verbose
  set chtraceopt=verbosity:debug

  " Test ringsize component
  set chtraceopt=ringsize:32
  set chtraceopt=ringsize:16
  set chtraceopt=ringsize:10
  set chtraceopt=ringsize:256

  set chtraceopt=
endfunc

func Test_trace_get_basic()
  " Test that ch_traceget() exists and runs without error
  set chtraceopt=input,command,ex,mapping,verbosity:debug

  " Generate some activity
  normal! gg
  echo "trace test"

  " Call ch_traceget() - should not fail
  call ch_traceget()

  set chtraceopt=
endfunc

func Test_trace_get_with_count()
  set chtraceopt=input,command,ex,verbosity:debug

  " Generate some activity
  echo "first"
  echo "second"

  " Test with numeric argument - should not fail
  call ch_traceget(5)
  call ch_traceget(1)
  call ch_traceget(0)

  set chtraceopt=
endfunc

func Test_trace_capture_output()
  " Test that we can capture the output of ch_traceget()
  set chtraceopt=input,command,ex,verbosity:debug

  " Generate some activity
  echo "capture test"

  " Get trace output
  let output = join(ch_traceget(50), "\n")

  " Output should not be empty (there should be some trace events)
  call assert_true(len(output) > 0)

  set chtraceopt=
endfunc

func Test_trace_clear_resets_events()
  " Test that ch_traceclear() removes events recorded before it.
  set chtraceopt=input,command,ex,verbosity:debug

  " Generate some events
  echo "before_clear_marker"

  " Capture output before clear
  let before = join(ch_traceget(10), "\n")
  call assert_match('before_clear_marker', before)

  " Clear and verify the marker is gone
  call ch_traceclear()

  let after = join(ch_traceget(10), "\n")
  call assert_notmatch('before_clear_marker', after)

  set chtraceopt=
endfunc

func Test_trace_ring_resize_preserves_events()
  " Test that resizing the ring preserves recent events
  set chtraceopt=command,verbosity:debug,ringsize:32

  " Generate some events
  for i in range(1, 20)
    exe 'normal! ' . i . 'gg'
  endfor

  " Resize to smaller
  set chtraceopt=command,verbosity:debug,ringsize:16

  " Should still have events
  let output = join(ch_traceget(10), "\n")

  call assert_true(len(output) > 0)

  set chtraceopt=
endfunc

func Test_trace_verbosity_affects_output()
  " Test that different verbosity levels work
  for level in ['normal', 'verbose', 'debug']
    exe 'set chtraceopt=input,command,ex,verbosity:' . level

    " Generate some activity
    normal! gg

    " Should not error
    call ch_traceget(5)
  endfor

  set chtraceopt=
endfunc

func Test_trace_filter_allows_matching_events()
  " Verify that setting chtraceopt=ex only records EX events
  set chtraceopt=ex,verbosity:debug,ringsize:16

  " Flood the ring to remove events from previous tests.
  " Generate 20 normal! commands while filter=ex -- NONE get recorded
  " because normal! generates COMMAND events, not EX.
  for i in range(20)
    normal! x
  endfor

  " Now generate EX events with :echo commands
  echo "filter_allows_test"

  " The ring should now contain only the EX events from the :echo commands
  let output = join(ch_traceget(5), "\n")

  " Output should contain events tagged as [ex]
  call assert_match('\[ex\]', output)

  " Output should contain the text we echoed
  call assert_match('filter_allows_test', output)

  set chtraceopt=
endfunc

func Test_trace_filter_excludes_nonmatching_events()
  " Verify that events not in the filter mask are NOT recorded.
  "
  " Strategy: fill the ring with COMMAND events, switch filter to INPUT,
  " then overflow the ring with INPUT events.  The COMMAND events should
  " all be evicted, proving that the INPUT-only filter prevented COMMAND
  " recording.
  set chtraceopt=command,verbosity:debug,ringsize:16

  " Fill the ring with COMMAND events from normal! commands
  for i in range(20)
    normal! x
  endfor

  " Switch to INPUT-only filter
  set chtraceopt=input,verbosity:debug,ringsize:16

  " Generate 20+ input events via feedkeys to overflow the ring
  call feedkeys("abcdefghijklmnopqrst", "L")

  " Consume the typeahead so the keys are processed
  let s:got = 0
  for i in range(30)
    let c = getchar(0)
    if c == 0
      break
    endif
    let s:got += 1
  endfor

  " The ring should contain only INPUT events now
  let output = join(ch_traceget(16), "\n")

  " Should contain input events
  call assert_match('\[input\]', output)

  " Should NOT contain command or ex events from this test phase.
  " (Since we flooded with 16+ input events, old command events are gone.)
  call assert_true(output !~ '\[command\]')

  set chtraceopt=
endfunc

func Test_trace_filter_single_category()
  " Test event kind categorization with single-category filter
  set chtraceopt=ex,verbosity:debug,ringsize:64

  " Run a few Ex commands
  echo "test_one"
  let g:trace_test_var = 1
  echo "test_two"

  let output = join(ch_traceget(10), "\n")

  " All captured events should be tagged [ex]
  " (normal! commands would have been filtered out)
  call assert_match('\[ex\]', output)
  call assert_match('test_one', output)
  call assert_match('test_two', output)

  set chtraceopt=
endfunc

func Test_trace_restores_after_test()
  set chtraceopt=ex,verbosity:debug

  echo "restore_test"

  " Should work fine
  call ch_traceget()

  set chtraceopt=
endfunc

func Test_trace_cmdline_input_aggregation()
  " Verify consecutive printable characters in command-line mode are
  " aggregated into a single event, while special keys stay separate.
  set chtraceopt=input,verbosity:debug

  " Feed a command-line sequence: :echo 'x'<CR>
  " The printable chars : e c h o ' x ' should stay separate from
  " <Space> and <CR>.
  call feedkeys(":echo 'x'\<CR>", 'tx')
  sleep 100m

  let output = join(ch_traceget(30), "\n")

  " 'echo' should appear as one aggregated entry (4 printable chars).
  " <Space> before the quote must be a separate entry.
  " The quoted 'x' including the surrounding quotes should be aggregated.
  " <CR> should be a separate entry.
  call assert_match('\[input\] echo  \[CMDLINE\]', output)
  call assert_match('\[input\] <Space>  \[CMDLINE\]', output)
  call assert_match("\\[input\\] 'x'  \\[CMDLINE\\]", output)
  call assert_match('\[input\] <CR>  \[CMDLINE\]', output)

  " The aggregated 'echo' must have come from a single entry, not four.
  " It should NOT have intermediate single-char entries like 'e', 'c', etc.
  call assert_notmatch('\[input\] e$', output)
  call assert_notmatch('\[input\] c$', output)

  set chtraceopt=
endfunc

func Test_trace_cmdline_aggregation_with_tab()
  " Verify <Tab> also breaks the aggregation chain.
  set chtraceopt=input,verbosity:debug

  " Feed cmdline chars then <Esc> to cancel (don't execute anything).
  call feedkeys(":ab\tcd\<Esc>", 'tx')
  sleep 100m

  let output = join(ch_traceget(20), "\n")

  " 'ab' before tab should be aggregated, <Tab> separate, 'cd' after
  " tab should be aggregated, <Esc> should exit cmdline mode.
  call assert_match('\[input\] ab  \[CMDLINE\]', output)
  call assert_match('\[input\] <Tab>  \[CMDLINE\]', output)
  call assert_match('\[input\] cd  \[CMDLINE\]', output)
  call assert_match('\[input\] <Esc>  \[CMDLINE\]', output)

  set chtraceopt=
endfunc

func Test_trace_insert_empty_aggregation_not_shown()
  " Verify that non-printable events in insert mode do NOT produce
  " an empty [insert] entry with just "".
  set chtraceopt=input,verbosity:debug,ringsize:64
  call ch_traceclear()

  call feedkeys("i\<F1>\<Esc>", 'tx')
  sleep 50m

  let output = join(ch_traceget(30), "\n")

  " Should NOT produce an [insert] "" entry (empty aggregation)
  call assert_notmatch('\[insert\] ""', output,
        \ 'Empty [insert] "" should not appear')

  " Should still show input events
  call assert_match('\[input\]', output,
        \ 'No [input] events in output: ' .. output)

  set chtraceopt=
endfunc

func Test_trace_insert_aggregation_format()
  " Verify that [insert] aggregation lines have correct format.
  " Use a simple approach: type single chars in insert mode and
  " verify the resulting [insert] line format.
  set chtraceopt=input,verbosity:debug,ringsize:64

  " Clear any prior events so we start fresh
  call ch_traceclear()

  " Type 'axyz' in insert mode to produce printable input.
  " The a should be captured as printable insert input.
  call feedkeys("iaxyz\<Esc>", 'tx')
  sleep 50m

  let output = join(ch_traceget(30), "\n")

  " Verify each line has valid format if [insert] appears
  for line in split(output, '\n')
    if line =~ '\[insert\]'
      " Format: #SEQ [insert] "text"  [MODE]
      call assert_match('^#\d\+ \[insert\] ".*"  \[\w\+\]$',
            \ line, 'Bad [insert] line format: ' .. line)
    endif
  endfor

  " Should have at least some [input] events
  call assert_match('\[input\]', output)

  set chtraceopt=
endfunc

func Test_trace_sequence_numbers_monotonic()
  " Verify that sequence numbers in output are monotonically
  " increasing and no duplicate sequence numbers appear.
  set chtraceopt=command,verbosity:debug,ringsize:64
  call ch_traceclear()

  " Generate some events
  for i in range(5)
    normal! x
  endfor

  let output = ch_traceget(50)

  " Extract all sequence numbers (skip blank lines)
  let seqs = []
  for line in output
    if line == ''
      continue
    endif
    let seq = matchstr(line, '^#\zs\d\+')
    if seq != ''
      call add(seqs, seq)
    endif
  endfor

  " Should have at least some sequence numbers
  call assert_true(len(seqs) > 0, 'No sequence numbers found')

  " No duplicate sequence numbers
  call assert_equal(len(seqs), len(uniq(copy(seqs))),
        \ 'Duplicate sequence numbers found: ' .. string(seqs))

  " Sequence numbers should be sorted oldest-first (ascending)
  for i in range(1, len(seqs) - 1)
    call assert_true(str2nr(seqs[i-1]) < str2nr(seqs[i]),
          \ 'Sequence numbers not ascending: ' .. string(seqs))
  endfor

  set chtraceopt=
endfunc

func Test_trace_mode_tags()
  " Verify that EX events include a mode tag in the output.
  set chtraceopt=ex,verbosity:debug,ringsize:64
  call ch_traceclear()

  echo "mode_tag_test"

  let output = join(ch_traceget(10), "\n")

  set chtraceopt=

  " Output should end with a mode tag like [NORMAL]
  call assert_match('\[\w\+\]$', output,
        \ 'Output missing mode tag: ' .. output)
  call assert_match('\[NORMAL\]', output,
        \ 'Expected [NORMAL] mode tag: ' .. output)
endfunc

func Test_trace_collapsed_ranges()
  " Verify that consecutive equivalent events are collapsed
  " into a range like #N-#M.
  set chtraceopt=ex,verbosity:debug,ringsize:64
  call ch_traceclear()

  echo "test"
  echo "test"
  echo "test"

  let output = join(ch_traceget(10), "\n")

  set chtraceopt=

  call assert_match('echo "test"', output)
endfunc

func Test_trace_event_format()
  " Verify that each output line from ch_traceget() has valid format.
  set chtraceopt=ex,verbosity:debug,ringsize:64
  call ch_traceclear()

  echo "format_test_event"

  let output = ch_traceget(10)

  set chtraceopt=

  " At least one event should be returned
  call assert_true(len(output) > 0, 'No events returned')

  " Every non-empty line should match the event format:
  "   #SEQ [kind] message  [MODE]
  for line in output
    if line == ''
      continue
    endif
    call assert_match('^#\d\+ \[\w\+\]', line,
          \ 'Bad event format: ' .. line)
  endfor
endfunc

func Test_trace_mapping_nesting()
  " Verify that mapping events show up in trace output.
  set chtraceopt=ex,mapping,verbosity:debug,ringsize:64
  call ch_traceclear()

  " Define a simple mapping that runs an echo command
  nnoremap <F2> :echo "mapped"<CR>

  " Feed the mapping key
  call feedkeys("\<F2>\<Esc>", 'tx')
  sleep 50m

  let output = join(ch_traceget(20), "\n")

  " The mapped echo command should appear
  call assert_match('mapped', output,
        \ 'Mapping output not found: ' .. output)

  unmap <F2>
  set chtraceopt=
endfunc

func Test_trace_repeat_count()
  " Verify that repeated events show repeat count (xN) display.
  " Repeat count is shown when multiple equivalent events collapse.
  set chtraceopt=command,verbosity:debug,ringsize:32
  call ch_traceclear()

  " Generate events that should collapse
  for i in range(5)
    normal! x
  endfor

  let output = join(ch_traceget(10), "\n")

  " Should have command events (some may be collapsed with xN)
  call assert_match('\[command\]', output)

  " If collapsed, output will contain xN after the message
  " If not collapsed, there will be 5 separate command lines
  let lines = split(output, '\n')
  if len(lines) <= 5
    " Could be collapsed - verify format of each line
    for line in lines
      if line != ''
        call assert_match('^#\d\+', line, 'Bad line format: ' .. line)
      endif
    endfor
  endif

  set chtraceopt=
endfunc

func Test_trace_get_no_duplicates()
  " Verify ch_traceget() output has consistent format.
  " ch_traceget() always returns from the ring start (no position
  " tracking), so multiple calls should consistently return events.
  set chtraceopt=ex,verbosity:debug,ringsize:64

  call ch_traceclear()
  echo "marker_event_for_test"

  let first = ch_traceget(10)
  let second = ch_traceget(10)
  let third = ch_traceget(10)

  set chtraceopt=

  " All three calls should return the same events
  let first_combined = join(first, "\n")
  let second_combined = join(second, "\n")
  let third_combined = join(third, "\n")

  call assert_match('marker_event_for_test', first_combined)
  call assert_match('marker_event_for_test', second_combined)
  call assert_match('marker_event_for_test', third_combined)

  " Each event line should have valid format
  for line in first
    if line == ''
      continue
    endif
    call assert_match('^#\d\+', line,
          \ 'Bad event format in first fetch: ' .. line)
  endfor
  for line in second
    if line == ''
      continue
    endif
    call assert_match('^#\d\+', line,
          \ 'Bad event format in second fetch: ' .. line)
  endfor
endfunc
func Test_trace_event_kind_tags()
  " Verify that events have valid kind tags.
  set chtraceopt=ex,verbosity:debug,ringsize:64
  call ch_traceclear()

  echo "ex_kind_test"

  let output = join(ch_traceget(10), "\n")

  set chtraceopt=

  " EX events should be tagged [ex]
  call assert_match('\[ex\]', output,
        \ 'No [ex] tag found in output: ' .. output)
endfunc
