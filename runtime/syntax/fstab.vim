" Vim syntax file
" Language: fstab file
" Maintainer: David Ne\v{c}as (Yeti) <yeti@physics.muni.cz>
" Original Maintainer: Radu Dineiu <littledragon@altern.org>
" License: This file can be redistribued and/or modified under the same terms
"   as Vim itself.
" URL: http://trific.ath.cx/Ftp/vim/syntax/fstab.vim
" Last Change: 2006-04-16

" Options: let fstab_unknown_fs_errors = 1 to highlight unknown filesystems
"          as errors

if version < 600
	syntax clear
elseif exists("b:current_syntax")
	finish
endif

" General
syn cluster fsGeneralCluster contains=fsComment
syn match fsComment /\s*#.*/
syn match fsOperator /[,=]/

" Device
syn cluster fsDeviceCluster contains=fsOperator,fsDeviceKeyword,fsDeviceError
syn match fsDeviceError /\%([^a-zA-Z0-9_\/#@:]\|^\w\{-}\ze\W\)/ contained
syn keyword fsDeviceKeyword contained none proc linproc tmpfs devpts sysfs usbfs
syn keyword fsDeviceKeyword contained LABEL nextgroup=fsDeviceLabel
syn match fsDeviceLabel contained /=[^ \t]\+/hs=s+1 contains=fsOperator

" Mount Point
syn cluster fsMountPointCluster contains=fsMountPointKeyword,fsMountPointError
syn match fsMountPointError /\%([^ \ta-zA-Z0-9_\/#@]\|\s\+\zs\w\{-}\ze\s\)/ contained
syn keyword fsMountPointKeyword contained none swap

" Type
syn cluster fsTypeCluster contains=fsTypeKeyword,fsTypeUnknown
syn match fsTypeUnknown /\s\+\zs\w\+/ contained
syn keyword fsTypeKeyword contained adfs affs atfs audiofs auto autofs befs bfs cd9660 cfs cifs coda cramfs devfs devpts e2compr efs ext2 ext3 fdesc hfs hpfs iso9660 jffs jffs2 jfs kernfs linprocfs mfs minix msdos ncpfs nfs none none ntfs null nwfs ovlfs portal proc procfs qnx4 reiserfs romfs shm smbfs std subfs swap sysfs sysv tcfs tmpfs udf ufs umap umsdos union usbfs userfs vfat vs3fs vxfs wrapfs wvfs xfs zisofs

" Options
" -------
" Options: General
syn cluster fsOptionsCluster contains=fsOperator,fsOptionsGeneral,fsOptionsKeywords,fsTypeUnknown
syn match fsOptionsNumber /\d\+/
syn match fsOptionsNumberOctal /[0-8]\+/
syn match fsOptionsString /[a-zA-Z0-9_-]\+/
syn keyword fsOptionsYesNo yes no
syn cluster fsOptionsCheckCluster contains=fsOptionsExt2Check,fsOptionsFatCheck
syn keyword fsOptionsSize 512 1024 2048
syn keyword fsOptionsGeneral async atime auto bind current defaults dev devgid devmode devuid dirsync exec force fstab kudzu loop mand move noatime noauto noclusterr noclusterw nodev nodiratime noexec nomand nosuid nosymfollow nouser owner rbind rdonly remount ro rq rw suid suiddir supermount sw sync union update user[s] xx
syn match fsOptionsGeneral /_netdev/

" Options: adfs
syn match fsOptionsKeywords contained /\<\%([ug]id\|o\%(wn\|th\)mask\)=/ nextgroup=fsOptionsNumber

" Options: affs
syn match fsOptionsKeywords contained /\<\%(set[ug]id\|mode\|reserved\)=/ nextgroup=fsOptionsNumber
syn match fsOptionsKeywords contained /\<\%(prefix\|volume\|root\)=/ nextgroup=fsOptionsString
syn match fsOptionsKeywords contained /\<bs=/ nextgroup=fsOptionsSize
syn keyword fsOptionsKeywords contained protect usemp verbose

" Options: cd9660
syn keyword fsOptionsKeywords contained extatt gens norrip nostrictjoilet

" Options: devpts
" -- everything already defined

" Options: ext2
syn match fsOptionsKeywords contained /\<check=*/ nextgroup=@fsOptionsCheckCluster
syn match fsOptionsKeywords contained /\<errors=/ nextgroup=fsOptionsExt2Errors
syn match fsOptionsKeywords contained /\<\%(res[gu]id\|sb\)=/ nextgroup=fsOptionsNumber
syn keyword fsOptionsExt2Check contained none normal strict
syn keyword fsOptionsExt2Errors contained continue panic
syn match fsOptionsExt2Errors contained /\<remount-ro\>/
syn keyword fsOptionsKeywords contained acl bsddf minixdf debug grpid bsdgroups minixdf noacl nocheck nogrpid oldalloc orlov sysvgroups nouid32 nobh user_xattr nouser_xattr

" Options: ext3
syn match fsOptionsKeywords contained /\<journal=/ nextgroup=fsOptionsExt3Journal
syn match fsOptionsKeywords contained /\<data=/ nextgroup=fsOptionsExt3Data
syn match fsOptionsKeywords contained /\<commit=/ nextgroup=fsOptionsNumber
syn keyword fsOptionsExt3Journal contained update inum
syn keyword fsOptionsExt3Data contained journal ordered writeback
syn keyword fsOptionsKeywords contained noload

" Options: fat
syn match fsOptionsKeywords contained /\<blocksize=/ nextgroup=fsOptionsSize
syn match fsOptionsKeywords contained /\<\%([dfu]mask\|codepage\)=/ nextgroup=fsOptionsNumberOctal
syn match fsOptionsKeywords contained /\%(cvf_\%(format\|option\)\|iocharset\)=/ nextgroup=fsOptionsString
syn match fsOptionsKeywords contained /\<check=/ nextgroup=@fsOptionsCheckCluster
syn match fsOptionsKeywords contained /\<conv=*/ nextgroup=fsOptionsConv
syn match fsOptionsKeywords contained /\<fat=/ nextgroup=fsOptionsFatType
syn match fsOptionsKeywords contained /\<dotsOK=/ nextgroup=fsOptionsYesNo
syn keyword fsOptionsFatCheck contained r n s relaxed normal strict
syn keyword fsOptionsConv contained b t a binary text auto
syn keyword fsOptionsFatType contained 12 16 32
syn keyword fsOptionsKeywords contained quiet sys_immutable showexec dots nodots

" Options: hfs
syn match fsOptionsKeywords contained /\<\%(creator|type\)=/ nextgroup=fsOptionsString
syn match fsOptionsKeywords contained /\<\%(dir\|file\|\)_umask=/ nextgroup=fsOptionsNumberOctal
syn match fsOptionsKeywords contained /\<\%(session\|part\)=/ nextgroup=fsOptionsNumber

" Options: hpfs
syn match fsOptionsKeywords contained /\<case=/ nextgroup=fsOptionsHpfsCase
syn keyword fsOptionsHpfsCase contained lower asis

" Options: iso9660
syn match fsOptionsKeywords contained /\<map=/ nextgroup=fsOptionsIsoMap
syn match fsOptionsKeywords contained /\<block=/ nextgroup=fsOptionsSize
syn match fsOptionsKeywords contained /\<\%(session\|sbsector\)=/ nextgroup=fsOptionsNumber
syn keyword fsOptionsIsoMap contained n o a normal off acorn
syn keyword fsOptionsKeywords contained norock nojoilet unhide cruft
syn keyword fsOptionsConv contained m mtext

" Options: jfs
syn keyword fsOptionsKeywords nointegrity integrity

" Options: nfs
syn match fsOptionsKeywords contained /\<\%(rsize\|wsize\|timeo\|retrans\|acregmin\|acregmax\|acdirmin\|acdirmax\|actimeo\|retry\|port\|mountport\|mounthost\|mountprog\|mountvers\|nfsprog\|nfsvers\|namelen\)=/ nextgroup=fsOptionsString
syn keyword fsOptionsKeywords contained bg fg soft hard intr cto ac tcp udp lock nobg nofg nosoft nohard nointr noposix nocto noac notcp noudp nolock

" Options: ntfs
syn match fsOptionsKeywords contained /\<\%(posix=*\|uni_xlate=\)/ nextgroup=fsOptionsNumber
syn keyword fsOptionsKeywords contained utf8

" Options: proc
" -- everything already defined

" Options: reiserfs
syn match fsOptionsKeywords contained /\<hash=/ nextgroup=fsOptionsReiserHash
syn match fsOptionsKeywords contained /\<resize=/ nextgroup=fsOptionsNumber
syn keyword fsOptionsReiserHash contained rupasov tea r5 detect
syn keyword fsOptionsKeywords contained hashed_relocation noborder nolog notail no_unhashed_relocation replayonly

" Options: subfs
syn match fsOptionsKeywords contained /\<fs=/ nextgroup=fsOptionsString
syn keyword fsOptionsKeywords contained procuid

" Options: swap
syn match fsOptionsKeywords contained /\<pri=/ nextgroup=fsOptionsNumber

" Options: tmpfs
syn match fsOptionsKeywords contained /\<nr_\%(blocks\|inodes\)=/ nextgroup=fsOptionsNumber

" Options: udf
syn match fsOptionsKeywords contained /\<\%(anchor\|partition\|lastblock\|fileset\|rootdir\)=/ nextgroup=fsOptionsString
syn keyword fsOptionsKeywords contained unhide undelete strict novrs

" Options: ufs
syn match fsOptionsKeywords contained /\<ufstype=/ nextgroup=fsOptionsUfsType
syn match fsOptionsKeywords contained /\<onerror=/ nextgroup=fsOptionsUfsError
syn keyword fsOptionsUfsType contained old hp 44bsd sun sunx86 nextstep openstep
syn match fsOptionsUfsType contained /\<nextstep-cd\>/
syn keyword fsOptionsUfsError contained panic lock umount repair

" Options: usbfs
syn match fsOptionsKeywords contained /\<\%(dev\|bus\|list\)\%(id\|gid\)=/ nextgroup=fsOptionsNumber
syn match fsOptionsKeywords contained /\<\%(dev\|bus\|list\)mode=/ nextgroup=fsOptionsNumberOctal

" Options: vfat
syn keyword fsOptionsKeywords contained nonumtail posix utf8
syn match fsOptionsKeywords contained /shortname=/ nextgroup=fsOptionsVfatShortname
syn keyword fsOptionsVfatShortname contained lower win95 winnt mixed

" Options: xfs
syn match fsOptionsKeywords contained /\%(biosize\|logbufs\|logbsize\|logdev\|rtdev\|sunit\|swidth\)=/ nextgroup=fsOptionsString
syn keyword fsOptionsKeywords contained dmapi xdsm noalign noatime noquota norecovery osyncisdsync quota usrquota uqnoenforce grpquota gqnoenforce

" Frequency / Pass No.
syn cluster fsFreqPassCluster contains=fsFreqPassNumber,fsFreqPassError
syn match fsFreqPassError /\s\+\zs\%(\D.*\|\S.*\|\d\+\s\+[^012]\)\ze/ contained
syn match fsFreqPassNumber /\d\+\s\+[012]\s*/ contained

" Groups
syn match fsDevice /^\s*\zs.\{-1,}\s/me=e-1 nextgroup=fsMountPoint contains=@fsDeviceCluster,@fsGeneralCluster
syn match fsMountPoint /\s\+.\{-}\s/me=e-1 nextgroup=fsType contains=@fsMountPointCluster,@fsGeneralCluster contained
syn match fsType /\s\+.\{-}\s/me=e-1 nextgroup=fsOptions contains=@fsTypeCluster,@fsGeneralCluster contained
syn match fsOptions /\s\+.\{-}\s/me=e-1 nextgroup=fsFreqPass contains=@fsOptionsCluster,@fsGeneralCluster contained
syn match fsFreqPass /\s\+.\{-}$/ contains=@fsFreqPassCluster,@fsGeneralCluster contained

" Whole line comments
syn match fsCommentLine /^#.*$/

if version >= 508 || !exists("did_config_syntax_inits")
	if version < 508
		let did_config_syntax_inits = 1
		command! -nargs=+ HiLink hi link <args>
	else
		command! -nargs=+ HiLink hi def link <args>
	endif

	HiLink fsOperator Operator
	HiLink fsComment Comment
	HiLink fsCommentLine Comment

	HiLink fsTypeKeyword Type
	HiLink fsDeviceKeyword Identifier
	HiLink fsDeviceLabel String
	HiLink fsFreqPassNumber Number

	if exists('fstab_unknown_fs_errors')
		HiLink fsTypeUnknown Error
	endif
	HiLink fsDeviceError Error
	HiLink fsMountPointError Error
	HiLink fsMountPointKeyword Keyword
	HiLink fsFreqPassError Error

	HiLink fsOptionsGeneral Type
	HiLink fsOptionsKeywords Keyword
	HiLink fsOptionsNumber Number
	HiLink fsOptionsNumberOctal Number
	HiLink fsOptionsString String
	HiLink fsOptionsSize Number
	HiLink fsOptionsExt2Check String
	HiLink fsOptionsExt2Errors String
	HiLink fsOptionsExt3Journal String
	HiLink fsOptionsExt3Data String
	HiLink fsOptionsFatCheck String
	HiLink fsOptionsConv String
	HiLink fsOptionsFatType Number
	HiLink fsOptionsYesNo String
	HiLink fsOptionsHpfsCase String
	HiLink fsOptionsIsoMap String
	HiLink fsOptionsReiserHash String
	HiLink fsOptionsUfsType String
	HiLink fsOptionsUfsError String

	HiLink fsOptionsVfatShortname String

	delcommand HiLink
endif

let b:current_syntax = "fstab"

" vim: ts=8 ft=vim
