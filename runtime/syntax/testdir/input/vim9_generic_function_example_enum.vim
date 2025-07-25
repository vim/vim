vim9script
# VIM_TEST_SETUP highlight link vim9DefTypeParam Todo
# VIM_TEST_SETUP highlight link vim9EnumValue Identifier
# VIM_TEST_SETUP let g:vimsyn_folding = "ef"
# VIM_TEST_SETUP setl fdc=2 fdl=99 fdm=syntax
# See: https://github.com/vim/vim/pull/17313#issuecomment-3033537127 (Aliaksei Budavei)


enum CommonPair
    HelloWorld<string, string>('hello', 'world'),
    Booleans<bool, bool>(true, false)

    const _fst: any
    const _snd: any

    def new<T, U>(fst: T, snd: U)
        this._fst = fst
        this._snd = snd
    enddef

    def First<T>(): T
        return this._fst
    enddef

    def Second<T>(): T
        return this._snd
    enddef

    def string(): string
        return printf("(%s, %s)", this._fst, this._snd)
    enddef
endenum

echo CommonPair.HelloWorld
echo CommonPair.Booleans

