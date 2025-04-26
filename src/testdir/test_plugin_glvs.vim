" Tests for GetLatestVimScripts plugin

" vim feature
set nocp
set cpo&vim

" constants
const s:dotvim= has("win32") ? "vimfiles" : ".vim"
const s:scriptdir = "$HOME/" . s:dotvim . "/GetLatest"
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

    " ensure we have the external tools we need
    call CheckTool('download')    " for HTTP fetch (curl or wget)
    call CheckTool('tar')         " for .tar.xz unpacking

    " add the required GetLatest dir (note $HOME is a dummy)
    call mkdir(s:scriptdir, "p")
    let &runtimepath = "$HOME/" . s:dotvim . "," . s:vimdir . "/runtime"

    " add plugin dir
    call mkdir("$HOME/" . s:dotvim . "/plugin")

    " doc file is required for the packages which use :helptags
    let docdir = "$HOME/" . s:dotvim . "/doc"
    call mkdir(docdir, "p")
    exe "split " . docdir . "/tags"
    w!
    bwipe!

    " load required plugins, getscript.vim would be loaded manually by the test
    " (instead of relying on autoload) because set up depends on shell selection
    runtime plugin/vimballPlugin.vim
    runtime plugin/getscriptPlugin.vim
endfunc

func CheckTool(tool)
    " define tools location
    if has('win32')
        if executable('git')
           let git_path = trim(system('powershell -Command "Split-Path (Split-Path (gcm git).Source)"'))
        endif

        if a:tool == 'bunzip2'
          if executable('bunzip2')
              let g:GetLatestVimScripts_bunzip2= "bunzip2"
          elseif executable('7z')
              let g:GetLatestVimScripts_bunzip2= "7z x"
          elseif exists('git_path')
              let g:GetLatestVimScripts_bunzip2= git_path . '\usr\bin\bunzip2'
          else
              throw "Skipped: Missing tool to decompress .bz2 files"
          endif
        endif

        if a:tool == 'gunzip'
          if executable('gunzip')
              let g:GetLatestVimScripts_gunzip= "gunzip"
          elseif executable('7z')
              let g:GetLatestVimScripts_gunzip="7z e"
          elseif exists('git_path')
              let g:GetLatestVimScripts_gunzip= git_path . '\usr\bin\gunzip'
          else
              throw "Skipped: Missing tool to decompress .gz files"
          endif
        endif

        if a:tool == 'unxz'
          if executable('unxz')
              let g:GetLatestVimScripts_unxz= "unxz"
          elseif executable('7z')
              let g:GetLatestVimScripts_unxz="7z x"
          elseif exists('git_path')
              let g:GetLatestVimScripts_unxz= git_path . '\mingw64\bin\unxz'
          else
              throw "Skipped: Missing tool to decompress .xz files"
          endif
        endif
    else
        " Mac or Unix
        if a:tool ==# 'bunzip2'
            if executable('bunzip2')
                let g:GetLatestVimScripts_bunzip2= "bunzip2"
            else
                throw "Skipped: Missing tool to decompress .bz2 files"
            endif
        endif

        if a:tool ==# 'gunzip'
            if executable('gunzip')
                let g:GetLatestVimScripts_gunzip= "gunzip"
            else
                throw "Skipped: Missing tool to decompress .gz files"
            endif
        endif

        if a:tool ==# 'unxz'
            if executable('unxz')
                let g:GetLatestVimScripts_unxz= "unxz"
            else
                throw "Skipped: Missing tool to decompress .xz files"
            endif
        endif

        " ----------------------------------------------------------------
        " tool=download => curl or wget
        if a:tool ==# 'download'
            if executable('curl')
                let g:GetLatestVimScripts_download = 'curl -f -L -o'
            elseif executable('wget')
                let g:GetLatestVimScripts_download = 'wget -q -O'
            else
                throw 'Skipped: Missing tool to download scripts'
            endif
        endif

        " tool=tar => tar or bsdtar
        if a:tool ==# 'tar'
            if executable('tar')
                let g:GetLatestVimScripts_tar = 'tar'
            elseif executable('bsdtar')
                let g:GetLatestVimScripts_tar = 'bsdtar'
            else
                throw 'Skipped: Missing tar utility'
            endif
        endif
    endif
