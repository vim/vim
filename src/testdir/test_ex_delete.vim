" Test :delete

let s:LINES = ["one", "\ttwo", "three"]

let s:NONE        = ''
let s:PRINT       = '        two'
let s:LIST        = '^Itwo$'
let s:NUMBER      = '  1         two'
let s:NUMBER_LIST = '  1 ^Itwo$'

func Test_ex_delete()
  new
  call setline(1, s:LINES)
  2
  " :dl is :delete with the "l" flag, not :dlist
  .dl
  call assert_equal(['one', 'three'], getline(1, 2))
  %delete _

  " :delete # used to clobber "0
  call setreg('#', '')
  call setreg('0', '')
  call setline(1, s:LINES)
  " msg
  call assert_equal([s:NUMBER], split(execute('1delete #'), "\n"))
  " was line 1 deleted?
  call assert_equal(["\ttwo", "three"], getline(1, '$'))
  call assert_equal('', getreg('#'))
  call assert_equal('', getreg('0'))

  bw!
endfunc

func Test_ex_delete_flags()
  " [cmd, msg]

  " No whitespace
  let tests = [
        "\ no flags (none)
        \ ['d',             s:NONE],
        \ ['de',            s:NONE],
        \ ['del',           s:NONE],
        \ ['dele',          s:NONE],
        \ ['delet',         s:NONE],
        \ ['delete',        s:NONE],
        "\ l flag (list)
        \ ['dl',            s:LIST],
        \ ['dell',          s:LIST],
        \ ['delel',         s:LIST],
        \ ['deletl',        s:LIST],
        \ ['deletel',       s:LIST],
        "\ p flag (print)
        \ ['dp',            s:PRINT],
        \ ['dep',           s:PRINT],
        \ ['delp',          s:PRINT],
        \ ['delep',         s:PRINT],
        \ ['deletp',        s:PRINT],
        \ ['deletep',       s:PRINT],
        "\ # flag (number)
        \ ['d#',            s:NUMBER],
        \ ['de#',           s:NUMBER],
        \ ['del#',          s:NUMBER],
        \ ['dele#',         s:NUMBER],
        \ ['delet#',        s:NUMBER],
        \ ['delete#',       s:NUMBER],
        "\ flags in any order
        \ ['deletelp',      s:LIST],
        \ ['deletepl',      s:LIST],
        \ ['deletel#',      s:NUMBER_LIST],
        \ ['delete#l',      s:NUMBER_LIST],
        \ ['deletep#',      s:NUMBER],
        \ ['delete#p',      s:NUMBER],
        \ ['deletelp#',     s:NUMBER_LIST],
        \ ['deletel#p',     s:NUMBER_LIST],
        \ ['deletepl#',     s:NUMBER_LIST],
        \ ['deletep#l',     s:NUMBER_LIST],
        \ ['delete#lp',     s:NUMBER_LIST],
        \ ['delete#pl',     s:NUMBER_LIST],
        "\ duplicate flags
        \ ['deletell',      s:LIST],
        \ ['deletepp',      s:PRINT],
        \ ['delete##',      s:NUMBER],
        \ ['deletellpp##',  s:NUMBER_LIST],
        \ ['deletelp#lp#',  s:NUMBER_LIST],
        \ ['delete#lp#pl#', s:NUMBER_LIST],
        \ ]
  " after {register}
  let tests += [
        "\ l flag (list)
        \ ['d _ l',            s:LIST],
        \ ['del _ l',          s:LIST],
        \ ['dele _ l',         s:LIST],
        \ ['delet _ l',        s:LIST],
        \ ['delete _ l',       s:LIST],
        "\ p flag (print)
        \ ['d _ p',            s:PRINT],
        \ ['de _ p',           s:PRINT],
        \ ['del _ p',          s:PRINT],
        \ ['dele _ p',         s:PRINT],
        \ ['delet _ p',        s:PRINT],
        \ ['delete _ p',       s:PRINT],
        "\ # flag (number)
        \ ['d _ #',            s:NUMBER],
        \ ['de _ #',           s:NUMBER],
        \ ['del _ #',          s:NUMBER],
        \ ['dele _ #',         s:NUMBER],
        \ ['delet _ #',        s:NUMBER],
        \ ['delete _ #',       s:NUMBER],
        "\ flags in any order
        \ ['delete _ lp',      s:LIST],
        \ ['delete _ pl',      s:LIST],
        \ ['delete _ l#',      s:NUMBER_LIST],
        \ ['delete _ #l',      s:NUMBER_LIST],
        \ ['delete _ p#',      s:NUMBER],
        \ ['delete _ #p',      s:NUMBER],
        \ ['delete _ lp#',     s:NUMBER_LIST],
        \ ['delete _ l#p',     s:NUMBER_LIST],
        \ ['delete _ pl#',     s:NUMBER_LIST],
        \ ['delete _ p#l',     s:NUMBER_LIST],
        \ ['delete _ #lp',     s:NUMBER_LIST],
        \ ['delete _ #pl',     s:NUMBER_LIST],
        "\ duplicate flags
        \ ['delete _ ll',      s:LIST],
        \ ['delete _ pp',      s:PRINT],
        \ ['delete _ ##',      s:NUMBER],
        \ ['delete _ llpp##',  s:NUMBER_LIST],
        \ ['delete _ lp#lp#',  s:NUMBER_LIST],
        \ ['delete _ #lp#pl#', s:NUMBER_LIST],
        \ ]
  " after {count}
  let tests += [
        "\ l flag (list)
        \ ['d 1 l',            s:LIST],
        \ ['del 1 l',          s:LIST],
        \ ['dele 1 l',         s:LIST],
        \ ['delet 1 l',        s:LIST],
        \ ['delete 1 l',       s:LIST],
        "\ p flag (print)
        \ ['d 1 p',            s:PRINT],
        \ ['de 1 p',           s:PRINT],
        \ ['del 1 p',          s:PRINT],
        \ ['dele 1 p',         s:PRINT],
        \ ['delet 1 p',        s:PRINT],
        \ ['delete 1 p',       s:PRINT],
        "\ # flag (number)
        \ ['d 1 #',            s:NUMBER],
        \ ['de 1 #',           s:NUMBER],
        \ ['del 1 #',          s:NUMBER],
        \ ['dele 1 #',         s:NUMBER],
        \ ['delet 1 #',        s:NUMBER],
        \ ['delete 1 #',       s:NUMBER],
        "\ flags in any order
        \ ['delete 1 lp',      s:LIST],
        \ ['delete 1 pl',      s:LIST],
        \ ['delete 1 l#',      s:NUMBER_LIST],
        \ ['delete 1 #l',      s:NUMBER_LIST],
        \ ['delete 1 p#',      s:NUMBER],
        \ ['delete 1 #p',      s:NUMBER],
        \ ['delete 1 lp#',     s:NUMBER_LIST],
        \ ['delete 1 l#p',     s:NUMBER_LIST],
        \ ['delete 1 pl#',     s:NUMBER_LIST],
        \ ['delete 1 p#l',     s:NUMBER_LIST],
        \ ['delete 1 #lp',     s:NUMBER_LIST],
        \ ['delete 1 #pl',     s:NUMBER_LIST],
        "\ duplicate flags
        \ ['delete 1 ll',      s:LIST],
        \ ['delete 1 pp',      s:PRINT],
        \ ['delete 1 ##',      s:NUMBER],
        \ ['delete 1 llpp##',  s:NUMBER_LIST],
        \ ['delete 1 lp#lp#',  s:NUMBER_LIST],
        \ ['delete 1 #lp#pl#', s:NUMBER_LIST],
        \ ]
  " before and after {register}
  let tests += [
        "\ flags in any order
        \ ['deletel _ p',      s:LIST],
        \ ['deletep _ l',      s:LIST],
        \ ['deletel _ #',      s:NUMBER_LIST],
        \ ['delete# _ l',      s:NUMBER_LIST],
        \ ['deletep _ #',      s:NUMBER],
        \ ['delete# _ p',      s:NUMBER],
        \ ['deletel _ p#',     s:NUMBER_LIST],
        \ ['deletel _ #p',     s:NUMBER_LIST],
        \ ['deletep _ l#',     s:NUMBER_LIST],
        \ ['deletep _ #l',     s:NUMBER_LIST],
        \ ['delete# _ lp',     s:NUMBER_LIST],
        \ ['delete# _ pl',     s:NUMBER_LIST],
        "\ duplicate flags
        \ ['deletel _ l',      s:LIST],
        \ ['deletep _ p',      s:PRINT],
        \ ['delete# _ #',      s:NUMBER],
        \ ['deletellp _ p##',  s:NUMBER_LIST],
        \ ['delete#lp _ #lp#', s:NUMBER_LIST],
        \ ]
  " before and after {count}
  let tests += [
        "\ flags in any order
        \ ['deletel 1 p',      s:LIST],
        \ ['deletep 1 l',      s:LIST],
        \ ['deletel 1 #',      s:NUMBER_LIST],
        \ ['delete# 1 l',      s:NUMBER_LIST],
        \ ['deletep 1 #',      s:NUMBER],
        \ ['delete# 1 p',      s:NUMBER],
        \ ['deletel 1 p#',     s:NUMBER_LIST],
        \ ['deletel 1 #p',     s:NUMBER_LIST],
        \ ['deletep 1 l#',     s:NUMBER_LIST],
        \ ['deletep 1 #l',     s:NUMBER_LIST],
        \ ['delete# 1 lp',     s:NUMBER_LIST],
        \ ['delete# 1 pl',     s:NUMBER_LIST],
        "\ duplicate flags
        \ ['deletel 1 l',      s:LIST],
        \ ['deletep 1 p',      s:PRINT],
        \ ['delete# 1 #',      s:NUMBER],
        \ ['deletellp 1 p##',  s:NUMBER_LIST],
        \ ['delete#lp 1 #lp#', s:NUMBER_LIST],
        \ ]
  " minimal whitespace, leading and trailing flags
  let tests += [
        \ ['deletel _p#',      s:NUMBER_LIST],
        \ ['deletel1p#',       s:NUMBER_LIST],
        \ ['deletel _1p#',     s:NUMBER_LIST],
        \]
  " maximum whitespace, leading and trailing flags
  let tests += [
        \ ['deletel _ p #',    s:NUMBER_LIST],
        \ ['deletel 1 p #',    s:NUMBER_LIST],
        \ ['deletel _ 1 p #',  s:NUMBER_LIST],
        \ ]

  new
  for [cmd, msg] in tests
    call setline(1, s:LINES)
    call cursor(1, 1)
    call assert_equal(split(msg, "\n"), split(execute(cmd), "\n"), cmd)
    " was line 1 deleted?
    call assert_equal(s:LINES[1:], getline(1, '$'), cmd)
  endfor
  bw!
