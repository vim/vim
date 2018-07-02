" Tests for the Tcl interface.

if !has('tcl')
  finish
end

" Helper function as there is no builtin tcleval() function similar
" to perleval, luaevel(), pyeval(), etc.
func TclEval(tcl_expr)
  let s = split(execute('tcl ' . a:tcl_expr), "\n")
  return (len(s) == 0) ? '' : s[-1]
endfunc

func Test_tcldo()
  " Check deleting lines does not trigger ml_get error.
  new
  call setline(1, ['one', 'two', 'three'])
  tcldo ::vim::command %d_
  bwipe!

  " Check that switching to another buffer does not trigger ml_get error.
  new
  let wincount = winnr('$')
  call setline(1, ['one', 'two', 'three'])
  tcldo ::vim::command new
  call assert_equal(wincount + 1, winnr('$'))
  %bwipe!
endfunc

" Test :tcldo with a range
func Test_tcldo_range()
  new
  call setline(1, ['line1', 'line2', 'line3', 'line4'])
  2,3tcldo set line [string toupper $line]
  call assert_equal(['line1', 'LINE2', 'LINE3', 'line4'], getline(1, '$'))
  bwipe!
endfunc

" Test ::vim::beep
func Test_vim_beep()
  call assert_beeps('tcl ::vim::beep')
endfunc

" Test ::vim::buffer
func Test_vim_buffer()
  " Test ::vim::buffer {nr}
  e Xfoo1
  let bn1 = bufnr('%')
  let b1 = TclEval('::vim::buffer ' . bn1)
  call assert_equal(b1, TclEval('set ::vim::current(buffer)'))

  new Xfoo2
  let bn2 = bufnr('%')
  let b2 = TclEval('::vim::buffer ' . bn2)
  call assert_equal(b2, TclEval('set ::vim::current(buffer)'))

  call assert_match('Xfoo1$', TclEval(b1 . ' name'))
  call assert_match('Xfoo2$', TclEval(b2 . ' name'))

  " Test ::vim::buffer exists {nr}
  call assert_match('^[1-9]\d*$', TclEval('::vim::buffer exists ' . bn1))
  call assert_match('^[1-9]\d*$', TclEval('::vim::buffer exists ' . bn2))
  call assert_equal('0', TclEval('::vim::buffer exists 54321'))

  " Test ::vim::buffer list
  call assert_equal('2',    TclEval('llength [::vim::buffer list]'))
  call assert_equal(b1.' '.b2, TclEval('::vim::buffer list'))

  %bwipe
endfunc

" Test ::vim::option
func Test_vim_option()
  set cc=3,5

  " Test getting option 'cc'
  call assert_equal('3,5', TclEval('::vim::option cc'))
  call assert_equal('3,5', &cc)

  " Test setting option 'cc' (it returns the old option value)
  call assert_equal('3,5', TclEval('::vim::option cc +4'))
  call assert_equal('+4', &cc)
  call assert_equal('+4', TclEval('::vim::option cc'))

  set cc&
endfunc

" Test ::vim::expr
func Test_vim_expr()
  call assert_equal(string(char2nr('X')),
        \           TclEval('::vim::expr char2nr("X")'))
endfunc

" Test ::vim::command
func Test_vim_command()
  call assert_equal('hello world',
        \           TclEval('::vim::command {echo "hello world"}'))

  call assert_fails('tcl ::vim::command xyz',
        \           'E492: Not an editor command: xyz')

  " With the -quiet option, the error should silently be ignored.
  call assert_equal('', TclEval('::vim::command -quiet xyz'))
endfunc

" Test ::vim::window list
func Test_vim_window_list()
  e Xfoo1
  new Xfoo2
  let w2 = TclEval('set ::vim::current(window)')
  wincmd j
  let w1 = TclEval('set ::vim::current(window)')

  call assert_equal('2', TclEval('llength [::vim::window list]'))
  call assert_equal(w2.' '.w1, TclEval('::vim::window list'))
  %bwipe
endfunc

" Test output messages
func Test_output()
  call assert_fails('tcl puts vimerr "an error"', 'an error')
  tcl puts vimout "a message"
  tcl puts "another message"
  let messages = split(execute('message'), "\n")
  call assert_equal('a message', messages[-2])
  call assert_equal('another message', messages[-1])
endfunc

" Test $win height (get and set window height)
func Test_window_height()
  new

  " Test setting window height
  tcl $::vim::current(window) height 2
  call assert_equal(2, winheight(0))

  " Test getting window height
  call assert_equal('2', TclEval('$::vim::current(window) height'))

  bwipe
endfunc

" Test $win cursor (get and set cursor)
func Test_window_cursor()
  new
  call setline(1, ['line1', 'line2', 'line3', 'line5'])
  tcl set win $::vim::current(window)

  tcl $win cursor 2 4
  call assert_equal([0, 2, 4, 0], getpos('.'))
  call assert_equal('row 2 column 4', TclEval('$win cursor'))

  " When setting ::vim::lbase to 0, line/col are counted from 0
  " instead of 1.
  tcl set ::vim::lbase 0
  call assert_equal([0, 2, 4, 0], getpos('.'))
  call assert_equal('row 1 column 3', TclEval('$win cursor'))
  tcl $win cursor 2 4
  call assert_equal([0, 3, 5, 0], getpos('.'))
  call assert_equal('row 2 column 4', TclEval('$win cursor'))
  tcl set ::vim::lbase 1
  call assert_equal('row 3 column 5', TclEval('$win cursor'))
  call assert_equal([0, 3, 5, 0], getpos('.'))

  " test $win cursor {$var}
  call cursor(2, 3)
  tcl array set here [$win cursor]
  call assert_equal([0, 2, 3, 0], getpos('.'))
  call cursor(3, 1)
  call assert_equal([0, 3, 1, 0], getpos('.'))
  tcl $win cursor here
  call assert_equal([0, 2, 3, 0], getpos('.'))
  call cursor(3, 1)
  call assert_equal([0, 3, 1, 0], getpos('.'))
  tcl $win cursor $here(row) $here(column)
  call assert_equal([0, 2, 3, 0], getpos('.'))

  tcl unset win here
  bwipe!
endfunc

" Test $win buffer
func Test_window_buffer()
  new Xfoo1
  new Xfoo2
  tcl set b2 $::vim::current(buffer)
  tcl set w2 $::vim::current(window)
  wincmd j
  tcl set b1 $::vim::current(buffer)
  tcl set w1 $::vim::current(window)

  call assert_equal(TclEval('set b1'), TclEval('$w1 buffer'))
  call assert_equal(TclEval('set b2'), TclEval('$w2 buffer'))
  call assert_equal(string(bufnr('Xfoo1')), TclEval('[$w1 buffer] number'))
  call assert_equal(string(bufnr('Xfoo2')), TclEval('[$w2 buffer] number'))

  tcl unset b1 b2 w1 w2
  %bwipe
endfunc

" Test $win command
func Test_window_command()
  new Xfoo1
  call setline(1, ['FOObar'])
  new Xfoo2
  call setline(1, ['fooBAR'])
  tcl set w2 $::vim::current(window)
  wincmd j
  tcl set w1 $::vim::current(window)

  tcl $w1 command "norm VU"
  tcl $w2 command "norm Vu"
  b! Xfoo1
  call assert_equal('FOOBAR', getline(1))
  b! Xfoo2
  call assert_equal('foobar', getline(1))

  call assert_fails('tcl $w1 command xyz',
        \           'E492: Not an editor command: xyz')
  tcl $w1 command -quiet xyz

  tcl unset w1 w2
  %bwipe!
endfunc

