" MUSHcode syntax file
" Maintainer:	Bek Oberin <gossamer@tertius.net.au>
" Last updated by Rimnal on Mon Aug 20 08:28:56 MDT 2001

" For version 5.x: Clear all syntax items
" For version 6.x: Quit when a syntax file was already loaded
if version < 600
  syntax clear
elseif exists("b:current_syntax")
  finish
endif


" regular mush functions
syntax keyword mushFunction contained abs acos add after and andflags aposs
syntax keyword mushFunction contained asin atan before capstr cat ceil center
syntax keyword mushFunction contained comp con conn controls convsecs convtime
syntax keyword mushFunction contained cos default delete dist2d dist3d div e
syntax keyword mushFunction contained edefault edit elements elock eq escape
syntax keyword mushFunction contained exit exp extract fdiv filter first flags
syntax keyword mushFunction contained floor fold foreach findable fullname get
syntax keyword mushFunction contained get_eval grab gt gte hasattr hasflag
syntax keyword mushFunction contained home idle index insert isdbref isnum
syntax keyword mushFunction contained isword iter last lattr lcon lcstr
syntax keyword mushFunction contained ldelete left lexits ljust ln lnum loc
syntax keyword mushFunction contained locate lock log lpos lt lte lwho map
syntax keyword mushFunction contained match matchall max member merge mid min
syntax keyword mushFunction contained mix mod money mudname mul munge name
syntax keyword mushFunction contained nearby neq next not num obj objeval
syntax keyword mushFunction contained objmem or orflags owner parent parse pi
syntax keyword mushFunction contained ports pos poss power r rand remove repeat
syntax keyword mushFunction contained replace rest reverse revwords right
syntax keyword mushFunction contained rjust rloc room round s scramble search
syntax keyword mushFunction contained secs secure setdiff setinter setq
syntax keyword mushFunction contained setunion shuffle sign sin sort sortby
syntax keyword mushFunction contained space splice sqrt squish starttime stats
syntax keyword mushFunction contained strlen strmatch sub subj switch tan time
syntax keyword mushFunction contained trim trunc type u ucstr udefault ulocal
syntax keyword mushFunction contained v version visible where wordpos words
syntax keyword mushFunction contained xcon xor
" only highligh functions when they have an in-bracket immediately after
syntax match mushFunctionBrackets  "\i\I*(" contains=mushFunction

" regular mush commands
syntax keyword mushAtCommandList contained @alias @chown @clone @create
syntax keyword mushAtCommandList contained @decompile @destroy @doing @dolist
syntax keyword mushAtCommandList contained @drain @edit @emit @entrances @femit
syntax keyword mushAtCommandList contained @force @fpose @fsay @halt @last
syntax keyword mushAtCommandList contained @link @list @listmotd @lock @mudwho
syntax keyword mushAtCommandList contained @mvattr @name @notify @oemit @parent
syntax keyword mushAtCommandList contained @password @pemit @ps @quota @robot
syntax keyword mushAtCommandList contained @search @set @stats @sweep @switch
syntax keyword mushAtCommandList contained @teleport @trigger @unlink @unlock
syntax keyword mushAtCommandList contained @verb @wait @wipe
syntax match mushCommand  "@\i\I*" contains=mushAtCommandList


syntax keyword mushCommand drop enter examine get give goto help inventory
syntax keyword mushCommand kill leave look news page pose say score use
syntax keyword mushCommand version whisper DOING LOGOUT OUTPUTPREFIX
syntax keyword mushCommand OUTPUTSUFFIX QUIT SESSION WHO

syntax match mushSpecial     "\*\|!\|=\|-\|\\\|+"
syntax match mushSpecial2 contained     "\*"

syntax match mushIdentifier   "&[^ ]\+"

syntax match mushVariable   "%r\|%t\|%cr\|%[A-Za-z0-9]\+\|%#\|##\|here"

" numbers
syntax match mushNumber	+[0-9]\++

" A comment line starts with a or # or " at the start of the line
" or an @@
syntax keyword mushTodo contained	TODO FIXME XXX
syntax match	mushComment	+^\s*@@.*$+	contains=mushTodo
syntax match	mushComment	+^".*$+	contains=mushTodo
syntax match	mushComment	+^#.*$+	contains=mushTodo

syntax region	mushFuncBoundaries start="\[" end="\]" contains=mushFunction,mushFlag,mushAttributes,mushNumber,mushCommand,mushVariable,mushSpecial2

" FLAGS
syntax keyword mushFlag PLAYER ABODE BUILDER CHOWN_OK DARK FLOATING
syntax keyword mushFlag GOING HAVEN INHERIT JUMP_OK KEY LINK_OK MONITOR
syntax keyword mushFlag NOSPOOF OPAQUE QUIET STICKY TRACE UNFINDABLE VISUAL
syntax keyword mushFlag WIZARD PARENT_OK ZONE AUDIBLE CONNECTED DESTROY_OK
syntax keyword mushFlag ENTER_OK HALTED IMMORTAL LIGHT MYOPIC PUPPET TERSE
syntax keyword mushFlag ROBOT SAFE TRANSPARENT VERBOSE CONTROL_OK COMMANDS

syntax keyword mushAttribute aahear aclone aconnect adesc adfail adisconnect
syntax keyword mushAttribute adrop aefail aenter afail agfail ahear akill
syntax keyword mushAttribute aleave alfail alias amhear amove apay arfail
syntax keyword mushAttribute asucc atfail atport aufail ause away charges
syntax keyword mushAttribute cost desc dfail drop ealias efail enter fail
syntax keyword mushAttribute filter forwardlist gfail idesc idle infilter
syntax keyword mushAttribute inprefix kill lalias last lastsite leave lfail
syntax keyword mushAttribute listen move odesc odfail odrop oefail oenter
syntax keyword mushAttribute ofail ogfail okill oleave olfail omove opay
syntax keyword mushAttribute orfail osucc otfail otport oufail ouse oxenter
syntax keyword mushAttribute oxleave oxtport pay prefix reject rfail runout
syntax keyword mushAttribute semaphore sex startup succ tfail tport ufail
syntax keyword mushAttribute use va vb vc vd ve vf vg vh vi vj vk vl vm vn
syntax keyword mushAttribute vo vp vq vr vs vt vu vv vw vx vy vz


if version >= 508 || !exists("did_mush_syntax_inits")
  if version < 508
    let did_mush_syntax_inits = 1
    command -nargs=+ HiLink hi link <args>
  else
    command -nargs=+ HiLink hi def link <args>
  endif

  " The default methods for highlighting.  Can be overridden later
  HiLink mushAttribute  Constant
  HiLink mushCommand    Function
  HiLink mushComment    Comment
  HiLink mushNumber     Number
  HiLink mushSetting    PreProc
  HiLink mushFunction   Statement
  HiLink mushVariable   Identifier
  HiLink mushSpecial    Special
  HiLink mushTodo       Todo
  HiLink mushFlag       Special
  HiLink mushIdentifier Identifier

  delcommand HiLink
endif

let b:current_syntax = "mush"

" mush: ts=17