endfunc

func Test_ex_delete_registers()
  " [cmd, reg, msg]
  let tests = [
        "\ flag-letter used as a register
        \ ['d l',     'l', s:NONE],
        \ ['d p',     'p', s:NONE],
          "\ "# not writable, is flag here
        \ ['d #',     '#', s:NUMBER],
        "\ attached flag, then register
        \ ['dl l',    'l', s:LIST],
        \ ['dp l',    'l', s:PRINT],
        \ ['d# l',    'l', s:NUMBER],
        \ ['dl p',    'p', s:LIST],
        \ ['dp p',    'p', s:PRINT],
        \ ['d# p',    'p', s:NUMBER],
          "\ "# not writable, is flag here
        \ ['dl #',    '#', s:NUMBER_LIST],
        \ ['dp #',    '#', s:NUMBER],
        \ ['d# #',    '#', s:NUMBER],
        "\ register, then packed tail flag
        \ ['d ll',    'l', s:LIST],
        \ ['d lp',    'l', s:PRINT],
        \ ['d l#',    'l', s:NUMBER],
        \ ['d pl',    'p', s:LIST],
        \ ['d pp',    'p', s:PRINT],
        \ ['d p#',    'p', s:NUMBER],
          "\ "# not writable, is flag here
        \ ['d #l',    '#', s:NUMBER_LIST],
        \ ['d #p',    '#', s:NUMBER],
        \ ['d ##',    '#', s:NUMBER],
        "\ register, then packed tail flag
        \ ['d llp#',  'l', s:NUMBER_LIST],
        "\ register, then spaced tail flag
        \ ['d l l',   'l', s:LIST],
        \ ['d l p',   'l', s:PRINT],
        \ ['d l #',   'l', s:NUMBER],
        \ ['d p l',   'p', s:LIST],
        \ ['d p p',   'p', s:PRINT],
        \ ['d p #',   'p', s:NUMBER],
        \ ['d # l',   '#', s:NUMBER_LIST],
        \ ['d # p',   '#', s:NUMBER],
        \ ['d # #',   '#', s:NUMBER],
        "\ register, then spaced tail flags
        \ ['d l lp#', 'l', s:NUMBER_LIST],
        \ ]
  new
  for [cmd, reg, msg] in tests
    call setline(1, s:LINES)
    call setreg(reg, '')
    call assert_equal(split(msg, "\n"), split(execute(cmd), "\n"), cmd)
    " was line 1 deleted?
    call assert_equal(s:LINES[1:], getline(1, '$'), cmd)
    " "# isn't writable with :delete, always a flag (POSIX)
    call assert_equal(reg == '#' ? '' : "one\n", getreg(reg), cmd)
  endfor
  bw!
