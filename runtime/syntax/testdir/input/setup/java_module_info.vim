vim9script

# Test filenames are required to begin with the filetype name prefix,
# whereas the name of a Java module declaration must be "module-info".
const name_a: string = 'input/java_module_info.java'
const name_b: string = 'input/module-info.java'

def ChangeFilename()
    exec 'saveas! ' .. name_b
enddef

def RestoreFilename()
    exec 'saveas! ' .. name_a
    delete(name_b)
enddef

autocmd_add([{
    replace:	true,
    group:	'java_syntax_tests',
    event:	'BufEnter',
    pattern:	name_a,
    cmd:	'ChangeFilename()',
    once:	true,
}, {
    group:	'java_syntax_tests',
    event:	['BufLeave', 'ExitPre'],
    pattern:	name_b,
    cmd:	'RestoreFilename()',
    once:	true,
}])

g:java_syntax_previews = [476]
