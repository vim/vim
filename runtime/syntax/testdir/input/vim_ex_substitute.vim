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