endfunc

func Test_ex_delete_count()
  new

  " [cmd, reg, msg]
  " register, count 1, optional flag
  let tests = [
        \ ['d l1',  'l', s:NONE],
        \ ['d p1',  'p', s:NONE],
        \ ['d l1l', 'l', s:LIST],
        \ ['d p1l', 'p', s:LIST],
        \ ['d l1#', 'l', s:NUMBER],
        \ ['d p1#', 'p', s:NUMBER],
        \ ]
  for [cmd, reg, msg] in tests
    call setline(1, s:LINES)
    call cursor(1, 1)
    call setreg(reg, '')
    call assert_equal(split(msg, "\n"), split(execute(cmd), "\n"), cmd)
    call assert_equal(s:LINES[1:], getline(1, '$'), cmd)
    call assert_equal("one\n", getreg(reg), cmd)
  endfor

  call setline(1, s:LINES)
  call setreg('#', '')
  " "# not writable, not flag here
  call assert_fails('d #1', 'E488:')
  call assert_equal('', getreg('#'))

  call setline(1, s:LINES)
  call setreg('#', '')
  " "# not writable, not flag here
  call assert_fails('d #1l', 'E488:')
  call assert_equal('', getreg('#'))

  call setline(1, s:LINES)
  call setreg('#', '')
  " "# not writable, not flag here
  call assert_fails('d #1p', 'E488:')
  call assert_equal('', getreg('#'))

  call setline(1, s:LINES)
  call setreg('#', '')
  " "# not writable, not flag here
  call assert_fails('d #1#', 'E488:')
  call assert_equal('', getreg('#'))

  " multiline count
  call setline(1, s:LINES)
  call assert_equal(['three$'], split(execute('d 2 l'), "\n"))
  call setline(1, s:LINES)
  call assert_equal(['three'], split(execute('d 2 p'), "\n"))
  call setline(1, s:LINES)
  call assert_equal(['  1 three'], split(execute('d 2 #'), "\n"))
  call setline(1, s:LINES)
  call assert_equal(['  1 three$'], split(execute('d 2 lp#'), "\n"))

  bw!
endfunc

" vim: shiftwidth=2 sts=2 expandtab
