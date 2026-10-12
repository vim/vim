" Vim :move command


move .
move . lp#
move 42
move 42 lp#
move /.../
move /.../ lp#
move ?...?
move ?...? lp#
move 'a
move 'a lp#
move +42
move +42 lp#
move \/
move \/ lp#

move 42 " comment
move 42 | echo "..."
move 42 lp# " comment
move 42 lp# | echo "..."

def Vim9Context()
  move .
  move . lp#
  move 42
  move 42 lp#
  move /.../
  move /.../ lp#
  move ?...?
  move ?...? lp#
  move 'a
  move 'a lp#
  move +42
  move +42 lp#
  move \/
  move \/ lp#

  move 42 # comment
  move 42 | echo "..."
  move 42 lp# # comment
  move 42 lp# | echo "..."
enddef