endfunc

func TearDown()
    call delete("$HOME/" . s:dotvim, "rf")

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
    elseif a:shell == "powershell"
        if !has("win32")
            throw 'Skipped: powershell desktop is missing'
        endif
        set shell=powershell shellcmdflag=-NoProfile\ -Command shellxquote=\"  \
        set shellpipe=2>&1  \| Out-File\ -Encoding\ default shellredir=2>&1  \| Out-File\ -Encoding\ default
    elseif a:shell == "pwsh"
        if !executable("pwsh")
            throw 'Skipped: powershell core is missing'
        endif
        set shell=pwsh shellcmdflag=-NoProfile\ -c shellpipe=>%s\ 2>&1 shellredir=>%s\ 2>&1
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
    exe "split " . s:scriptdir . "/GetLatestVimScripts.dat"
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
        let check = filereadable("$HOME/" . s:dotvim . "/" . s:packages[a:package]['package'])
        call assert_true(check, "The plugin was not downloaded")
    endif

    call assert_true(s:packages[a:package]->has_key('files'), "This package lacks validation files")
    for file in s:packages[a:package]['files']
        let check = filereadable("$HOME/" . s:dotvim . "/" . file)
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
    call CheckTool('bunzip2')

    " select different shells
    call SetShell('default')

    " add the corresponding script
    call SelectScript('vim.bz2')

    " load the plugins specified
    GLVS

    call ValidateInstall('vim.bz2')
endfunc

func Test_glvs_powershell_vim_bz2()
    call CheckTool('bunzip2')

    " select different shells
    call SetShell('powershell')

    " add the corresponding script
    call SelectScript('vim.bz2')

    " load the plugins specified
    GLVS

    call ValidateInstall('vim.bz2')
endfunc

func Test_glvs_pwsh_vim_bz2()
    call CheckTool('bunzip2')

    " select different shells
    call SetShell('pwsh')

    " add the corresponding script
    call SelectScript('vim.bz2')

    " load the plugins specified
    GLVS

    call ValidateInstall('vim.bz2')
endfunc

func Test_glvs_default_vba_gz()
    call CheckTool('gunzip')

    " select different shells
    call SetShell('default')

    " add the corresponding script
    call SelectScript('vba.gz')

    " load the plugins specified
    GLVS

    call ValidateInstall('vba.gz')
endfunc

func Test_glvs_powershell_vba_gz()
    call CheckTool('gunzip')

    " select different shells
    call SetShell('powershell')

    " add the corresponding script
    call SelectScript('vba.gz')

    " load the plugins specified
    GLVS

    call ValidateInstall('vba.gz')
endfunc

func Test_glvs_pwsh_vba_gz()
    call CheckTool('gunzip')

    " select different shells
    call SetShell('pwsh')

    " add the corresponding script
    call SelectScript('vba.gz')

    " load the plugins specified
    GLVS

    call ValidateInstall('vba.gz')
endfunc

func Test_glvs_default_tar_xz()
    call CheckTool('unxz')

    " select different shells
    call SetShell('default')

    " add the corresponding script
    call SelectScript('tar.xz')

    " load the plugins specified
    GLVS

    call ValidateInstall('tar.xz')
endfunc

func Test_glvs_powershell_tar_xz()
    call CheckTool('unxz')

    " select different shells
    call SetShell('powershell')

    " add the corresponding script
    call SelectScript('tar.xz')

    " load the plugins specified
    GLVS

    call ValidateInstall('tar.xz')
endfunc

func Test_glvs_pwsh_tar_xz()
    call CheckTool('unxz')

    " select different shells
    call SetShell('pwsh')

    " add the corresponding script
    call SelectScript('tar.xz')

    " load the plugins specified
    GLVS

    call ValidateInstall('tar.xz')
endfunc

" vim: set sw=4 ts=4 et:
