# GDB commands
# gdb --batch --ex 'help all' | sed 's/^\(Command class:\|Unclassified commands\)/\# \1/; s/ -- .*//; s/, /\n/g'


# Command class: aliases


# Command class: breakpoints

awatch
break
brea
bre
br
b
break-range
catch
catch assert
catch catch
catch exception
catch exec
catch fork
catch handlers
catch load
catch rethrow
catch signal
catch syscall
catch throw
catch unload
catch vfork
clear
cl
commands
end
condition
delete
del
d
delete bookmark
delete breakpoints
delete checkpoint
delete display
delete mem
delete tracepoints
delete tr
delete tvariable
disable
disa
dis
disable breakpoints
disable display
disable frame-filter
disable mem
disable missing-debug-handler
disable missing-objfile-handler
disable pretty-printer
disable probes
disable type-printer
disable unwinder
disable xmethod
dprintf
enable
en
enable breakpoints
enable breakpoints count
enable breakpoints delete
enable breakpoints once
enable count
enable delete
enable display
enable frame-filter
enable mem
enable missing-debug-handler
enable missing-objfile-handler
enable once
enable pretty-printer
enable probes
enable type-printer
enable unwinder
enable xmethod
ftrace
hbreak
ignore
rbreak
rwatch
save
save breakpoints
save gdb-index
save tracepoints
skip
skip delete
skip disable
skip enable
skip file
skip function
strace
tbreak
tcatch
tcatch assert
tcatch catch
tcatch exception
tcatch exec
tcatch fork
tcatch handlers
tcatch load
tcatch rethrow
tcatch signal
tcatch syscall
tcatch throw
tcatch unload
tcatch vfork
thbreak
trace
trac
tra
tr
tp
watch

# Command class: data

