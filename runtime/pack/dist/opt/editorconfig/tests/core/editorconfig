#!/bin/bash
# editorconfig: Editorconfig Vimscript core CLI
# Copyright (c) 2018--2019 Chris White.  All rights reserved.
# Licensed CC-BY-SA, version 3.0 or any later version, at your option.

# Documentation {{{1
helpstr=$(cat<<'EOF'
editorconfig: command-line invoker for the Vimscript editorconfig core

Normal usage:
    editorconfig [-f <config-file name>] [-b <version>]
        [-x <extra information>] <filenames...>

The default <config-file name> is ".editorconfig".
If -b is given, behave as <version>.
If -x is given, the <extra information> is included in the debug-output file.

Other options:
    editorconfig -h, --help     Show this help
    editorconfig -v, --version  Show version information

Environment variables:
    VIM_EXE             File/path of vim (default "vim")
    EDITORCONFIG_DEBUG  File/path to which to append debug output

EOF
)

# }}}1

# Get the directory of this script into $this_script_dir. {{{1
# From https://stackoverflow.com/a/246128/2877364 by
# https://stackoverflow.com/users/407731 et al.

this_script_dir=
function get_dir()
{
    local script_source_path="${BASH_SOURCE[0]}"
    while [ -h "$script_source_path" ]; do
        # resolve $script_source_path until the file is no longer a symlink
        this_script_dir="$( cd -P "$( dirname "$script_source_path" )" >/dev/null && pwd )"
        script_source_path="$(readlink "$script_source_path")"
        [[ $script_source_path != /* ]] && script_source_path="$this_script_dir/$script_source_path"
            # if $script_source_path was a relative symlink, we need to resolve
            # it relative to the path where the symlink file was located
    done
    this_script_dir="$( cd -P "$( dirname "$script_source_path" )" >/dev/null && pwd )"
} #get_dir()

get_dir

# }}}1

# Setup debug output, if $EDITORCONFIG_DEBUG is given {{{1
debug="${EDITORCONFIG_DEBUG}"   # Debug filename
if [[ $debug && $debug != /* ]]; then     # Relative to this script unless it
    debug="${this_script_dir}/${debug}"     # starts with a slash.  This is because
fi                              # cwd is usually not $this_script_dir when testing.
if [[ $debug ]] && ! touch "$debug"; then
    echo "Could not write file '$debug' - aborting" 1>&2
    exit 1
fi

[[ $debug ]] && echo "$(date) ==================================" >> "$debug"

# }}}1

# Option processing {{{1

# Use a manually-specified Vim, if any
if [[ $VIM_EXE ]]; then
    vim_pgm="$VIM_EXE"
else
    vim_pgm="vim"
fi

# Command-line options
confname=
ver=
print_ver=
extra_info=

while getopts 'hvf:b:-:x:' opt ; do
    case "$opt" in
        (v) print_ver=1
            ;;

        (f) confname="$OPTARG"
            ;;

        (b) ver="$OPTARG"
            ;;

        (-) case "$OPTARG" in   # hacky long-option processing
                version)    print_ver=1
                            ;;
                dummy)      # A dummy option so that I can test
                            # list-valued EDITORCONFIG_CMD
                            ;;
                help)       echo "$helpstr"
                            exit 0
                            ;;
            esac
            ;;

        (h) echo "$helpstr"
            exit 0
            ;;

        # A way to put the test name into the log
        (x) extra_info="$OPTARG"
            ;;

    esac
done

shift $(( $OPTIND - 1 ))

if [[ $print_ver ]]; then
    echo "EditorConfig VimScript Core Version 0.12.2"
    exit 0
fi

if (( "$#" < 1 )); then
    exit 1
fi

if [[ $1 = '-' ]]; then
    echo "Reading filenames from stdin not yet supported" 1>&2  # TODO
    exit 1
fi

# }}}1

# Build the Vim command line {{{1

fn="$(mktemp)"      # Vim will write the settings into here.  ~stdout.
script_output_fn="${debug:+$(mktemp)}"  # Vim's :messages.  ~stderr.

cmd="call editorconfig_core#currbuf_cli({"

# Names
cmd+="'output':'${fn//\'/\'\'}', "
    # filename to put the settings in
[[ $debug ]] && cmd+=" 'dump':'${script_output_fn//\'/\'\'}', "
    # where to put debug info

# Filenames to get the settings for
cmd+="'target':["
for f in "$@" ; do
    cmd+="'${f//\'/\'\'}', "
done
cmd+="],"
    # filename to get the settings for

# Job
cmd+="}, {"
[[ $confname ]] && cmd+="'config':'${confname//\'/\'\'}', "
    # config name (e.g., .editorconfig)
[[ $ver ]] && cmd+="'version':'${ver//\'/\'\'}', "
    # version number we should behave as
cmd+="})"

vim_args=(
    -c "set runtimepath+=$this_script_dir/../.."
    -c "$cmd"
)

# }}}1

# Run the editorconfig core through Vim {{{1
# Thanks for options to
# http://vim.wikia.com/wiki/Vim_as_a_system_interpreter_for_vimscript .
# Add -V1 to the below for debugging output.
# Do not output anything to stdout or stderr,
# since it messes up ctest's interpretation
# of the results.

"$vim_pgm" -nNes -i NONE -u NONE -U NONE \
    "${vim_args[@]}" \
    </dev/null &>> "${debug:-/dev/null}"
vimstatus="$?"
if [[ $vimstatus -eq 0 ]]; then
    cat "$fn"
fi

# }}}1

# Produce debug output {{{1
# Debug output cannot be included on stdout or stderr, because
# ctest's regex check looks both of those places.  Therefore, dump to a
# separate debugging file.
if [[ $debug ]]
then
    [[ $extra_info ]] && echo "--- $extra_info ---" >> "$debug"
    echo "Vim in $vim_pgm" >> "$debug"
    echo "Current directory: $(pwd)" >> "$debug"
    echo "Script directory: $this_script_dir" >> "$debug"
    echo Vim args: "${vim_args[@]}" >> "$debug"
    #od -c <<<"${vim_args[@]}" >> "$debug"
    echo "Vim returned $vimstatus" >> "$debug"
    echo "Vim messages were: " >> "$debug"
    cat "$script_output_fn" >> "$debug"
    echo "Output was:" >> "$debug"
    od -c "$fn" >> "$debug"

    rm -f "$script_output_fn"
fi

# }}}1

# Cleanup {{{1

rm -f "$fn"

# }}}1

exit "$vimstatus"   # forward the Vim exit status to the caller
# vi: set ft=sh fdm=marker:
