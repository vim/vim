" Vim :substitute command


substitute/foo/bar/&
substitute/foo/bar/cegiInp#lr

snomagic/foo/bar/&
snomagic/foo/bar/cegiInp#lr

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
  substitute/foo/bar/
  let bar = str->substitute(str, pat, sub, flags)
endfunction

def Foo()
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
" FIXME - matches vimUserFunc
" s(/(//( " comment
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
" FIXME - matches vimUserFunc
" s (/(//( " comment
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

" FIXME
&
&&
~
~&

" FIXME
&cegiInp#lr
&&cegiInp#lr
~cegiInp#lr
~&cegiInp#lr

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

str[s]
str(s)

def Test()
  str[s]
  str(s)
enddef