agent-printf
append
append binary
append binary memory
append binary value
append memory
append value
call
disassemble
display
dump
dump binary
dump binary memory
dump binary value
dump ihex
dump ihex memory
dump ihex value
dump memory
dump srec
dump srec memory
dump srec value
dump tekhex
dump tekhex memory
dump tekhex value
dump value
dump verilog
dump verilog memory
dump verilog value
explore
explore type
explore value
find
init-if-undefined
mem
memory-tag
memory-tag check
memory-tag print-allocation-tag
memory-tag print-logical-tag
memory-tag set-allocation-tag
memory-tag with-logical-tag
output
print
inspect
p
print-object
po
printf
ptype
restore
set
set ada
set ada print-signatures
set ada source-charset
set ada trust-PAD-over-XVS
set agent
set always-read-ctf
set annotate
set architecture
set processor
set args
set arm
set arm abi
set arm apcs32
set arm disassembler
set arm fallback-mode
set arm force-mode
set arm fpu
set arm unwind-secure-frames
set auto-connect-native-target
set auto-load
set auto-load gdb-scripts
set auto-load libthread-db
set auto-load local-gdbinit
set auto-load python-scripts
set auto-load safe-path
set auto-load scripts-directory
set auto-solib-add
set backtrace
set backtrace limit
set backtrace past-entry
set backtrace past-main
set basenames-may-differ
set breakpoint
set breakpoint always-inserted
set breakpoint auto-hw
set breakpoint condition-evaluation
set breakpoint pending
set can-use-hw-watchpoints
set case-sensitive
set charset
set check
set ch
set c
set check range
set check type
set circular-trace-buffer
set code-cache
set coerce-float-to-double
set compile-args
set compile-gcc
set complaints
set confirm
set cp-abi
set cris-dwarf2-cfi
set cris-mode
set cris-version
set cwd
set data-directory
set dcache
set dcache line-size
set dcache size
set debug
set debug aarch64
set debug arc
set debug arch
set debug arm
set debug auto-load
set debug bfd-cache
set debug bpf
set debug breakpoint
set debug check-physname
set debug coff-pe-read
set debug compile
set debug compile-cplus-scopes
set debug compile-cplus-types
set debug csky
set debug displaced
set debug dwarf-die
set debug dwarf-line
set debug dwarf-read
set debug entry-values
set debug event-loop
set debug expression
set debug fortran-array-slicing
set debug frame
set debug hppa
set debug index-cache
set debug infcall
set debug infrun
set debug jit
set debug libthread-db
set debug linux-namespaces
set debug linux-nat
set debug mach-o
set debug microblaze
set debug mips
set debug notification
set debug observer
set debug or1k
set debug overload
set debug parser
set debug py-breakpoint
set debug py-micmd
set debug py-unwind
set debug record
set debug remote
set debug remote-packet-max-chars
set debug riscv
set debug riscv breakpoints
set debug riscv gdbarch
set debug riscv infcall
set debug riscv unwinder
set debug separate-debug-file
set debug serial
set debug skip
set debug solib
set debug solib-dsbt
set debug stap-expression
set debug symbol-lookup
set debug symfile
set debug symtab-create
set debug target
set debug threads
set debug timestamp
set debug tui
set debug varobj
set debug xml
set debug xtensa
set debug-file-directory
set debuginfod
set debuginfod enabled
set debuginfod urls
set debuginfod verbose
set default-collect
set demangle-style
set detach-on-fork
set direct-call-timeout
set directories
set disable-randomization
set disassemble-next-line
set disassembler-options
set disassembly-flavor
set disconnected-dprintf
set disconnected-tracing
set displaced-stepping
set dprintf-channel
set dprintf-function
set dprintf-style
set dump-excluded-mappings
set editing
set endian
set environment
set exec-direction
set exec-done-display
set exec-file-mismatch
set exec-wrapper
set extended-prompt
set extension-language
set filename-display
set follow-exec-mode
set follow-fork-mode
set fortran
set fortran repack-array-slices
set frame-filter
set frame-filter priority
set gnutarget
set g
set guile
set gu
set guile print-stack
set height
set heuristic-fence-post
set history
set history expansion
set history filename
set history remove-duplicates
set history save
set history size
set host-charset
set index-cache
set index-cache directory
set index-cache enabled
set indirect-call-timeout
set inferior-tty
tty
set input-radix
set interactive-mode
set language
set libthread-db-search-path
set listsize
set logging
set logging debugredirect
set logging enabled
set logging file
set logging overwrite
set logging redirect
set max-completions
set max-user-call-depth
set max-value-size
set may-call-functions
set may-insert-breakpoints
set may-insert-fast-tracepoints
set may-insert-tracepoints
set may-interrupt
set may-write-memory
set may-write-registers
set mem
set mem inaccessible-by-default
set mi-async
set mips
set mips abi
set mips compression
set mips mask-address
set mipsfpu
set mipsfpu auto
set mipsfpu double
set mipsfpu 1
set mipsfpu yes
set mipsfpu on
set mipsfpu none
set mipsfpu 0
set mipsfpu no
set mipsfpu off
set mipsfpu single
set multiple-symbols
set non-stop
set observer
set opaque-type-resolution
set osabi
set output-radix
set overload-resolution
set pagination
set powerpc
set powerpc exact-watchpoints
set powerpc soft-float
set powerpc vector-abi
set print
set pr
set p
set print address
set print array
set print array-indexes
set print asm-demangle
set print characters
set print demangle
set print elements
set print entry-values
set print finish
set print frame-arguments
set print frame-info
set print inferior-events
set print max-depth
set print max-symbolic-offset
set print memory-tag-violations
set print nibbles
set print null-stop
set print object
set print pascal_static-members
set print pretty
set print raw-frame-arguments
set print raw-values
set print repeats
set print sevenbit-strings
set print static-members
set print symbol
set print symbol-filename
set print symbol-loading
set print thread-events
set print type
set print type hex
set print type methods
set print type nested-type-limit
set print type typedefs
set print union
set print vtbl
set prompt
set python
set python dont-write-bytecode
set python ignore-environment
set python print-stack
set radix
set range-stepping
set ravenscar
set ravenscar task-switching
set record
set rec
set record btrace
set record btrace bts
set record btrace bts buffer-size
set record btrace cpu
set record btrace cpu auto
set record btrace cpu none
set record btrace pt
set record btrace pt buffer-size
set record btrace pt event-tracing
set record btrace replay-memory-access
set record full
set record full insn-number-max
set record full memory-query
set record full stop-at-limit
set record function-call-history-size
set record instruction-history-size
set remote
set remote TracepointSource-packet
set remote Z-packet
set remote access-watchpoint-packet
set remote agent-packet
set remote allow-packet
set remote attach-packet
set remote binary-download-packet
set remote X-packet
set remote binary-upload-packet
set remote breakpoint-commands-packet
set remote btrace-conf-bts-size-packet
set remote btrace-conf-pt-event-tracing-packet
set remote btrace-conf-pt-ptwrite-packet
set remote btrace-conf-pt-size-packet
set remote catch-syscalls-packet
set remote conditional-breakpoints-packet
set remote conditional-tracepoints-packet
set remote ctrl-c-packet
set remote disable-btrace-packet
set remote disable-randomization-packet
set remote enable-btrace-bts-packet
set remote enable-btrace-pt-packet
set remote environment-hex-encoded-packet
set remote environment-reset-packet
set remote environment-unset-packet
set remote error-message-packet
set remote exec-event-feature-packet
set remote exec-file
set remote fast-tracepoints-packet
set remote fetch-register-packet
set remote p-packet
set remote fork-event-feature-packet
set remote get-thread-information-block-address-packet
set remote get-thread-local-storage-address-packet
set remote hardware-breakpoint-limit
set remote hardware-breakpoint-packet
set remote hardware-watchpoint-length-limit
set remote hardware-watchpoint-limit
set remote hostio-close-packet
set remote hostio-fstat-packet
set remote hostio-lstat-packet
set remote hostio-open-packet
set remote hostio-pread-packet
set remote hostio-pwrite-packet
set remote hostio-readlink-packet
set remote hostio-setfs-packet
set remote hostio-stat-packet
set remote hostio-unlink-packet
set remote hwbreak-feature-packet
set remote install-in-trace-packet
set remote interrupt-on-connect
set remote interrupt-sequence
set remote kill-packet
set remote library-info-packet
set remote library-info-svr4-packet
set remote memory-map-packet
set remote memory-read-packet-size
set remote memory-tagging-address-check-packet
set remote memory-tagging-feature-packet
set remote memory-write-packet-size
set remote multiprocess-feature-packet
set remote no-resumed-stop-reply-packet
set remote noack-packet
set remote osdata-packet
set remote pass-signals-packet
set remote pid-to-exec-file-packet
set remote program-signals-packet
set remote query-attached-packet
set remote read-aux-vector-packet
set remote read-btrace-conf-packet
set remote read-btrace-packet
set remote read-fdpic-loadmap-packet
set remote read-sdata-object-packet
set remote read-siginfo-object-packet
set remote read-watchpoint-packet
set remote reverse-continue-packet
set remote reverse-step-packet
set remote run-packet
set remote search-memory-packet
set remote set-register-packet
set remote P-packet
set remote set-working-dir-packet
set remote software-breakpoint-packet
set remote startup-with-shell-packet
set remote static-tracepoints-packet
set remote supported-packets-packet
set remote swbreak-feature-packet
set remote symbol-lookup-packet
set remote system-call-allowed
set remote target-features-packet
set remote thread-events-packet
set remote thread-options-packet
set remote threads-packet
set remote trace-buffer-size-packet
set remote trace-status-packet
set remote traceframe-info-packet
set remote unwind-info-block-packet
set remote verbose-resume-packet
set remote verbose-resume-supported-packet
set remote vfork-event-feature-packet
set remote write-siginfo-object-packet
set remote write-watchpoint-packet
set remote-mips64-transfers-32bit-regs
set remoteaddresssize
set remotecache
set remoteflow
set remotelogbase
set remotelogfile
set remotetimeout
set remotewritesize
set riscv
set riscv numeric-register-names
set riscv use-compressed-breakpoints
set schedule-multiple
set scheduler-locking
set script-extension
set serial
set serial baud
set serial parity
set sh
set sh calling-convention
set solib-search-path
set source
set source open
set stack-cache
set startup-quietly
set startup-with-shell
set step-mode
set stop-on-solib-events
set struct-convention
set style
set style address
set style disassembler address
set style address background
set style address foreground
set style address intensity
set style command
set style command background
set style command foreground
set style command intensity
set style disassembler
set style disassembler comment
set style disassembler comment background
set style disassembler comment foreground
set style disassembler comment intensity
set style disassembler enabled
set style disassembler immediate
set style disassembler immediate background
set style disassembler immediate foreground
set style disassembler immediate intensity
set style disassembler mnemonic
set style disassembler mnemonic background
set style disassembler mnemonic foreground
set style disassembler mnemonic intensity
set style disassembler register
set style disassembler register background
set style disassembler register foreground
set style disassembler register intensity
set style emoji
set style enabled
set style error-prefix
set style filename
set style filename background
set style filename foreground
set style filename intensity
set style function
set style disassembler symbol
set style function background
set style function foreground
set style function intensity
set style highlight
set style highlight background
set style highlight foreground
set style highlight intensity
set style line-number
set style line-number background
set style line-number foreground
set style line-number intensity
set style metadata
set style metadata background
set style metadata foreground
set style metadata intensity
set style sources
set style title
set style title background
set style title foreground
set style title intensity
set style tui-active-border
set style tui-active-border background
set style tui-active-border foreground
set style tui-border
set style tui-border background
set style tui-border foreground
set style tui-current-position
set style variable
set style variable background
set style variable foreground
set style variable intensity
set style version
set style version background
set style version foreground
set style version intensity
set style warning-prefix
set substitute-path
set suppress-cli-notifications
set sysroot
set solib-absolute-prefix
set target-charset
set target-file-system-kind
set target-wide-charset
set tcp
set tcp auto-retry
set tcp connect-timeout
set tdesc
set tdesc filename
set trace-buffer-size
set trace-commands
set trace-notes
set trace-stop-notes
set trace-user
set trust-readonly-sections
set tui
set tui active-border-mode
set tui border-kind
set tui border-mode
set tui compact-source
set tui mouse-events
set tui tab-width
set unwind-on-signal
set unwindonsignal
set unwind-on-terminating-exception
set unwind-on-timeout
set use-coredump-filter
set use-deprecated-index-sections
set variable
set var
set verbose
set warn-language-frame-mismatch
set watchdog
set width
set write
undisplay
whatis
with
w
x

# Command class: files

add-symbol-file
add-symbol-file-from-memory
cd
core-file
directory
edit
exec-file
file
forward-search
fo
search
generate-core-file
gcore
list
l
load
nosharedlibrary
path
pwd
remote
remote delete
remote get
remote put
remove-symbol-file
reverse-search
rev
section
sharedlibrary
symbol-file

# Command class: internals

maintenance
mt
maintenance agent
maintenance agent-eval
maintenance agent-printf
maintenance btrace
maintenance btrace clear
maintenance btrace clear-packet-history
maintenance btrace packet-history
maintenance canonicalize
maintenance check
maintenance check libthread-db
maintenance check psymtabs
maintenance check symtabs
maintenance check xml-descriptions
maintenance cplus
maintenance cp
maintenance cplus first_component
maintenance demangler-warning
maintenance deprecate
maintenance dump-me
maintenance expand-symtabs
maintenance flush
maintenance flush dcache
maintenance flush register-cache
maintenance flush source-cache
maintenance flush symbol-cache
maintenance frame-unwinder
maintenance frame-unwinder disable
maintenance frame-unwinder enable
maintenance ignore-probes
maintenance info
maintenance i
maintenance info bfds
maintenance info blocks
maintenance info breakpoints
maintenance info btrace
maintenance info frame-unwinders
maintenance info inline-frames
maintenance info jit
maintenance info line-table
maintenance info linux-lwps
maintenance info program-spaces
maintenance info psymtabs
maintenance info screen
maintenance info sections
maintenance info selftests
maintenance info symtabs
maintenance info target-sections
maintenance internal-error
maintenance internal-warning
maintenance packet
maintenance print
maintenance print arc
maintenance print arc arc-instruction
maintenance print architecture
maintenance print c-tdesc
maintenance print cooked-registers
maintenance print core-file-backed-mappings
maintenance print dummy-frames
maintenance print frame-id
maintenance print msymbols
maintenance print objfiles
maintenance print psymbols
maintenance print raw-registers
maintenance print record-instruction
maintenance print reggroups
maintenance print register-groups
maintenance print registers
maintenance print remote-registers
maintenance print statistics
maintenance print symbol-cache
maintenance print symbol-cache-statistics
maintenance print symbols
maintenance print target-stack
maintenance print type
maintenance print unwind
maintenance print user-registers
maintenance print xml-tdesc
maintenance selftest
maintenance set
maintenance set ada
maintenance set ada ignore-descriptive-types
maintenance set backtrace-on-fatal-signal
maintenance set bfd-sharing
maintenance set btrace
maintenance set btrace pt
maintenance set btrace pt skip-pad
maintenance set catch-demangler-crashes
maintenance set check-libthread-db
maintenance set debuginfod
maintenance set debuginfod download-sections
maintenance set demangler-warning
maintenance set demangler-warning quit
maintenance set dwarf
maintenance set dwarf always-disassemble
maintenance set dwarf max-cache-age
maintenance set dwarf synchronous
maintenance set force-internal-tls-address-lookup
maintenance set gnu-source-highlight
maintenance set gnu-source-highlight enabled
maintenance set ignore-prologue-end-flag
maintenance set internal-error
maintenance set internal-error backtrace
maintenance set internal-error corefile
maintenance set internal-error quit
maintenance set internal-warning
maintenance set internal-warning backtrace
maintenance set internal-warning corefile
maintenance set internal-warning quit
maintenance set libopcodes-styling
maintenance set libopcodes-styling enabled
maintenance set per-command
maintenance set per-command space
maintenance set per-command symtab
maintenance set per-command time
maintenance set profile
maintenance set selftest
maintenance set selftest verbose
maintenance set show-all-tib
maintenance set show-debug-regs
maintenance set symbol-cache-size
maintenance set target-async
maintenance set target-non-stop
maintenance set test-settings
maintenance set test-settings auto-boolean
maintenance set test-settings boolean
maintenance set test-settings enum
maintenance set test-settings filename
maintenance set test-settings integer
maintenance set test-settings optional-filename
maintenance set test-settings string
maintenance set test-settings string-noescape
maintenance set test-settings uinteger
maintenance set test-settings zinteger
maintenance set test-settings zuinteger
maintenance set test-settings zuinteger-unlimited
maintenance set tui-left-margin-verbose
maintenance set tui-resize-message
maintenance set worker-threads
maintenance show
maintenance show ada
maintenance show ada ignore-descriptive-types
maintenance show backtrace-on-fatal-signal
maintenance show bfd-sharing
maintenance show btrace
maintenance show btrace pt
maintenance show btrace pt skip-pad
maintenance show catch-demangler-crashes
maintenance show check-libthread-db
maintenance show debuginfod
maintenance show debuginfod download-sections
maintenance show demangler-warning
maintenance show demangler-warning quit
maintenance show dwarf
maintenance show dwarf always-disassemble
maintenance show dwarf max-cache-age
maintenance show dwarf synchronous
maintenance show force-internal-tls-address-lookup
maintenance show gnu-source-highlight
maintenance show gnu-source-highlight enabled
maintenance show ignore-prologue-end-flag
maintenance show internal-error
maintenance show internal-error backtrace
maintenance show internal-error corefile
maintenance show internal-error quit
maintenance show internal-warning
maintenance show internal-warning backtrace
maintenance show internal-warning corefile
maintenance show internal-warning quit
maintenance show libopcodes-styling
maintenance show libopcodes-styling enabled
maintenance show per-command
maintenance show per-command space
maintenance show per-command symtab
maintenance show per-command time
maintenance show profile
maintenance show selftest
maintenance show selftest verbose
maintenance show show-all-tib
maintenance show show-debug-regs
maintenance show symbol-cache-size
maintenance show target-async
maintenance show target-non-stop
maintenance show test-options-completion-result
maintenance show test-settings
maintenance show test-settings auto-boolean
maintenance show test-settings boolean
maintenance show test-settings enum
maintenance show test-settings filename
maintenance show test-settings integer
maintenance show test-settings optional-filename
maintenance show test-settings string
maintenance show test-settings string-noescape
maintenance show test-settings uinteger
maintenance show test-settings zinteger
maintenance show test-settings zuinteger
maintenance show test-settings zuinteger-unlimited
maintenance show tui-left-margin-verbose
maintenance show tui-resize-message
maintenance show worker-threads
maintenance space
maintenance test-options
maintenance test-options require-delimiter
maintenance test-options unknown-is-error
maintenance test-options unknown-is-operand
maintenance time
maintenance translate-address
maintenance undeprecate
maintenance wait-for-index-cache
maintenance with

# Command class: obscure

checkpoint
compare-sections
compile
expression
compile code
end
compile file
end
compile print
end
complete
guile
end
gu
end
guile-repl
gr
monitor
python
end
py
end
python-interactive
pi
record
rec
record btrace
record b
record btrace bts
record bts
record btrace pt
record pt
record delete
record del
record d
record full
record full restore
record function-call-history
record goto
record goto begin
record goto start
record goto end
record instruction-history
record save
record stop
record s
restart
stop

# Command class: running

advance
attach
continue
fg
c
detach
detach checkpoint
detach inferiors
disconnect
finish
fin
handle
inferior
interrupt
jump
j
kill
kill inferiors
next
n
nexti
ni
queue-signal
reverse-continue
rc
reverse-finish
reverse-next
rn
reverse-nexti
rni
reverse-step
rs
reverse-stepi
rsi
run
r
signal
start
starti
step
s
stepi
si
taas
target
target core
target exec
target extended-remote
target native
target record-btrace
target record-core
target record-full
target remote
target tfile
task
task apply
task apply all
tfaas
thread
t
thread apply
thread apply all
thread find
thread name
until
u

# Command class: stack

backtrace
where
bt
down
dow
do
faas
frame
f
frame address
frame apply
frame apply all
frame apply level
frame function
frame level
frame view
return
select-frame
select-frame address
select-frame function
select-frame level
select-frame view
up

# Command class: status

info
inf
i
info address
info all-registers
info args
info auto-load
info auto-load gdb-scripts
info auto-load libthread-db
info auto-load local-gdbinit
info auto-load python-scripts
info auxv
info bookmarks
info breakpoints
info b
info checkpoints
info classes
info common
info connections
info copying
info dcache
info display
info exceptions
info extensions
info files
info float
info frame
info f
info frame address
info frame function
info frame level
info frame view
info frame-filter
info functions
info guile
info gu
info inferiors
info io_registers
info line
info linker-namespaces
info locals
info macro
info macros
info main
info mem
info missing-debug-handlers
info missing-objfile-handlers
info module
info module functions
info module variables
info modules
info os
info pretty-printer
info probes
info probes all
info probes dtrace
info probes stap
info proc
info proc all
info proc cmdline
info proc cwd
info proc exe
info proc files
info proc mappings
info proc stat
info proc status
info program
info record
info rec
info registers
info r
info scope
info selectors
info sharedlibrary
info dll
info signals
info handle
info skip
info source
info sources
info stack
info s
info static-tracepoint-markers
info symbol
info target
info tasks
info terminal
info threads
info tracepoints
info tp
info tvariables
info type-printers
info types
info unwinder
info variables
info vector
info vtbl
info w32
info w32 thread-information-block
info w32 tib
info warranty
info watchpoints
info win
info xmethod
macro
macro define
macro expand
macro exp
macro expand-once
macro exp1
macro list
macro undef
show
info set
show ada
show ada print-signatures
show ada source-charset
show ada trust-PAD-over-XVS
show agent
show always-read-ctf
show annotate
show architecture
show args
show arm
show arm abi
show arm apcs32
show arm disassembler
show arm fallback-mode
show arm force-mode
show arm fpu
show arm unwind-secure-frames
show auto-connect-native-target
show auto-load
show auto-load gdb-scripts
show auto-load libthread-db
show auto-load local-gdbinit
show auto-load python-scripts
show auto-load safe-path
show auto-load scripts-directory
show auto-solib-add
show backtrace
show backtrace limit
show backtrace past-entry
show backtrace past-main
show basenames-may-differ
show breakpoint
show breakpoint always-inserted
show breakpoint auto-hw
show breakpoint condition-evaluation
show breakpoint pending
show can-use-hw-watchpoints
show case-sensitive
show charset
show check
show ch
show c
show check range
show check type
show circular-trace-buffer
show code-cache
show coerce-float-to-double
show commands
show compile-args
show compile-gcc
show complaints
show configuration
show confirm
show convenience
show conv
show copying
show cp-abi
show cris-dwarf2-cfi
show cris-mode
show cris-version
show cwd
show data-directory
show dcache
show dcache line-size
show dcache size
show debug
show debug aarch64
show debug arc
show debug arch
show debug arm
show debug auto-load
show debug bfd-cache
show debug bpf
show debug breakpoint
show debug check-physname
show debug coff-pe-read
show debug compile
show debug compile-cplus-scopes
show debug compile-cplus-types
show debug csky
show debug displaced
show debug dwarf-die
show debug dwarf-line
show debug dwarf-read
show debug entry-values
show debug event-loop
show debug expression
show debug fortran-array-slicing
show debug frame
show debug hppa
show debug index-cache
show debug infcall
show debug infrun
show debug jit
show debug libthread-db
show debug linux-namespaces
show debug linux-nat
show debug mach-o
show debug microblaze
show debug mips
show debug notification
show debug observer
show debug or1k
show debug overload
show debug parser
show debug py-breakpoint
show debug py-micmd
show debug py-unwind
show debug record
show debug remote
show debug remote-packet-max-chars
show debug riscv
show debug riscv breakpoints
show debug riscv gdbarch
show debug riscv infcall
show debug riscv unwinder
show debug separate-debug-file
show debug serial
show debug skip
show debug solib
show debug solib-dsbt
show debug stap-expression
show debug symbol-lookup
show debug symfile
show debug symtab-create
show debug target
show debug threads
show debug timestamp
show debug tui
show debug varobj
show debug xml
show debug xtensa
show debug-file-directory
show debuginfod
show debuginfod enabled
show debuginfod urls
show debuginfod verbose
show default-collect
show demangle-style
show detach-on-fork
show direct-call-timeout
show directories
show disable-randomization
show disassemble-next-line
show disassembler-options
show disassembly-flavor
show disconnected-dprintf
show disconnected-tracing
show displaced-stepping
show dprintf-channel
show dprintf-function
show dprintf-style
show dump-excluded-mappings
show editing
show endian
show environment
show exec-direction
show exec-done-display
show exec-file-mismatch
show exec-wrapper
show extended-prompt
show extension-language
show filename-display
show follow-exec-mode
show follow-fork-mode
show fortran
show fortran repack-array-slices
show frame-filter
show frame-filter priority
show gnutarget
show guile
show gu
show guile print-stack
show height
show heuristic-fence-post
show history
show history expansion
show history filename
show history remove-duplicates
show history save
show history size
show host-charset
show index-cache
show index-cache directory
show index-cache enabled
show index-cache stats
show indirect-call-timeout
show inferior-tty
show input-radix
show interactive-mode
show jit-reader-directory
show language
show libthread-db-search-path
show listsize
show logging
show logging debugredirect
show logging enabled
show logging file
show logging overwrite
show logging redirect
show max-completions
show max-user-call-depth
show max-value-size
show may-call-functions
show may-insert-breakpoints
show may-insert-fast-tracepoints
show may-insert-tracepoints
show may-interrupt
show may-write-memory
show may-write-registers
show mem
show mem inaccessible-by-default
show mi-async
show mips
show mips abi
show mips compression
show mips mask-address
show mipsfpu
show multiple-symbols
show non-stop
show observer
show opaque-type-resolution
show osabi
show output-radix
show overload-resolution
show pagination
show paths
show powerpc
show powerpc exact-watchpoints
show powerpc soft-float
show powerpc vector-abi
show print
show pr
show p
show print address
show print array
show print array-indexes
show print asm-demangle
show print characters
show print demangle
show print elements
show print entry-values
show print finish
show print frame-arguments
show print frame-info
show print inferior-events
show print max-depth
show print max-symbolic-offset
show print memory-tag-violations
show print nibbles
show print null-stop
show print object
show print pascal_static-members
show print pretty
show print raw-frame-arguments
show print raw-values
show print repeats
show print sevenbit-strings
show print static-members
show print symbol
show print symbol-filename
show print symbol-loading
show print thread-events
show print type
show print type hex
show print type methods
show print type nested-type-limit
show print type typedefs
show print union
show print vtbl
show prompt
show python
show python dont-write-bytecode
show python ignore-environment
show python print-stack
show radix
show range-stepping
show ravenscar
show ravenscar task-switching
show record
show rec
show record btrace
show record btrace bts
show record btrace bts buffer-size
show record btrace cpu
show record btrace pt
show record btrace pt buffer-size
show record btrace pt event-tracing
show record btrace replay-memory-access
show record full
show record full insn-number-max
show record full memory-query
show record full stop-at-limit
show record function-call-history-size
show record instruction-history-size
show remote
show remote TracepointSource-packet
show remote Z-packet
show remote access-watchpoint-packet
show remote agent-packet
show remote allow-packet
show remote attach-packet
show remote binary-download-packet
show remote X-packet
show remote binary-upload-packet
show remote breakpoint-commands-packet
show remote btrace-conf-bts-size-packet
show remote btrace-conf-pt-event-tracing-packet
show remote btrace-conf-pt-ptwrite-packet
show remote btrace-conf-pt-size-packet
show remote catch-syscalls-packet
show remote conditional-breakpoints-packet
show remote conditional-tracepoints-packet
show remote ctrl-c-packet
show remote disable-btrace-packet
show remote disable-randomization-packet
show remote enable-btrace-bts-packet
show remote enable-btrace-pt-packet
show remote environment-hex-encoded-packet
show remote environment-reset-packet
show remote environment-unset-packet
show remote error-message-packet
show remote exec-event-feature-packet
show remote exec-file
show remote fast-tracepoints-packet
show remote fetch-register-packet
show remote p-packet
show remote fork-event-feature-packet
show remote get-thread-information-block-address-packet
show remote get-thread-local-storage-address-packet
show remote hardware-breakpoint-limit
show remote hardware-breakpoint-packet
show remote hardware-watchpoint-length-limit
show remote hardware-watchpoint-limit
show remote hostio-close-packet
show remote hostio-fstat-packet
show remote hostio-lstat-packet
show remote hostio-open-packet
show remote hostio-pread-packet
show remote hostio-pwrite-packet
show remote hostio-readlink-packet
show remote hostio-setfs-packet
show remote hostio-stat-packet
show remote hostio-unlink-packet
show remote hwbreak-feature-packet
show remote install-in-trace-packet
show remote interrupt-on-connect
show remote interrupt-sequence
show remote kill-packet
show remote library-info-packet
show remote library-info-svr4-packet
show remote memory-map-packet
show remote memory-read-packet-size
show remote memory-tagging-address-check-packet
show remote memory-tagging-feature-packet
show remote memory-write-packet-size
show remote multiprocess-feature-packet
show remote no-resumed-stop-reply-packet
show remote noack-packet
show remote osdata-packet
show remote pass-signals-packet
show remote pid-to-exec-file-packet
show remote program-signals-packet
show remote query-attached-packet
show remote read-aux-vector-packet
show remote read-btrace-conf-packet
show remote read-btrace-packet
show remote read-fdpic-loadmap-packet
show remote read-sdata-object-packet
show remote read-siginfo-object-packet
show remote read-watchpoint-packet
show remote reverse-continue-packet
show remote reverse-step-packet
show remote run-packet
show remote search-memory-packet
show remote set-register-packet
show remote P-packet
show remote set-working-dir-packet
show remote software-breakpoint-packet
show remote startup-with-shell-packet
show remote static-tracepoints-packet
show remote supported-packets-packet
show remote swbreak-feature-packet
show remote symbol-lookup-packet
show remote system-call-allowed
show remote target-features-packet
show remote thread-events-packet
show remote thread-options-packet
show remote threads-packet
show remote trace-buffer-size-packet
show remote trace-status-packet
show remote traceframe-info-packet
show remote unwind-info-block-packet
show remote verbose-resume-packet
show remote verbose-resume-supported-packet
show remote vfork-event-feature-packet
show remote write-siginfo-object-packet
show remote write-watchpoint-packet
show remote-mips64-transfers-32bit-regs
show remoteaddresssize
show remotecache
show remoteflow
show remotelogbase
show remotelogfile
show remotetimeout
show remotewritesize
show riscv
show riscv numeric-register-names
show riscv use-compressed-breakpoints
show schedule-multiple
show scheduler-locking
show script-extension
show serial
show serial baud
show serial parity
show sh
show sh calling-convention
show solib-search-path
show source
show source open
show stack-cache
show startup-quietly
show startup-with-shell
show step-mode
show stop-on-solib-events
show struct-convention
show style
show style address
show style disassembler address
show style address background
show style address foreground
show style address intensity
show style command
show style command background
show style command foreground
show style command intensity
show style disassembler
show style disassembler comment
show style disassembler comment background
show style disassembler comment foreground
show style disassembler comment intensity
show style disassembler enabled
show style disassembler immediate
show style disassembler immediate background
show style disassembler immediate foreground
show style disassembler immediate intensity
show style disassembler mnemonic
show style disassembler mnemonic background
show style disassembler mnemonic foreground
show style disassembler mnemonic intensity
show style disassembler register
show style disassembler register background
show style disassembler register foreground
show style disassembler register intensity
show style emoji
show style enabled
show style error-prefix
show style filename
show style filename background
show style filename foreground
show style filename intensity
show style function
show style disassembler symbol
show style function background
show style function foreground
show style function intensity
show style highlight
show style highlight background
show style highlight foreground
show style highlight intensity
show style line-number
show style line-number background
show style line-number foreground
show style line-number intensity
show style metadata
show style metadata background
show style metadata foreground
show style metadata intensity
show style sources
show style title
show style title background
show style title foreground
show style title intensity
show style tui-active-border
show style tui-active-border background
show style tui-active-border foreground
show style tui-border
show style tui-border background
show style tui-border foreground
show style tui-current-position
show style variable
show style variable background
show style variable foreground
show style variable intensity
show style version
show style version background
show style version foreground
show style version intensity
show style warning-prefix
show substitute-path
show suppress-cli-notifications
show sysroot
show solib-absolute-prefix
show target-charset
show target-file-system-kind
show target-wide-charset
show tcp
show tcp auto-retry
show tcp connect-timeout
show tdesc
show tdesc filename
show trace-buffer-size
show trace-commands
show trace-notes
show trace-stop-notes
show trace-user
show trust-readonly-sections
show tui
show tui active-border-mode
show tui border-kind
show tui border-mode
show tui compact-source
show tui mouse-events
show tui tab-width
show unwind-on-signal
show unwindonsignal
show unwind-on-terminating-exception
show unwind-on-timeout
show use-coredump-filter
show use-deprecated-index-sections
show user
show values
show varsize-limit
show verbose
show version
show warn-language-frame-mismatch
show warranty
show watchdog
show width
show write

