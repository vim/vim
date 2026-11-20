" Vim :substitute command


substitute/foo
substitute/foo/bar
substitute/foo/bar/

substitute/foo/bar/l
substitute/foo/bar/p
substitute/foo/bar/lp#

substitute/foo/bar/ l
substitute/foo/bar/ p
substitute/foo/bar/ lp#

substitute/foo/bar/42l
substitute/foo/bar/42p
substitute/foo/bar/42lp#

substitute/foo/bar/ 42l
substitute/foo/bar/ 42p
substitute/foo/bar/ 42lp#

substitute/foo/bar/42 l
substitute/foo/bar/42 p
substitute/foo/bar/42 lp#

substitute/foo/bar/ 42 l
substitute/foo/bar/ 42 p
substitute/foo/bar/ 42 lp#

substitute/foo/bar/&
substitute/foo/bar/cegiInp#lr

snomagic/foo
snomagic/foo/bar
snomagic/foo/bar/

snomagic/foo/bar/&
snomagic/foo/bar/cegiInp#lr

smagic/foo
smagic/foo/bar
smagic/foo/bar/

smagic/foo/bar/&
smagic/foo/bar/cegiInp#lr

:substitute/foo/bar/&
:substitute/foo/bar/cegiInp#lr

:snomagic/foo/bar/&
:snomagic/foo/bar/cegiInp#lr

:smagic/foo/bar/&
:smagic/foo/bar/cegiInp#lr

call Foo() | substitute/foo/bar/&
call Foo() | substitute/foo/bar/cegiInp#lr

call Foo() | snomagic/foo/bar/&
call Foo() | snomagic/foo/bar/cegiInp#lr

call Foo() | smagic/foo/bar/&
call Foo() | smagic/foo/bar/cegiInp#lr

let foo = str->substitute(str, pat, sub, flags)

function Foo()
  substitute/foo
  substitute/foo/bar
  substitute/foo/bar/
  let bar = str->substitute(str, pat, sub, flags)
endfunction

def Foo()
  substitute/foo
  substitute/foo/bar
  substitute/foo/bar/
  let bar = str->substitute(str, pat, sub, flags)
enddef


" various delimiters

s!/!//! " comment
" s"/"//" " comment (works but disallowed)
s#/#//# " comment
s$/$//$ " comment
s%/%//% " comment
s&/&//& " comment
s'/'//' " comment
s(/(//( " comment
s)/)//) " comment
s*/*//* " comment
s+/+//+ " comment
s,/,//, " comment
s-/-//- " comment
s././/. " comment
s/X/XX/ " comment
s:/://: " comment
s;/;//; " comment
s</<//< " comment
s=/=//= " comment
s>/>//> " comment
s?/?//? " comment
s@/@//@ " comment
s[/[//[ " comment
" s\/\//\ " comment (disallowed)
s]/]//] " comment
s^/^//^ " comment
s_/_//_ " comment
s`/`//` " comment
s{/{//{ " comment
" s|/|//| " comment (disallowed)
s}/}//} " comment
s~/~//~ " comment

s !/!//! " comment
" s "/"//" " comment (works but disallowed)
s #/#//# " comment
s $/$//$ " comment
s %/%//% " comment
s &/&//& " comment
s '/'//' " comment
s (/(//( " comment
s )/)//) " comment
s */*//* " comment
s +/+//+ " comment
s ,/,//, " comment
s -/-//- " comment
s ././/. " comment
s /X/XX/ " comment
s :/://: " comment
s ;/;//; " comment
s </<//< " comment
s =/=//= " comment
s >/>//> " comment
s ?/?//? " comment
s @/@//@ " comment
s [/[//[ " comment
" s \/\//\ " comment (disallowed)
s ]/]//] " comment
s ^/^//^ " comment
s _/_//_ " comment
s `/`//` " comment
s {/{//{ " comment
" s |/|//| " comment (disallowed)
s }/}//} " comment
s ~/~//~ " comment

s//{string}/
s //{string}/


" Repeat commands

s
:s
s 42
:s 42
s42
:s42

s cegiInp#lr
:s cegiInp#lr
s cegiInp#lr42
:s cegiInp#lr42
s cegiInp#lr 42
:s cegiInp#lr 42

sg
:sg
sgi
:sgi
sg 42
:sg 42
sgi 42
:sgi 42
sg42
:sg42
sgi42
:sgi42


" :& and :~

&
&&
~
~&

&  42
&& 42
~  42
~& 42

&42
&&42
~42
~&42

&  lp#
&& lp#
~  lp#
~& lp#

&lp#
&&lp#
~lp#
~&lp#

&  42 lp#
&& 42 lp#
~  42 lp#
~& 42 lp#

&42 lp#
&&42 lp#
~42 lp#
~&42 lp#

&  42lp#
&& 42lp#
~  42lp#
~& 42lp#

&cegiInp#lr
&&cegiInp#lr
~cegiInp#lr
~&cegiInp#lr

&cegiInp#lr  42
&&cegiInp#lr 42
~cegiInp#lr  42
~&cegiInp#lr 42

&cegiInp#lr42
&&cegiInp#lr42
~cegiInp#lr42
~&cegiInp#lr42

&cegiInp#lr  lp#
&&cegiInp#lr lp#
~cegiInp#lr  lp#
~&cegiInp#lr lp#

&cegiInp#lrlp#
&&cegiInp#lrlp#
~cegiInp#lrlp#
~&cegiInp#lrlp#

&cegiInp#lr  42 lp#
&&cegiInp#lr 42 lp#
~cegiInp#lr  42 lp#
~&cegiInp#lr 42 lp#

&cegiInp#lr42lp#
&&cegiInp#lr42lp#
~cegiInp#lr42lp#
~&cegiInp#lr42lp#


" 2 and 3 letter repeat-previous variants

:sc  | :sce | :scg | :sci | :scI | :scn | :scp | :scl |
:sgc | :sge | :sg  | :sgi | :sgI | :sgn | :sgp | :sgl | :sgr
:sic | :sie |      | :si  | :siI | :sin | :sip |      | :sir
:sIc | :sIe | :sIg | :sIi | :sI  | :sIn | :sIp | :sIl | :sIr
:src |      | :srg | :sri | :srI | :srn | :srp | :srl | :sr


" exceptions
:scr  " is  `:scriptnames`
:se   " is  `:set`
:sig  " is  `:sign`
:sil  " is  `:silent`
:sn   " is  `:snext`
:sp   " is  `:split`
:sl   " is  `:sleep`
:sre  " is  `:srewind`


" Vi compatibility

s\/{string}/
s\?{string}?
s\&{string}&

s \/{string}/
s \?{string}?
s \&{string}&


" Trailing comment and bar

s" comment
s| echo "Foo"

s " comment
s | echo "Foo"


" Issue #13883

def Test()
  str[s]
  str(s)
enddef

