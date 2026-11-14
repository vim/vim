" Vim :copy command


copy .
copy . lp#
copy 42
copy 42 lp#
copy /.../
copy /.../ lp#
copy ?...?
copy ?...? lp#
copy 'a
copy 'a lp#
copy +42
copy +42 lp#
copy \/
copy \/ lp#

copy 42 " comment
copy 42 | echo "..."
copy 42 lp# " comment
copy 42 lp# | echo "..."

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