# Command class: support

add-auto-load-safe-path
add-auto-load-scripts-directory
adi
adi assign
adi examine
adi x
alias
apropos
define
end
define-prefix
demangle
document
end
dont-repeat
down-silently
echo
help
h
if
interpreter-exec
make
new-ui
overlay
ov
ovly
overlay auto
overlay list-overlays
overlay load-target
overlay manual
overlay map-overlay
overlay off
overlay unmap-overlay
pipe
|
quit
exit
q
shell
!
source
up-silently
while

# Command class: text-user-interface

+
-
<
>
tui
tui disable
tui enable
tui focus
fs
focus
tui layout
layout
tui layout asm
tui layout next
tui layout prev
tui layout regs
tui layout split
tui layout src
tui new-layout
tui refresh
refresh
tui reg
tui window
tui window height
wh
winheight
tui window width
winwidth
update

# Command class: tracepoints

actions
collect
end
passcount
tdump
teval
tfind
tfind end
tfind none
tfind line
tfind outside
tfind pc
tfind range
tfind start
tfind tracepoint
tsave
tstart
tstatus
tstop
tvariable
while-stepping
stepping
ws

# Command class: user-defined


# Unclassified commands

add-inferior
clone-inferior
eval
flash-erase
function
function _any_caller_is
function _any_caller_matches
function _as_string
function _caller_is
function _caller_matches
function _cimag
function _creal
function _gdb_maint_setting
function _gdb_maint_setting_str
function _gdb_setting
function _gdb_setting_str
function _isvoid
function _memeq
function _regex
function _shell
function _streq
function _strlen
jit-reader-load
jit-reader-unload
remove-inferiors
unset
unset environment
unset exec-wrapper
unset substitute-path
unset tdesc
unset tdesc filename