" Test $win expr
func Test_window_expr()
  new Xfoo1
  new Xfoo2
  tcl set w2 $::vim::current(window)
  wincmd j
  tcl set w1 $::vim::current(window)

  call assert_equal('Xfoo1', TclEval('$w1 expr bufname("%")'))
  call assert_equal('Xfoo2', TclEval('$w2 expr bufname("%")'))

  tcl unset w1 w2
  %bwipe
endfunc

" Test $win option
func Test_window_option()
  new Xfoo1
  new Xfoo2
  tcl set w2 $::vim::current(window)
  wincmd j
  tcl set w1 $::vim::current(window)

  " Test setting window option
  tcl $w1 option syntax java
  tcl $w2 option syntax rust

  call assert_equal('java', &syntax)
  wincmd k
  call assert_equal('rust', &syntax)

  " Test getting window option
  call assert_equal('java', TclEval('$w1 option syntax'))
  call assert_equal('rust', TclEval('$w2 option syntax'))

  tcl unset w1 w2
  %bwipe
endfunc

" Test $buf name
func Test_buffer_name()
  " Test buffer name with a named buffer
  new Xfoo
  call assert_equal(expand('%:p'), TclEval('$::vim::current(buffer) name'))
  bwipe

  " Test buffer name with an unnamed buffer
  new
  call assert_equal('', TclEval('$::vim::current(buffer) name'))
  bwipe
endfunc

" Test $buf number
func Test_buffer_number()
  new
  call assert_equal(string(bufnr('%')), TclEval('$::vim::current(buffer) number'))
  new
  call assert_equal(string(bufnr('%')), TclEval('$::vim::current(buffer) number'))
  bwipe %
endfunc

" Test $buf count and $buf last
func Test_buffer_count()
  new
  call setline(1, ['one', 'two', 'three'])
  call assert_equal('3', TclEval('$::vim::current(buffer) count'))

  " "$buf count" and "$buf last" do the same thing. Why 2 commands?
  call assert_equal('3', TclEval('$::vim::current(buffer) last'))
  bwipe!
endfunc

" Test $buf delete (delete line(s) in buffer)
func Test_buffer_delete()
  new
  call setline(1, ['one', 'two', 'three', 'four', 'five', 'six', 'seven', 'eight'])
  tcl $::vim::current(buffer) delete 4 6
  tcl $::vim::current(buffer) delete 2
  call assert_equal(['one', 'three', 'seven', 'eight'], getline(1, '$'))

  call assert_fails('tcl $::vim::current(buffer) delete -1', 'line number out of range')
  call assert_fails('tcl $::vim::current(buffer) delete  0', 'line number out of range')
  call assert_fails('tcl $::vim::current(buffer) delete  5', 'line number out of range')

  bwipe!
endfunc

" Test $buf insert (insert line(s) in buffer)
func Test_buffer_insert()
  new
  tcl set buf $::vim::current(buffer)
  tcl $buf insert 1 "first"
  tcl $buf insert 2 "second"
  tcl $buf insert 2 "third"
  tcl $buf insert 4 "fourth"
  tcl $buf insert 1 "fifth"
  call assert_equal(['fifth', 'first', 'third', 'second', 'fourth', ''], getline(1, '$'))

  call assert_fails('tcl $buf insert -1 "x"', 'line number out of range')
  call assert_fails('tcl $buf insert  0 "x"', 'line number out of range')
  call assert_fails('tcl $buf insert  7 "x"', 'line number out of range')

  tcl unset buf
  bwipe!
endfunc

" Test $buf append (append line in buffer)
func Test_buffer_append()
  new
  tcl set buf $::vim::current(buffer)
  tcl $buf append 1 "first"
  tcl $buf append 2 "second"
  tcl $buf append 2 "third"
  tcl $buf append 4 "fourth"
  tcl $buf append 1 "fifth"
  call assert_equal(['', 'fifth', 'first', 'third', 'second', 'fourth'], getline(1, '$'))

  call assert_fails('tcl $buf append -1 "x"', 'line number out of range')
  call assert_fails('tcl $buf append  0 "x"', 'line number out of range')
  call assert_fails('tcl $buf append  7 "x"', 'line number out of range')

  tcl unset buf
  bwipe!
