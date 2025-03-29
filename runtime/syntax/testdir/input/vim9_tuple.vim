vim9script

# Vim9 tuples


# https://github.com/vim/vim/pull/16935#issuecomment-2738310273

function LegacyMakeEntry(key, value) abort
    return (a:key, a:value)
endfunction

def MakeEntry(key: string, value: any): tuple<string, any>
    return (key, value)
enddef

function LegacyMakeLazyList(e1, e2) abort
    return ({e1_, e2_ -> {-> [e1_, e2_]}}(a:e1, a:e2))
endfunction

def MakeLazyList(e1: any, e2: any): func(): list<any>
    return (((e1_: any, e2_: any) => () => [e1_, e2_])(e1, e2))
enddef

echo MakeEntry('key', 'value') == list2tuple(MakeLazyList('key', 'value')())
echo LegacyMakeEntry('key', 'value') == list2tuple(LegacyMakeLazyList('key', 'value')())

