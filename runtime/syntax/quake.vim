" Vim syntax file
" Language:	    Quake[1-3] Configuration File
" Maintainer:	    Nikolai Weibull <source@pcppopper.org>
" URL:		    http://www.pcppopper.org/vim/syntax/pcp/quake/
" Latest Revision:  2004-05-22
" arch-tag:	    a95793d7-cab3-4544-a78c-1cea47b5870b
" Variables: 	quake_is_quake1 - the syntax is to be used for quake1 configs
" 		quake_is_quake2 - the syntax is to be used for quake2 configs
" 		quake_is_quake3 - the syntax is to be used for quake3 configs


if version < 600
  syntax clear
elseif exists("b:current_syntax")
  finish
endif


" Set iskeyword since we need `-' (and potentially others) in keywords.
" For version 5.x: Set it globally
" For version 6.x: Set it locally
if version >= 600
  command -nargs=1 SetIsk setlocal iskeyword=<args>
else
  command -nargs=1 SetIsk set iskeyword=<args>
endif
SetIsk 48-57,65-90,97-122,+,-,_
delcommand SetIsk


" comments
syn region	quakeComment	display oneline start="//" end="$" end=";" keepend contains=quakeTodo

" todo
syn keyword	quakeTodo	contained TODO FIXME XXX NOTE

" string (can contain numbers (which should be hilighted as such)
syn region	quakeString	display oneline start=+"+ skip=+\\"+ end=+"\|$+ contains=quakeNumbers,@quakeCommands

" number
syn case ignore
syn match	quakeNumbers	display transparent "\<\d\|\.\d" contains=quakeNumber,quakeFloat,quakeOctalError,quakeOctal
syn match	quakeNumber	display contained "\d\+\>"
" Flag the first zero of an octal number as something special
syn match	quakeOctal	display contained "0\o\+\>" contains=quakeOctalZero
syn match	quakeOctalZero	display contained "\<0"
" floating point number, with dot
syn match	quakeFloat	display contained "\d\+\.\d*"
" floating point number, starting with a dot
syn match	quakeFloat	display contained "\.\d\+\>"
" flag an octal number with wrong digits
syn match	quakeOctalError	display contained "0\o*[89]\d*"
syn case match

" commands
syn case ignore
syn cluster	quakeCommands	contains=quakeCommand,quake1Command,quake12Command,Quake2Command,Quake23Command,Quake3Command

syn keyword	quakeCommand	+attack +back +forward +left +lookdown +lookup
syn keyword	quakeCommand	+mlook +movedown +moveleft +moveright +moveup
syn keyword	quakeCommand	+right +speed +strafe -attack -back bind
syn keyword	quakeCommand	bindlist centerview clear connect cvarlist dir
syn keyword	quakeCommand	disconnect dumpuser echo error exec -forward
syn keyword	quakeCommand	god heartbeat joy_advancedupdate kick kill
syn keyword	quakeCommand	killserver -left -lookdown -lookup map
syn keyword	quakeCommand	messagemode messagemode2 -mlook modellist
syn keyword	quakeCommand	-movedown -moveleft -moveright -moveup play
syn keyword	quakeCommand	quit rcon reconnect record -right say say_team
syn keyword	quakeCommand	screenshot serverinfo serverrecord serverstop
syn keyword	quakeCommand	set sizedown sizeup snd_restart soundinfo
syn keyword	quakeCommand	soundlist -speed spmap status -strafe stopsound
syn keyword	quakeCommand	toggleconsole unbind unbindall userinfo pause
syn keyword	quakeCommand	vid_restart viewpos wait weapnext weapprev

if exists("quake_is_quake1")
syn keyword	quake1Command	sv
endif

if exists("quake_is_quake1") || exists("quake_is_quake2")
syn keyword	quake12Command	+klook alias cd impulse link load save
syn keyword	quake12Command	timerefresh changing info loading
syn keyword	quake12Command	pingservers playerlist players score
endif

