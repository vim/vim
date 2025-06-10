vim9script

# VIM_TEST_SETUP hi link vim9Super Todo
# VIM_TEST_SETUP hi link vim9This Todo
# See: https://github.com/vim/vim/pull/16476#issuecomment-2635119478


class A
    const _value: number

    def new(this._value)
    enddef

    def K(): func(any): number
        return ((_: any) => this._value)
    enddef
endclass

class B extends A
    def K(): func(any): number
        return ((_: any) => super._value)
    enddef
endclass

echo 1 == A.new(1).K()(null)
echo 2 == B.new(2).K()(null)

