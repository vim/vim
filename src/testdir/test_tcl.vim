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
  call assert_fails('tcl ::vim::beep x', 'wrong # args: should be "::vim::beep"')
endfunc

" Test ::vim::buffer
func Test_vim_buffer()
  " Test ::vim::buffer {nr}
  e Xfoo1
  call setline(1, ['foobar'])
  let bn1 = bufnr('%')
  let b1 = TclEval('::vim::buffer ' . bn1)
  call assert_equal(b1, TclEval('set ::vim::current(buffer)'))

  new Xfoo2
  call setline(1, ['barfoo'])
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
  tcl <<EOF
    proc eachbuf { cmd } {
      foreach b [::vim::buffer list] { $b command $cmd }
    }
EOF
  tcl eachbuf %s/foo/FOO/g
  b! Xfoo1
  call assert_equal(['FOObar'], getline(1, '$'))
  b! Xfoo2
  call assert_equal(['barFOO'], getline(1, '$'))

  call assert_fails('tcl ::vim::buffer',
        \           'wrong # args: should be "::vim::buffer option"')
  call assert_fails('tcl ::vim::buffer ' . bn1 . ' x',
        \           'wrong # args: should be "::vim::buffer bufNumber"')
  call assert_fails('tcl ::vim::buffer 4321', 'invalid buffer number')
  call assert_fails('tcl ::vim::buffer x',
        \           'bad option "x": must be exists or list')
  call assert_fails('tcl ::vim::buffer exists',
        \           'wrong # args: should be "::vim::buffer exists bufNumber"')
  call assert_fails('tcl ::vim::buffer exists x',
        \           'expected integer but got "x"')
  call assert_fails('tcl ::vim::buffer list x',
        \           'wrong # args: should be "::vim::buffer list "')

  tcl rename eachbuf ""
  %bwipe!
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

  call assert_fails('tcl ::vim::option xxx', 'unknown vimOption')
  call assert_fails('tcl ::vim::option',
        \           'wrong # args: should be "::vim::option vimOption ?value?"')

  set cc&
endfunc

" Test ::vim::expr
func Test_vim_expr()
  call assert_equal(string(char2nr('X')),
        \           TclEval('::vim::expr char2nr("X")'))

  call assert_fails('tcl ::vim::expr x y',
        \           'wrong # args: should be "::vim::expr vimExpr"')
endfunc

" Test ::vim::command
func Test_vim_command()
  call assert_equal('hello world',
        \           TclEval('::vim::command {echo "hello world"}'))

  " With the -quiet option, the error should silently be ignored.
  call assert_equal('', TclEval('::vim::command -quiet xyz'))

  call assert_fails('tcl ::vim::command',
       \            'wrong # args: should be "::vim::command ?-quiet? exCommand"')
  call assert_fails('tcl ::vim::command -foo xyz', 'unknown flag: -foo')
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

  call assert_fails('tcl ::vim::window x', 'unknown option')
  call assert_fails('tcl ::vim::window list x',
        \           'wrong # args: should be "::vim::window option"')

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

  call assert_fails('tcl puts',
        \           'wrong # args: should be "puts ?-nonewline? ?channelId? string"')
endfunc

" Test $win height (get and set window height)
func Test_window_height()
  new

  " Test setting window height
  tcl $::vim::current(window) height 2
  call assert_equal(2, winheight(0))

  " Test getting window height
  call assert_equal('2', TclEval('$::vim::current(window) height'))

  call assert_fails('tcl $::vim::current(window) height 2 2', 'wrong # args:')
  call assert_fails('tcl $::vim::current(window) height x',
        \ 'expected integer but got "x"')
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

  call assert_fails('tcl $win cursor 1 1 1', 'wrong # args:')

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

  call assert_fails('tcl $w1 buffer x', 'wrong # args:')

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

  call assert_fails('tcl $w1 expr', 'wrong # args:')
  call assert_fails('tcl $w1 expr x x', 'wrong # args:')

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

" Test $win delcmd {cmd}
func Test_window_delcmd()
  new
  tcl $::vim::current(window) delcmd [list set msg "window deleted"]
  call assert_fails('tcl set msg', "can't read \"msg\": no such variable")
  q
  call assert_equal('window deleted', TclEval('set msg'))

  call assert_fails('tcl $::vim::current(window) delcmd', 'wrong # args')

  tcl unset msg
  bwipe
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

  call assert_fails('tcl $::vim::current(buffer) name x', 'wrong # args:')

  bwipe
endfunc

" Test $buf number
func Test_buffer_number()
  new
  call assert_equal(string(bufnr('%')), TclEval('$::vim::current(buffer) number'))
  new
  call assert_equal(string(bufnr('%')), TclEval('$::vim::current(buffer) number'))

  call assert_fails('tcl $::vim::current(buffer) number x', 'wrong # args:')

  %bwipe
endfunc

" Test $buf count and $buf last
func Test_buffer_count()
  new
  call setline(1, ['one', 'two', 'three'])
  call assert_equal('3', TclEval('$::vim::current(buffer) count'))
  call assert_equal('3', TclEval('$::vim::current(buffer) last'))

  " Check that $buf count and $buf last differ when ::vim::lbase is 0.
  tcl set ::vim::lbase 0
  call assert_equal('3', TclEval('$::vim::current(buffer) count'))
  call assert_equal('2', TclEval('$::vim::current(buffer) last'))

  call assert_fails('tcl $::vim::current(buffer) count x', 'wrong # args:')
  call assert_fails('tcl $::vim::current(buffer) last x',  'wrong # args:')

  tcl set ::vim::lbase 1
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

  call assert_fails('tcl $::vim::current(buffer) delete', 'wrong # args:')
  call assert_fails('tcl $::vim::current(buffer) delete 1 2 3', 'wrong # args:')

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

  call assert_fails('tcl $buf append', 'wrong # args:')
  call assert_fails('tcl $buf append 1 x x', 'wrong # args:')

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
  tcl $::vim::current(buffer) set 4 3 c
  call assert_equal(['line1', 'a', 'c'], getline(1, '$'))

  call assert_fails('tcl $::vim::current(buffer) set 0 "x"', 'line number out of range')
  call assert_fails('tcl $::vim::current(buffer) set 5 "x"', 'line number out of range')

  call assert_fails('tcl $::vim::current(buffer) set', 'wrong # args:')
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

  call assert_fails('tcl $::vim::current(buffer) get x', 'expected integer but got "x"')
  call assert_fails('tcl $::vim::current(buffer) get 1 1 1', 'wrong # args:')

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

  call assert_equal('row 3 column 1', TclEval('$::vim::current(buffer) mark a'))
  call assert_equal('row 4 column 3', TclEval('$::vim::current(buffer) mark B'))

  call assert_fails('tcl $::vim::current(buffer) mark /', 'invalid mark name')
  call assert_fails('tcl $::vim::current(buffer) mark z', 'mark not set')
  call assert_fails('tcl $::vim::current(buffer) mark', 'wrong # args:')

  delmarks aB
  bwipe!
endfunc

" Test $buf option (test and set option in context of a buffer)
func Test_buffer_option()
  new Xfoo1
  tcl set b1 $::vim::current(buffer)
  new Xfoo2
  tcl set b2 $::vim::current(buffer)

  tcl $b1 option foldcolumn 2
  tcl $b2 option foldcolumn 3

  call assert_equal(3, &foldcolumn)
  wincmd j
  call assert_equal(2, &foldcolumn)

  call assert_equal('2', TclEval('$b1 option foldcolumn'))
  call assert_equal('3', TclEval('$b2 option foldcolumn'))

  call assert_fails('tcl $::vim::current(buffer) option', 'wrong # args:')

  set foldcolumn&
  tcl unset b1 b2
  %bwipe
endfunc

" Test $buf expr (evaluate vim expression)
func Test_buffer_expr()
  new Xfoo1
  norm ifoo1
  tcl set b1 $::vim::current(buffer)

  new Xfoo2
  norm ifoo2
  tcl set b2 $::vim::current(buffer)

  call assert_equal('foo1', TclEval('$b1 expr getline(1)'))
  call assert_equal('foo2', TclEval('$b2 expr getline(1)'))

  call assert_fails('tcl expr', 'wrong # args:')

  tcl unset b1 b2
  %bwipe!
endfunc

" Test $buf delcmd {cmd} (command executed when buffer is deleted)
func Test_buffer_delcmd()
  new Xfoo
  split
  tcl $::vim::current(buffer) delcmd [list set msg "buffer deleted"]
  q
  call assert_fails('tcl set msg', "can't read \"msg\": no such variable")
  q
  call assert_equal('buffer deleted', TclEval('set msg'))

  call assert_fails('tcl $::vim::current(window) delcmd', 'wrong # args')
  call assert_fails('tcl $::vim::current(window) delcmd x x', 'wrong # args')

  tcl unset msg
  %bwipe
endfunc

func Test_vim_current()
  " Only test errors as ::vim::current(...) is already indirectly
  " tested by many other tests.
  call assert_fails('tcl $::vim::current(buffer)', 'wrong # args:')
  call assert_fails('tcl $::vim::current(window)', 'wrong # args:')
endfunc

" Test $buf windows (windows list of a buffer)
func Test_buffer_windows()
  new Xfoo
  split
  new Xbar
  split
  vsplit

  tcl set bar_wl [$::vim::current(buffer) windows]
  2wincmd j
  tcl set foo_wl [$::vim::current(buffer) windows]

  call assert_equal('2', TclEval('llength $foo_wl'))
  call assert_equal('3', TclEval('llength $bar_wl'))

  call assert_fails('tcl $::vim::current(buffer) windows x', 'wrong # args:')

  tcl unset bar_wl foo_wl
  %bwipe
endfunc

" Test :tclfile
func Test_tclfile()
  call delete('Xtcl_file')
  call writefile(['set pi [format "%.2f" [expr acos(-1.0)]]'], 'Xtcl_file')
  call setfperm('Xtcl_file', 'r-xr-xr-x')

  tclfile Xtcl_file
  call assert_equal('3.14', TclEval('set pi'))

  tcl unset pi
  call delete('Xtcl_file')
endfunc

" Test :tclfile with syntax error in tcl script
func Test_tclfile_error()
  call delete('Xtcl_file')
  call writefile(['xyz'], 'Xtcl_file')
  call setfperm('Xtcl_file', 'r-xr-xr-x')

  call assert_fails('tclfile Xtcl_file', 'invalid command name "xyz"')

  call delete('Xtcl_file')
endfunc

" Test exiting current Tcl interprepter and re-creating one.
func Test_tcl_exit()
  tcl set foo "foo"
  call assert_fails('tcl exit 3', 'E572: exit code 3')

  " The Tcl interpreter should have been deleted and a new one
  " is re-created with the next :tcl command.
  call assert_fails('tcl set foo', "can't read \"foo\": no such variable")
  tcl set bar "bar"
  call assert_equal('bar', TclEval('set bar'))

  tcl unset bar
endfunc
