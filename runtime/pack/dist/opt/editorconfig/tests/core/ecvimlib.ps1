# ecvimlib.ps1: Editorconfig Vimscript core CLI, PowerShell version,
# library routines.
# Copyright (c) 2018--2019 Chris White.  All rights reserved.
# Licensed CC-BY-SA, version 3.0 or any later version, at your option.
#
# N.B.: debug output uses Warning only because those are displayed by default.

#Requires -Version 3

# Get the directory of this script.  From
# https://stackoverflow.com/a/5466355/2877364 by
# https://stackoverflow.com/users/23283/jaredpar

$global:DIR = $PSScriptRoot

### Set up debugging output ============================================

$global:debug=$env:EDITORCONFIG_DEBUG  # Debug filename

if($global:debug -and ($global:debug -notmatch '^/')) {
    # Relative to this script unless it starts with a slash.  This is because
    # cwd is usually not $DIR when testing.
    $global:debug="${DIR}/${global:debug}"
}

### Process args =======================================================

function de64_args($argv) {
    $argv | % {
        $b64 = $_ -replace '-','=' -replace '_','/' -replace '\.','+'
        [System.Text.Encoding]::UTF8.GetString([System.Convert]::FromBase64String($b64))
    }
}

### Helpers ============================================================

# Append a string to $debug in UTF-8 rather than the default UTF-16
filter global:D($file = $debug) {
    if($debug) {
        echo $_ | Out-File -FilePath $file -Encoding utf8 -Append
    }
}

# Escape a string for Vim
function global:vesc($str) {
    return "'" + ($str -replace "'","''") + "'"
}

# Escape a string for a command-line argument.
# See https://docs.microsoft.com/en-us/dotnet/api/system.diagnostics.processstartinfo.arguments?view=netframework-4.7.2
function global:argesc($arg) {
    return '"' + ($arg -replace '"','"""') + '"'
}

### Find the Vim EXE ===================================================

function global:Find-Vim
{
    if($env:VIM_EXE) {
        if($debug) { echo "Using env Vim $($env:VIM_EXE)" | D }
        return $env:VIM_EXE
    }

    $vims = @(get-childitem 'c:\program files*\vim\**\vim.exe' | `
            sort LastWriteTime -Descending)     # @() => always array

    # write-host ($vims | format-table | out-string)    # DEBUG
    # write-host ($vims | get-member | out-string)
    if($vims.count -gt 0) {
        if($debug) { echo "Using found Vim $($vims[0].FullName)" | D }
        return $vims[0].FullName
    }

    throw "Could not find vim.exe.  Please set VIM_EXE to the path to your Vim."
} #Find-Vim

### Runner =============================================================

# Run a process with the given arguments.
function global:run_process
{
    param(
        [Parameter(Mandatory=$true, Position=0)][string]$run,
        [string]$extrapath,
        [string]$stdout,        # Redirect stdout to this file
        [string]$stderr,        # Redirect stderr to this file
        [string[]]$argv         # Arguments to $run
    )
    $si = new-object Diagnostics.ProcessStartInfo
    if($extrapath) {
        $si.EnvironmentVariables['path']+=";${extrapath}"
    }
    $si.FileName=$run

    # Stringify the arguments (blech)
    $argstr = $argv | % { (argesc $_) + ' ' }
    $si.Arguments = $argstr;

    if($debug) { echo "Running process $run with arguments >>$argstr<<" | D }

    $si.UseShellExecute=$false
    # DEBUG  $si.RedirectStandardInput=$true
    if($stdout) {
        if($debug) { echo "Saving stdout to ${stdout}" | D }
        $si.RedirectStandardOutput=$true;
    }
    if($stderr) {
        if($debug) { echo "Saving stderr to ${stderr}" | D }
        $si.RedirectStandardError=$true;
    }

    $p = [Diagnostics.Process]::Start($si)
    # DEBUG $p.StandardInput.Close()        # < /dev/null

    $p.WaitForExit()
    $retval = $p.ExitCode

    if($stdout) {
        echo "Standard output:" | D $stdout
        $p.StandardOutput.ReadToEnd() | `
            Out-File -FilePath $stdout -Encoding utf8 -Append
    }

    if($stderr) {
        echo "Standard error:" | D $stderr
        $p.StandardError.ReadToEnd() | `
            Out-File -FilePath $stderr -Encoding utf8 -Append
    }

    $p.Close()

    return $retval
}

if($debug) {
    echo "======================================================" | D
    Get-Date -format F | D
}

$global:VIM = Find-Vim
