" Tests for GetLatestVimScripts plugin

" constants
const s:dotvim= has("win32") ? "vimfiles" : ".vim"
const s:scriptdir = $"{$HOME}/{s:dotvim}/GetLatest"
const s:vimdir = expand("<script>:h:h:h")

" Before each test recreate the .vim dir structure expected by GLVS and load the plugin
func SetUp()

    " vim feature
    set nocp
    set cpo&vim

    " add the required GetLatest dir (note $HOME is a dummy)
    call mkdir(s:scriptdir, "p")
    let &runtimepath = $"{$HOME}/{s:dotvim},{s:vimdir}/runtime"

    " doc file is required for the packages which use :helptags
    let docdir = $"{$HOME}/{s:dotvim}/doc"
    call mkdir(docdir, "p")
    exe $"split {docdir}/tags"
    w!
    bwipe!

    " load required plugins, getscript.vim would be loaded manually by the test
    " (instead of relying on autoload) because set up depends on shell selection
    runtime plugin/vimballPlugin.vim
    runtime plugin/getscriptPlugin.vim

endfunc

" After each test remove the contents of the .vim dir and reset the script
func TearDown()
    call delete($"{$HOME}/{s:dotvim}", "rf")

    " getscript.vim include guard
    unlet! g:loaded_getscript g:loaded_getscriptPlugin
    " remove all globals (shell dependents)
    let script_globals = keys(g:)
    call filter(script_globals, 'v:val =~ "GetLatestVimScripts_"')
    if len(script_globals)
        call map(script_globals, '"g:" . v:val')
        exe "unlet " . script_globals->join()
    endif
endfunc

" Ancillary functions

func SetShell(shell)

    " select different shells
    if a:shell == "default"
        set shell& shellcmdflag& shellxquote& shellpipe& shellredir&
    elseif a:shell == "powershell" " help dos-powershell
        " powershell desktop is windows only
        if !has("win32")
            throw 'Skipped: powershell desktop is missing'
        endif
        set shell=powershell shellcmdflag=-Command shellxquote=\"
        set shellpipe=2>&1\ \|\ Out-File\ -Encoding\ default shellredir=2>&1\ \|\ Out-File\ -Encoding\ default
    elseif a:shell == "pwsh" " help dos-powershell
        " powershell core works crossplatform
        if !executable("pwsh")
            throw 'Skipped: powershell core is missing'
        endif
        set shell=pwsh shellcmdflag=-c shellpipe=>%s\ 2>&1 shellredir=>%s\ 2>&1
        if has("win32")
            set shellxquote=\"
        else
            set shellxquote=
        endif
    else
        assert_report("Trying to select and unknown shell")
    endif

    " reload script to force new shell options
    runtime autoload/getscript.vim

endfunc

func SelectScript(script_lines)

    " add the corresponding file
    exe $"split {s:scriptdir}/GetLatestVimScripts.dat"
    let scripts =<< trim END
        ScriptID SourceID Filename
        --------------------------
    END
    call setline(1, scripts)
    call append(line('$'), a:script_lines)
    w!
    bwipe!

endfunc

" Tests
"
func Test_glvs_load_default_vmb()

    " select different shells
    call SetShell("default")

    " add the corresponding script
    call SelectScript("6106 1 :AutoInstall: setpwsh.vmb")

    " load the plugins specified
    GLVS

    " check if installation work out
    let check = filereadable($"{$HOME}/{s:dotvim}/pack/setpwsh/start/setpwsh/doc/setpwsh.txt")
    call assert_true(check, "The plugin was not installed")

endfunc

func Test_glvs_load_pwsh_vmb()

    " select different shells
    call SetShell("pwsh")

    " add the corresponding script
    call SelectScript("6106 1 :AutoInstall: setpwsh.vmb")

    " load the plugins specified
    GLVS

    " check if installation work out
    let check = filereadable($"{$HOME}/{s:dotvim}/pack/setpwsh/start/setpwsh/doc/setpwsh.txt")
    call assert_true(check, "The plugin was not installed")

endfunc

func Test_glvs_load_powershell_vmb()

    " select different shells
    call SetShell("powershell")

    " add the corresponding script
    call SelectScript("6106 1 :AutoInstall: setpwsh.vmb")

    " load the plugins specified
    GLVS

    " check if installation work out
    let check = filereadable($"{$HOME}/{s:dotvim}/pack/setpwsh/start/setpwsh/doc/setpwsh.txt")
    call assert_true(check, "The plugin was not installed")

endfunc