if exists("quake_is_quake2")
syn keyword	quake2Command	cmd demomap +use condump download drop gamemap
syn keyword	quake2Command	give gun_model setmaster sky sv_maplist wave
syn keyword	quake2Command	cmdlist gameversiona gun_next gun_prev invdrop
syn keyword	quake2Command	inven invnext invnextp invnextw invprev
syn keyword	quake2Command	invprevp invprevw invuse menu_addressbook
syn keyword	quake2Command	menu_credits menu_dmoptions menu_game
syn keyword	quake2Command	menu_joinserver menu_keys menu_loadgame
syn keyword	quake2Command	menu_main menu_multiplayer menu_options
syn keyword	quake2Command	menu_playerconfig menu_quit menu_savegame
syn keyword	quake2Command	menu_startserver menu_video
syn keyword	quake2Command	notarget precache prog togglechat vid_front
syn keyword	quake2Command	weaplast
endif

if exists("quake_is_quake2") || exists("quake_is_quake3")
syn keyword	quake23Command	imagelist modellist path z_stats
endif

if exists("quake_is_quake3")
syn keyword	quake3Command	+info +scores +zoom addbot arena banClient
syn keyword	quake3Command	banUser callteamvote callvote changeVectors
syn keyword	quake3Command	cinematic clientinfo clientkick cmd cmdlist
syn keyword	quake3Command	condump configstrings crash cvar_restart devmap
syn keyword	quake3Command	fdir follow freeze fs_openedList Fs_pureList
syn keyword	quake3Command	Fs_referencedList gfxinfo globalservers
syn keyword	quake3Command	hunk_stats in_restart -info levelshot
syn keyword	quake3Command	loaddeferred localservers map_restart mem_info
syn keyword	quake3Command	messagemode3 messagemode4 midiinfo model music
syn keyword	quake3Command	modelist net_restart nextframe nextskin noclip
syn keyword	quake3Command	notarget ping prevframe prevskin reset restart
syn keyword	quake3Command	s_disable_a3d s_enable_a3d s_info s_list s_stop
syn keyword	quake3Command	scanservers -scores screenshotJPEG sectorlist
syn keyword	quake3Command	serverstatus seta setenv sets setu setviewpos
syn keyword	quake3Command	shaderlist showip skinlist spdevmap startOribt
syn keyword	quake3Command	stats stopdemo stoprecord systeminfo togglemenu
syn keyword	quake3Command	tcmd team teamtask teamvote tell tell_attacker
syn keyword	quake3Command	tell_target testgun testmodel testshader toggle
syn keyword	quake3Command	touchFile vminfo vmprofile vmtest vosay
syn keyword	quake3Command	vosay_team vote votell vsay vsay_team vstr
syn keyword	quake3Command	vtaunt vtell vtell_attacker vtell_target weapon
syn keyword	quake3Command	writeconfig -zoom
syn match	quake3Command	display "\<[+-]button\(\d\|1[0-4]\)\>"
endif

syn case match

" Define the default highlighting.
" For version 5.7 and earlier: only when not done already
" For version 5.8 and later: only when an item doesn't have highlighting yet
if version >= 508 || !exists("did_screen_syn_inits")
  if version < 508
    let did_screen_syn_inits = 1
    command -nargs=+ HiLink hi link <args>
  else
    command -nargs=+ HiLink hi def link <args>
  endif

  HiLink quakeComment 	Comment
  HiLink quakeTodo 	Todo
  HiLink quakeString 	String
  HiLink quakeNumber	Number
  HiLink quakeOctal	Number
  HiLink quakeOctalZero	Number
  HiLink quakeFloat	Number
  HiLink quakeOctalError	Error
  HiLink quakeCommand	quakeCommands
  HiLink quake1Command	quakeCommands
  HiLink quake12Command	quakeCommands
  HiLink quake2Command	quakeCommands
  HiLink quake23Command	quakeCommands
  HiLink quake3Command	quakeCommands
  HiLink quakeCommands	Keyword

  delcommand HiLink
endif

" vim: set sts=2 sw=2:
