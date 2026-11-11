vim9script
# VIM_TEST_SETUP highlight link vim9UserGenericType Todo
# See: https://github.com/vim/vim/pull/21512#issuecomment-6157916214 (Aliaksei Budavei)

class Pair<T, U>
    public const X: T
    public const Y: U
endclass

var p: Pair<number, number> = Pair<number, number>.new(1, 0)
echo 1 == xor(p.X, p.Y)

