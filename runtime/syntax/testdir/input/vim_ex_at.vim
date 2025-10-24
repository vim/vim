" Vim :@ command


@0
@1
@2
@3
@4
@5
@6
@7
@8
@9

@a
@k
@z

@"
@.
@=
@*
@+

@:

" repeats
@
@@

@a | echo "..."
@a " comment


def Vim9Context()
  :@0
  :@1
  :@2
  :@3
  :@4
  :@5
  :@6
  :@7
  :@8
  :@9

  :@a
  :@k
  :@z

  :@"
  :@.
  :@=
  :@*
  :@+

  :@:

  # repeats
  :@
  :@@

  :@a | echo "..."
  :@a # comment
enddef

