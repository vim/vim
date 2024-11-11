" Tests for GetLatestVimScripts plugin

" vim feature
set nocp
set cpo&vim

" constants
const s:dotvim= has("win32") ? "vimfiles" : ".vim"
const s:scriptdir = $"{$HOME}/{s:dotvim}/GetLatest"
const s:vimdir = expand("<script>:h:h:h")
const s:packages = {
    \ 'vmb': {
        \ 'spec': '4979 1 :AutoInstall: AnsiEsc.vim',
        \ 'files': ['plugin/AnsiEscPlugin.vim', 'autoload/AnsiEsc.vim']
        \ },
    \ 'vim.bz2': {
        \ 'spec': '514 1 :AutoInstall: mrswin.vim',
        \ 'files': ['plugin/mrswin.vim']
        \ },
    \ 'vba.gz': {
        \ 'spec': '120 1 :AutoInstall: Decho.vim',
        \ 'package': 'GetLatest/Installed/Decho.vba',
        \ 'files': ['plugin/Decho.vim', 'syntax/Decho.vim']
        \ },
    \ 'tar.xz': {
        \ 'spec': '5632 1 :AutoInstall: dumpx',
        \ 'package': 'GetLatest/Installed/dumpx.tar',
        \ 'files': ['dumpx/plugin/dumpx.vim', 'dumpx/doc/dumpx.txt']
        \ }
    \ }

" Before each test recreate the .vim dir structure expected by GLVS and load the plugin
func SetUp()

    " add the required GetLatest dir (note $HOME is a dummy)
    call mkdir(s:scriptdir, "p")
    let &runtimepath = $"{$HOME}/{s:dotvim},{s:vimdir}/runtime"

    " add plugin dir
    call mkdir($"{$HOME}/{s:dotvim}/plugin")

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

    " provide accesibility to git binary tools like unzip (for gzip.vim and other plugins)
    if has('win32') && !executable('gunzip') && executable('git')
        let git_path = trim(system('powershell -Command "Split-Path (Split-Path (gcm git).Source)"'))
        let $PATH .= $';{git_path}\usr\bin;{git_path}\mingw64\bin'
        " some of the git tools are script and bash must be called explicitly
        let g:GetLatestVimScripts_gunzip="bash gunzip"
    endif

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
        call assert_report("Trying to select and unknown shell")
    endif

    " reload script to force new shell options
    runtime autoload/getscript.vim

endfunc

func SelectScript(package)

    " add the corresponding file
    exe $"split {s:scriptdir}/GetLatestVimScripts.dat"
    let scripts =<< trim END
        ScriptID SourceID Filename
        --------------------------
    END
    call setline(1, scripts)
    call append(line('$'), s:packages[a:package]['spec'])
    w!
    bwipe!

endfunc

func ValidateInstall(package)
    " check the package is expected
    call assert_true(s:packages->has_key(a:package), "This package is unexpected")

    " check if installation work out
    if s:packages[a:package]->has_key('package')
        let check = filereadable($"{$HOME}/{s:dotvim}/".s:packages[a:package]['package'])
        call assert_true(check, "The plugin was not downloaded")
    endif

    call assert_true(s:packages[a:package]->has_key('files'), "This package lacks validation files")
    for file in s:packages[a:package]['files']
        let check = filereadable($"{$HOME}/{s:dotvim}/".file)
        call assert_true(check, "The plugin was not installed")
    endfor
endfunc

" Tests
"
func Test_glvs_default_vmb()

    " select different shells
    call SetShell('default')

    " add the corresponding script
    call SelectScript('vmb')

    " load the plugins specified
    GLVS

    call ValidateInstall('vmb')

endfunc

func Test_glvs_pwsh_vmb()

    " select different shells
    call SetShell('pwsh')

    " add the corresponding script
    call SelectScript('vmb')

    " load the plugins specified
    GLVS

    call ValidateInstall('vmb')

endfunc

func Test_glvs_powershell_vmb()

    " select different shells
    call SetShell('powershell')

    " add the corresponding script
    call SelectScript('vmb')

    " load the plugins specified
    GLVS

    call ValidateInstall('vmb')

endfunc

func Test_glvs_default_vim_bz2()

    " select different shells
    call SetShell('default')

    " add the corresponding script
    call SelectScript('vim.bz2')

    " load the plugins specified
    GLVS

    call ValidateInstall('vim.bz2')

endfunc

func Test_glvs_powershell_vim_bz2()

    " select different shells
    call SetShell('powershell')

    " add the corresponding script
    call SelectScript('vim.bz2')

    " load the plugins specified
    GLVS

    call ValidateInstall('vim.bz2')

endfunc

func Test_glvs_pwsh_vim_bz2()

    " select different shells
    call SetShell('pwsh')

    " add the corresponding script
    call SelectScript('vim.bz2')

    " load the plugins specified
    GLVS

    call ValidateInstall('vim.bz2')

endfunc

func Test_glvs_default_vba_gz()

    " select different shells
    call SetShell('default')

    " add the corresponding script
    call SelectScript('vba.gz')

    " load the plugins specified
    GLVS

    call ValidateInstall('vba.gz')

endfunc

func Test_glvs_powershell_vba_gz()

    " select different shells
    call SetShell('powershell')

    " add the corresponding script
    call SelectScript('vba.gz')

    " load the plugins specified
    GLVS

    call ValidateInstall('vba.gz')

endfunc

func Test_glvs_pwsh_vba_gz()

    " select different shells
    call SetShell('pwsh')

    " add the corresponding script
    call SelectScript('vba.gz')

    " load the plugins specified
    GLVS

    call ValidateInstall('vba.gz')

endfunc

func Test_glvs_default_tar_xz()

    " select different shells
    call SetShell('default')

    " add the corresponding script
    call SelectScript('tar.xz')

    " load the plugins specified
    GLVS

    call ValidateInstall('tar.xz')

endfunc

func Test_glvs_powershell_tar_xz()

    " select different shells
    call SetShell('powershell')

    " add the corresponding script
    call SelectScript('tar.xz')

    " load the plugins specified
    GLVS

    call ValidateInstall('tar.xz')

endfunc

func Test_glvs_pwsh_tar_xz()

    " select different shells
    call SetShell('pwsh')

    " add the corresponding script
    call SelectScript('tar.xz')

    " load the plugins specified
    GLVS

    call ValidateInstall('tar.xz')

endfunc