endfunc

" Test $buf set (replacing line(s) in a buffer)
func Test_buffer_set()
  new
  call setline(1, ['line1', 'line2', 'line3', 'line4', 'line5'])
  tcl $::vim::current(buffer) set 2 a
  call assert_equal(['line1', 'a', 'line3', 'line4', 'line5'], getline(1, '$'))
  tcl $::vim::current(buffer) set 3 4 b
  call assert_equal(['line1', 'a', 'b', 'line5'], getline(1, '$'))

  call assert_fails('tcl $::vim::current(buffer) set 0 "x"', 'line number out of range')
  call assert_fails('tcl $::vim::current(buffer) set 5 "x"', 'line number out of range')
  bwipe!
endfunc

" Test $buf get (get line(s) from buffer)
func Test_buffer_get()
  new
  call setline(1, ['first line', 'two', 'three', 'last line'])
  tcl set buf $::vim::current(buffer)

  call assert_equal('first line', TclEval('$buf get top'))
  call assert_equal('first line', TclEval('$buf get begin'))
  call assert_equal('last line',  TclEval('$buf get bottom'))
  call assert_equal('last line',  TclEval('$buf get last'))

  call assert_equal('first line', TclEval('$buf get 1'))
  call assert_equal('two',        TclEval('$buf get 2'))
  call assert_equal('three',      TclEval('$buf get 3'))
  call assert_equal('last line',  TclEval('$buf get 4'))

  call assert_equal('two three',         TclEval('$buf get 2 3'))
  call assert_equal('two three',         TclEval('$buf get 3 2'))
  call assert_equal('three {last line}', TclEval('$buf get 3 last'))

  call assert_fails('tcl $buf get -1',   'line number out of range')
  call assert_fails('tcl $buf get  0',   'line number out of range')
  call assert_fails('tcl $buf get  5',   'line number out of range')
  call assert_fails('tcl $buf get  0 1', 'line number out of range')

  tcl unset buf
  bwipe!
endfunc

" Test $buf mark (get position of a mark)
func Test_buffer_mark()
  new
  call setline(1, ['one', 'two', 'three', 'four'])
  /three
  norm! ma
  norm! jllmB

  call assert_equal('row 3 column 1', TclEval('$::vim::current(buffer) mark "a"'))
  call assert_equal('row 4 column 3', TclEval('$::vim::current(buffer) mark "B"'))

  delmarks aB
  bwipe!
endfunc

" Test $buf list
func Test_buffer_list()
  e Xfoo
  call setline(1, ['foobar'])
  new Xbar
  call setline(1, ['barfoo'])

  call assert_equal('2', TclEval('llength [::vim::buffer list]'))

  tcl <<EOF
    proc eachbuf { cmd } {
      foreach b [::vim::buffer list] { $b command $cmd }
    }
EOF
  tcl eachbuf %s/foo/FOO/g
  b! Xfoo
  call assert_equal(['FOObar'], getline(1, '$'))
  b! Xbar
  call assert_equal(['barFOO'], getline(1, '$'))

  tcl rename eachbuf ""
  %bwipe!
endfunc

" Test :tclfile
func Test_tclfile()
  call delete('Xtcl_file')
  call writefile(['set pi [format "%.2f" [expr acos(-1.0)]]'], 'Xtcl_file')
  call setfperm('Xtcl_file', 'r-xr-xr-x')

  tclfile Xtcl_file
  call assert_equal('3.14', TclEval('set pi'))

  tcl unset pi
  call delete('Xlua_file')
endfunc

" Test :tclfile with syntax error in tcl script
func Test_tclfile_error()
  call delete('Xtcl_file')
  call writefile(['xyz'], 'Xtcl_file')
  call setfperm('Xtcl_file', 'r-xr-xr-x')

  call assert_fails('tclfile Xtcl_file', 'invalid command name "xyz"')

  call delete('Xlua_file')
endfunc
