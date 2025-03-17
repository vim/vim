vim9script
# Vim9 function return types
# VIM_TEST_SETUP hi link vimUserType Todo


# Issue #14442 (vim.vim: confusion for function return types starting on another line)

def TestA______________________________________________________________():
    \ void
enddef

def TestB____(result: dict<list<number>>, fs: list<func(number): number>):
    \ void
enddef

def TestC____(maybe: bool, F: func(): dict<func(number): number>): func():
	\ void
    return () => {
    }
enddef

def TestD____(fs: list<func(): dict<func(number): bool>>): func(): func():
	\ void
    return () => () => {
    }
enddef

class Tests
    def TestA__________________________________________________________():
	\ void
    enddef

    def TestB(result: dict<list<number>>, fs: list<func(number): number>):
	\ void
    enddef

    def TestC(maybe: bool, F: func(): dict<func(number): number>): func():
	    \ void
	return () => {
	}
    enddef

    def TestD(fs: list<func(): dict<func(number): bool>>): func(): func():
	    \ void
	return () => () => {
	}
    enddef
endclass

def F(G: func(number, number, number): lib.Trampoline):
					\ func(number, number, number):
					\ func(): lib.Trampoline
	return ((H: func(number, number, number): lib.Trampoline) =>
				(a1: number, a2: number, n: number) =>
				() =>
		H(a2, (a1 + a2), (n - 1)))(G)
enddef

defcompile

