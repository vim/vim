" Vim syntax file
" Language:	    GRUB Configuration File
" Maintainer:	    Nikolai Weibull <source@pcppopper.org>
" URL:		    http://www.pcppopper.org/vim/syntax/pcp/grub/
" Latest Revision:  2004-05-06
" arch-tag:	    7a56ddd0-e551-44bc-b8c0-235fedbdf3c0

if version < 600
  syntax clear
elseif exists("b:current_syntax")
  finish
endif

" comments
syn region  grubComment	    display oneline start="^#" end="$" contains=grubTodo

" todo
syn keyword grubTodo	    contained TODO FIXME XXX NOTE

" devices
syn match   grubDevice	    display "(\([fh]d\d\|\d\+\|0x\x\+\)\(,\d\+\)\=\(,\l\)\=)"

" block lists
syn match   grubBlock	    display "\(\d\+\)\=+\d\+\(,\(\d\+\)\=+\d\+\)*"

" numbers
syn match   grubNumbers	    display "+\=\<\d\+\|0x\x\+\>"

syn match  grubBegin	    display "^" nextgroup=grubCommand,grubComment skipwhite

" menu commands
syn keyword grubCommand	    contained default fallback hiddenmenu timeout title

" general commands
syn keyword grubCommand	    contained bootp color device dhcp hide ifconfig pager
syn keyword grubCommand	    contained partnew parttype password rarp serial setkey
syn keyword grubCommand	    contained terminal tftpserver unhide blocklist boot cat
syn keyword grubCommand	    contained chainloader cmp configfile debug displayapm
syn keyword grubCommand	    contained displaymem embed find fstest geometry halt help
syn keyword grubCommand	    contained impsprobe initrd install ioprobe kernel lock
syn keyword grubCommand	    contained makeactive map md5crypt module modulenounzip pause
syn keyword grubCommand	    contained quit reboot read root rootnoverify savedefault
syn keyword grubCommand	    contained setup testload testvbe uppermem vbeprobe

" colors
syn match   grubColor	    "\(blink-\)\=\(black\|blue\|green\|cyan\|red\|magenta\|brown\|yellow\|white\)"
syn match   grubColor	    "\<\(blink-\)\=light-\(gray\|blue\|green\|cyan\|red\|magenta\)"
syn match   grubColor	    "\<\(blink-\)\=dark-gray"

" specials
syn keyword grubSpecial	    saved

" Define the default highlighting.
" For version 5.7 and earlier: only when not done already
" For version 5.8 and later: only when an item doesn't have highlighting yet
if version >= 508 || !exists("did_grub_syn_inits")
  if version < 508
    let did_grub_syn_inits = 1
    command -nargs=+ HiLink hi link <args>
  else
    command -nargs=+ HiLink hi def link <args>
  endif

  HiLink grubComment	Comment
  HiLink grubTodo	Todo
  HiLink grubNumbers	Number
  HiLink grubDevice	Identifier
  HiLink grubBlock	Identifier
  HiLink grubCommand	Keyword
  HiLink grubColor	Identifier
  HiLink grubSpecial	Special
  delcommand HiLink
endif

let b:current_syntax = "grub"

" vim: set sts=2 sw=2:
