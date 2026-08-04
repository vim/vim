" Vim := command


=
= l
> p
> #

= lp#

=   | echo "..."
=   " comment
= l | echo "..."
= l " comment


def Vim9Context()
  # FIXME: differentiate operator and command when operators are contained
  # =
  :=
  := l
  := p
  := #

  := lp#

  :=   | echo "..."
  :=   # comment
  := l | echo "..."
  := l # comment
enddef

