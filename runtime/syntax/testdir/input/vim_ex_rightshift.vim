" Vim :> command

>
>>
>>>

> >
> > >

> 42
>> 42
>>> 42

> l
> p
> #
>> l
>> p
>> #

> lp#
>> lp#

> 42 l
> 42 p
> 42 #
>> 42 l
>> 42 p
>> 42 #

> 42 lp#
>> 42 lp#

>    | echo "..."
>    " comment
> >  | echo "..."
> >  " comment
> 42 | echo "..."
> 42 " comment
> l  | echo "..."
> l  " comment


def Vim9Context()
  # FIXME: differentiate operator and command when operators are contained
  # >
  :>
  :> >
  :> >>

  :> >
  :> > >

  :> 42
  :> > 42
  :> >> 42

  :> l
  :> p
  :> #
  :> > l
  :> > p
  :> > #

  :> lp#
  :> > lp#

  :> 42 l
  :> 42 p
  :> 42 #
  :> > 42 l
  :> > 42 p
  :> > 42 #

  :> 42 lp#
  :>> 42 lp#

  :>    | echo "..."
  :>    # comment
  :> >  | echo "..."
  :> >  # comment
  :> 42 | echo "..."
  :> 42 # comment
  :> l  | echo "..."
  :> l  # comment
enddef
