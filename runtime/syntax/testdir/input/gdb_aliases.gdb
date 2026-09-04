# GDB commands with dedicated aliases
# gdb --batch --ex 'help all' | sed -n 's/ -- .*//; /, /{s/, /\n/g; G; p}'

break
brea
bre
br
b

clear
cl

delete
del
d

delete tracepoints
delete tr

disable
disa
dis

enable
en

trace
trac
tra
tr
tp

print
inspect
p

print-object
po

set architecture
set processor

set check
set ch
set c

set gnutarget
set g

set guile
set gu

set inferior-tty
tty

set mipsfpu double
set mipsfpu 1
set mipsfpu yes
set mipsfpu on

set mipsfpu none
set mipsfpu 0
set mipsfpu no
set mipsfpu off

set print
set pr
set p

set record
set rec

set remote binary-download-packet
set remote X-packet

set remote fetch-register-packet
set remote p-packet

set remote set-register-packet
set remote P-packet

set style address
set style disassembler address

set style function
set style disassembler symbol

set sysroot
set solib-absolute-prefix

set unwind-on-signal
set unwindonsignal

set variable
set var

with
w

forward-search
fo
search

generate-core-file
gcore

list
l

reverse-search
rev

maintenance
mt

maintenance cplus
maintenance cp

maintenance info
maintenance i

compile
expression

guile
gu

guile-repl
gr

python
py

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

record goto begin
record goto start

record stop
record s

continue
fg
c

finish
fin

jump
j

next
n

nexti
ni

reverse-continue
rc

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

step
s

stepi
si

thread
t

until
u

backtrace
where
bt

down
dow
do

frame
f

info
inf
i

info breakpoints
info b

info frame
info f

info guile
info gu

info record
info rec

info registers
info r

info sharedlibrary
info dll

info signals
info handle

info stack
info s

info tracepoints
info tp

info w32 thread-information-block
info w32 tib

macro expand
macro exp

macro expand-once
macro exp1

show
info set

show check
show ch
show c

show convenience
show conv

show guile
show gu

show print
show pr
show p

show record
show rec

show remote binary-download-packet
show remote X-packet

show remote fetch-register-packet
show remote p-packet

show remote set-register-packet
show remote P-packet

show style address
show style disassembler address

show style function
show style disassembler symbol

show sysroot
show solib-absolute-prefix

show unwind-on-signal
show unwindonsignal

adi examine
adi x

help
h

overlay
ov
ovly

pipe
|

quit
exit
q

shell
!

tui focus
fs
focus

tui layout
layout

tui refresh
refresh

tui window height
wh
winheight

tui window width
winwidth

tfind end
tfind none

while-stepping
stepping
ws

