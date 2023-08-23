# editorconfig2.ps1: Editorconfig Vimscript core CLI, PowerShell version
# Copyright (c) 2018--2019 Chris White.  All rights reserved.
# Licensed CC-BY-SA, version 3.0 or any later version, at your option.
# Thanks to https://cecs.wright.edu/~pmateti/Courses/233/Labs/Scripting/bashVsPowerShellTable.html
# by Gallagher and Mateti.

#Requires -Version 3

. "$PSScriptRoot\ecvimlib.ps1"

# Argument parsing =================================================== {{{1

$argv = @(de64_args($args))

# Defaults
$report_version = $false
$set_version = ''
$config_name = '.editorconfig'
$extra_info = ''
$files=@()

# Hand-parse - pretend we're sort of like getopt.
$idx = 0
while($idx -lt $argv.count) {
    $a = $argv[$idx]

    switch -CaseSensitive -Regex ($a) {
        '^(-v|--version)$' { $report_version = $true }

        '^--dummy$' {
            # A dummy option so that I can test list-valued EDITORCONFIG_CMD
        }

        '^-f$' {
            if($idx -eq ($argv.count-1)) {
                throw '-f <filename>: no filename provided'
            } else {
                ++$idx
                $config_name = $argv[$idx]
            }
        } #-f

        '^-b$' {
            if($idx -eq ($argv.count-1)) {
                throw '-b <version>: no version provided'
            } else {
                ++$idx
                $set_version = $argv[$idx]
            }
        } #-b

        '^-x$' {
            if($idx -eq ($argv.count-1)) {
                throw '-x <extra info>: no info provided'
            } else {
                ++$idx
                $extra_info = $argv[$idx]
            }
        } #-x

        '^--$' {    # End of options, so capture the rest as filenames
            ++$idx;
            while($idx -lt $argv.count) {
                $files += $argv[$idx]
            }
        }

        default { $files += $a }
    }

    ++$idx
} # end foreach argument

# }}}1
# Argument processing ================================================ {{{1

if($debug) {
    if($extra_info -ne '') {
        echo "--- $extra_info --- "             | D
    }

    echo "Running in       $DIR"                | D
    echo "Vim executable:  $VIM"                | D
    echo "report version?  $report_version"     | D
    echo "set version to:  $set_version"        | D
    echo "config filename: $config_name"        | D
    echo "Filenames:       $files"              | D
    echo "Args:            $args"               | D
    echo "Decoded args:    $argv"               | D
}

if($report_version) {
    echo "EditorConfig VimScript Core Version 0.12.2"
    exit
}

if($files.count -lt 1) {
    exit
}

if($files[0] -eq '-') {
    echo "Reading filenames from stdin not yet supported" # TODO
    exit 1
}

$fn=[System.IO.Path]::GetTempFileName();
    # Vim will write the settings into here.  Sort of like stdout.
$script_output_fn = ''
if($debug) {
    $script_output_fn = [System.IO.Path]::GetTempFileName()
}

# Permit throwing in setup commands
$cmd = ''
if($env:EDITORCONFIG_EXTRA) {
    $cmd += $env:EDITORCONFIG_EXTRA + ' | '
}

# }}}1
# Build Vim command line ============================================= {{{1
$cmd += 'call editorconfig_core#currbuf_cli({'

# Names
$cmd += "'output':" + (vesc($fn)) + ", "
    # filename to put the settings in
if($debug) {
    $cmd += " 'dump':" + (vesc($script_output_fn)) + ", "
    # where to put debug info
}

# Filenames to get the settings for
$cmd += "'target':["
ForEach ($item in $files) {
    $cmd += (vesc($item)) + ", "
}
$cmd += "],"

# Job
$cmd += "}, {"
if($config_name) { $cmd += "'config':" + (vesc($config_name)) + ", " }
    # config name (e.g., .editorconfig)
if($set_version) { $cmd += "'version':" + (vesc($set_version)) + ", " }
    # version number we should behave as
$cmd += "})"

#$cmd =':q!'  # DEBUG
if($debug) { echo "Using Vim command ${cmd}" | D }
$vim_args = @(
    '-c', "set runtimepath+=${DIR}\..\..",
    '-c', $cmd,
    '-c', 'quit!'   # TODO write a wrapper that will cquit on exception
)

# Run editorconfig.  Thanks for options to
# http://vim.wikia.com/wiki/Vim_as_a_system_interpreter_for_vimscript .
# Add -V1 to the below for debugging output.
# Do not output anything to stdout or stderr,
# since it messes up ctest's interpretation
# of the results.

$basic_args = '-nNes','-i','NONE','-u','NONE','-U','NONE'   #, '-V1'

# }}}1
# Run Vim ============================================================ {{{1

if($debug) { echo "Running vim ${VIM}" | D }
$vimstatus = run_process $VIM -stdout $debug -stderr $debug `
    -argv ($basic_args+$vim_args)
if($debug) { echo "Done running vim" | D }

if($vimstatus -eq 0) {
    cat $fn
}

# }}}1
# Produce debug output =============================================== {{{1

# Debug output cannot be included on stdout or stderr, because
# ctest's regex check looks both of those places.  Therefore, dump to a
# separate debugging file.

if($debug) {
    echo "Current directory:" | D
    (get-item -path '.').FullName | D
    echo "Script directory: $DIR" | D
###     echo Vim args: "${vim_args[@]}" >> "$debug"
###     #od -c <<<"${vim_args[@]}" >> "$debug"
    echo "Vim returned $vimstatus" | D
    echo "Vim messages were: " | D
    cat $script_output_fn | D
    echo "Output was:" | D

    # Modified from https://www.itprotoday.com/powershell/get-hex-dumps-files-powershell
    Get-Content $script_output_fn -Encoding Byte -ReadCount 16 | `
    ForEach-Object {
        $output = ""
        $chars = ''
        foreach ( $byte in $_ ) {
            $output += "{0:X2} " -f $byte
            if( ($byte -ge 32) -and ($byte -le 127) ) {
                $chars += [char]$byte
            } else {
                $chars += '.'
            }
        }
        $output + ' ' + $chars
    } | D

    del -Force $script_output_fn
} #endif $debug

# }}}1

del -Force $fn

exit $vimstatus

# vi: set fdm=marker:
