" Vim :mark and :k commands
" :k not available in Vim9
" VIM_TEST_SETUP hi link vimMarkArg Todo


mark `
mark '
mark a
mark A
mark k
mark K
mark z
mark Z
mark [
mark ]
mark <
mark >

:mark `
:mark '
:mark a
:mark A
:mark k
:mark K
:mark z
:mark Z
:mark [
:mark ]
:mark <
:mark >

call Foo() | mark `
call Foo() | mark '
call Foo() | mark a
call Foo() | mark A
call Foo() | mark k
call Foo() | mark K
call Foo() | mark z
call Foo() | mark Z
call Foo() | mark [
call Foo() | mark ]
call Foo() | mark <
call Foo() | mark >

call Foo() | :mark `
call Foo() | :mark '
call Foo() | :mark a
call Foo() | :mark A
call Foo() | :mark k
call Foo() | :mark K
call Foo() | :mark z
call Foo() | :mark Z
call Foo() | :mark [
call Foo() | :mark ]
call Foo() | :mark <
call Foo() | :mark >

k`
k'
ka
kA
kk
kK
kz
kZ
k[
k]
k<
k>

:k`
:k'
:ka
:kA
:kk
:kK
:kz
:kZ
:k[
:k]
:k<
:k>

call Foo() | k`
call Foo() | k'
call Foo() | ka
call Foo() | kA
call Foo() | kk
call Foo() | kK
call Foo() | kz
call Foo() | kZ
call Foo() | k[
call Foo() | k]
call Foo() | k<
call Foo() | k>

call Foo() | :k`
call Foo() | :k'
call Foo() | :ka
call Foo() | :kA
call Foo() | :kk
call Foo() | :kK
call Foo() | :kz
call Foo() | :kZ
call Foo() | :k[
call Foo() | :k]
call Foo() | :k<
call Foo() | :k>

k `
k '
k a
k A
k k
k K
k z
k Z
k [
k ]
k <
k >

:k `
:k '
:k a
:k A
:k k
:k K
:k z
:k Z
:k [
:k ]
:k <
:k >

call Foo() | k `
call Foo() | k '
call Foo() | k a
call Foo() | k A
call Foo() | k k
call Foo() | k K
call Foo() | k z
call Foo() | k Z
call Foo() | k [
call Foo() | k ]
call Foo() | k <
call Foo() | k >

call Foo() | :k `
call Foo() | :k '
call Foo() | :k a
call Foo() | :k A
call Foo() | :k k
call Foo() | :k K
call Foo() | :k z
call Foo() | :k Z
call Foo() | :k [
call Foo() | :k ]
call Foo() | :k <
call Foo() | :k >


mark a | echo "FOO"
ka     | echo "FOO"
k a    | echo "FOO"
mark a " comment
ka     " comment
k a    " comment


function Foo()
  k a
  ka
  mark a
  :k a
  :ka
  :mark a
endfunction


" Errors

mark "
mark ^
mark .
" TODO: matches as vimFunc
" mark (
mark )
mark {
mark }
mark 0
mark 9

k"
k^
k.
" TODO: matches as vimFunc
" k(
k)
k{
k}
k0
k9

k "
k ^
k .
" TODO: matches as vimFunc
" k (
k )
k {
k }
k 0
k 9

