" Vim syntax file
" Language:	Fvwm{1,2} configuration file
" Maintainer:	Haakon Riiser <hakonrk@fys.uio.no>
" Last Change:	2002 Jun 2
"
" Thanks to David Necas (Yeti) for adding Fvwm 2.4 support.

" For version 5.x: Clear all syntax items
" For version 6.x: Quit when a syntax file was already loaded
if version < 600
    syn clear
elseif exists("b:current_syntax")
    finish
endif

" Fvwm configuration files are case insensitive
syn case ignore

" Identifiers in Fvwm can contain most characters, so we only
" include the most common ones here.
if version >= 600
    setlocal iskeyword=_,-,+,.,a-z,A-Z,48-57
else
    set iskeyword=_,-,+,.,a-z,A-Z,48-57
endif

" Read system colors from the color database (rgb.txt)
if exists("rgb_file")
    " We don't want any hit-return prompts, so we make sure that
    " &shortmess is set to `O'
    let __fvwm_oldshm = &shortmess
    set shortmess=O

    " And we set &report to a huge number, so that no hit-return prompts
    " will be given
    let __fvwm_oldreport = &report
    set report=10000

    " Append the color database to the fvwm configuration, and read the
    " colors from this buffer
    let __fvwm_i = line("$") + 1
    exe "$r" rgb_file
    let __fvwm_lastline = line("$")
    while __fvwm_i <= __fvwm_lastline
	let __fvwm_s = matchstr(getline(__fvwm_i), '^\s*\d\+\s\+\d\+\s\+\d\+\s\+\h.*$')
	if __fvwm_s != ""
	    exe "syn keyword fvwmColors ".substitute(__fvwm_s, '^\s*\d\+\s\+\d\+\s\+\d\+\s\+\(\h.*\)$', '\1', "")
	endif
	let __fvwm_i = __fvwm_i + 1
    endwhile

    " Remove the appended data
    undo

    " Goto first line again
    1

    " and restore the old values of the variables
    let &shortmess = __fvwm_oldshm
    let &report = __fvwm_oldreport
    unlet __fvwm_i __fvwm_s __fvwm_lastline __fvwm_oldshm __fvwm_oldreport
endif
" done reading colors

syn match   fvwmWhitespace	"\s\+" contained
syn match   fvwmEnvVar		"\$\w\+"
syn match   fvwmModConf		"^\s*\*\a\+" contains=fvwmWhitespace
syn match   fvwmString		'".\{-}"'
syn match   fvwmRGBValue	"#\x\{3}"
syn match   fvwmRGBValue	"#\x\{6}"
syn match   fvwmRGBValue	"#\x\{9}"
syn match   fvwmRGBValue	"#\x\{12}"
syn match   fvwmRGBValue	"rgb:\x\{1,4}/\x\{1,4}/\x\{1,4}"
syn match   fvwmPath		"\<IconPath\s.*$"lc=8 contains=fvwmEnvVar
syn match   fvwmPath		"\<ModulePath\s.*$"lc=10 contains=fvwmEnvVar
syn match   fvwmPath		"\<PixmapPath\s.*$"lc=10 contains=fvwmEnvVar
syn match   fvwmModule		"\<Module\s\+\w\+"he=s+6
syn match   fvwmKey		"\<Key\s\+\w\+"he=s+3
syn keyword fvwmExec		Exec
syn match   fvwmComment		"^#.*$"

if (exists("b:fvwm_version") && b:fvwm_version == 1) || (exists("use_fvwm_1") && use_fvwm_1)
    syn match  fvwmEnvVar	"\$(\w\+)"
    syn region fvwmStyle	matchgroup=fvwmFunction start="^\s*Style\>"hs=e-5 end="$" oneline keepend contains=fvwmString,fvwmKeyword,fvwmWhiteSpace

    syn keyword fvwmFunction	AppsBackingStore AutoRaise BackingStore
    syn keyword fvwmFunction	Beep BoundaryWidth ButtonStyle
    syn keyword fvwmFunction	CenterOnCirculate CirculateDown
    syn keyword fvwmFunction	CirculateHit CirculateSkip
    syn keyword fvwmFunction	CirculateSkipIcons CirculateUp
    syn keyword fvwmFunction	ClickTime ClickToFocus Close Cursor
    syn keyword fvwmFunction	CursorMove DecorateTransients Delete
    syn keyword fvwmFunction	Desk DeskTopScale DeskTopSize Destroy
    syn keyword fvwmFunction	DontMoveOff EdgeResistance EdgeScroll
    syn keyword fvwmFunction	EndFunction EndMenu EndPopup Focus
    syn keyword fvwmFunction	Font Function GotoPage HiBackColor
    syn keyword fvwmFunction	HiForeColor Icon IconBox IconFont
    syn keyword fvwmFunction	Iconify IconPath Key Lenience Lower
    syn keyword fvwmFunction	Maximize MenuBackColor MenuForeColor
    syn keyword fvwmFunction	MenuStippleColor Module ModulePath Mouse
    syn keyword fvwmFunction	Move MWMBorders MWMButtons MWMDecorHints
    syn keyword fvwmFunction	MWMFunctionHints MWMHintOverride MWMMenus
    syn keyword fvwmFunction	NoBorder NoBoundaryWidth Nop NoPPosition
    syn keyword fvwmFunction	NoTitle OpaqueMove OpaqueResize Pager
    syn keyword fvwmFunction	PagerBackColor PagerFont PagerForeColor
    syn keyword fvwmFunction	PagingDefault PixmapPath Popup Quit Raise
    syn keyword fvwmFunction	RaiseLower RandomPlacement Refresh Resize
    syn keyword fvwmFunction	Restart SaveUnders Scroll SloppyFocus
    syn keyword fvwmFunction	SmartPlacement StartsOnDesk StaysOnTop
    syn keyword fvwmFunction	StdBackColor StdForeColor Stick Sticky
    syn keyword fvwmFunction	StickyBackColor StickyForeColor
    syn keyword fvwmFunction	StickyIcons StubbornIconPlacement
    syn keyword fvwmFunction	StubbornIcons StubbornPlacement
    syn keyword fvwmFunction	SuppressIcons Title TogglePage Wait Warp
    syn keyword fvwmFunction	WindowFont WindowList WindowListSkip
    syn keyword fvwmFunction	WindowsDesk WindowShade XORvalue

    " These keywords are only used after the "Style" command.  To avoid
    " name collision with several commands, they are contained.
    syn keyword fvwmKeyword	BackColor BorderWidth BoundaryWidth contained
    syn keyword fvwmKeyword	Button CirculateHit CirculateSkip Color contained
    syn keyword fvwmKeyword	DoubleClick ForeColor Handles HandleWidth contained
    syn keyword fvwmKeyword	Icon IconTitle NoBorder NoBoundaryWidth contained
    syn keyword fvwmKeyword	NoButton NoHandles NoIcon NoIconTitle contained
    syn keyword fvwmKeyword	NoTitle Slippery StartIconic StartNormal contained
    syn keyword fvwmKeyword	StartsAnyWhere StartsOnDesk StaysOnTop contained
    syn keyword fvwmKeyword	StaysPut Sticky Title WindowListHit contained
    syn keyword fvwmKeyword	WindowListSkip contained
elseif (exists("b:fvwm_version") && b:fvwm_version == 2) || (exists("use_fvwm_2") && use_fvwm_2)
    syn match   fvwmEnvVar	"\${\w\+}"
    syn match   fvwmEnvVar	"\$\[[^]]\+\]"
    syn match   fvwmEnvVar	"\$[$0-9*]"
    syn match   fvwmDef		'^\s*+\s*".\{-}"' contains=fvwmMenuString,fvwmWhitespace
    syn match   fvwmIcon	'%.\{-}%' contained
    syn match   fvwmIcon	'\*.\{-}\*' contained
    syn match   fvwmMenuString	'".\{-}"' contains=fvwmIcon,fvwmShortcutKey contained
    syn match   fvwmShortcutKey	"&." contained
    syn match   fvwmModule	"\<KillModule\s\+\w\+"he=s+10 contains=fvwmModuleName
    syn match   fvwmModule	"\<SendToModule\s\+\w\+"he=s+12 contains=fvwmModuleName
    syn match   fvwmModule	"\<DestroyModuleConfig\s\+\w\+"he=s+19 contains=fvwmModuleName

    syn keyword fvwmFunction	AddButtonStyle AddTitleStyle AddToDecor AddToFunc
    syn keyword fvwmFunction	AddToMenu AnimatedMove Beep BorderStyle BugOpts
    syn keyword fvwmFunction	BusyCursor ButtonState ButtonStyle ChangeDecor
    syn keyword fvwmFunction	ChangeMenuStyle ClickTime Close ColorLimit
    syn keyword fvwmFunction	ColormapFocus CopyMenuStyle Current CursorMove
    syn keyword fvwmFunction	CursorStyle DefaultColors DefaultColorset
    syn keyword fvwmFunction	DefaultFont DefaultIcon DefaultLayers Delete Desk
    syn keyword fvwmFunction	DeskTopSize Destroy DestroyDecor DestroyFunc
    syn keyword fvwmFunction	DestroyMenu DestroyMenuStyle Direction Echo
    syn keyword fvwmFunction	EdgeResistance EdgeScroll EdgeThickness Emulate
    syn keyword fvwmFunction	EscapeFunc Exec ExecUseShell ExitFunction
    syn keyword fvwmFunction	FakeClick FlipFocus Focus Function GlobalOpts
    syn keyword fvwmFunction	GnomeButton GotoDesk GotoDeskAndPage GotoPage
    syn keyword fvwmFunction	HideGeometryWindow HilightColor HilightColorset
    syn keyword fvwmFunction	IconFont IconPath Iconify IgnoreModifiers
    syn keyword fvwmFunction	ImagePath Key Layer Lower Maximize Menu MenuStyle
    syn keyword fvwmFunction	ModulePath ModuleSynchronous ModuleTimeout
    syn keyword fvwmFunction	Mouse Move MoveThreshold MoveToDesk MoveToPage
    syn keyword fvwmFunction	MoveToScreen Next None Nop OpaqueMoveSize
    syn keyword fvwmFunction	PipeRead PixmapPath PlaceAgain PointerKey
    syn keyword fvwmFunction	Popup Prev Quit QuitScreen QuitSession Raise
    syn keyword fvwmFunction	RaiseLower Read Recapture RecaptureWindow
    syn keyword fvwmFunction	Refresh RefreshWindow Resize ResizeMove
    syn keyword fvwmFunction	Restart SaveQuitSession SaveSession Scroll
    syn keyword fvwmFunction	SetAnimation SetEnv SetMenuDelay SetMenuStyle
    syn keyword fvwmFunction	Silent SnapAttraction SnapGrid Stick Stroke
    syn keyword fvwmFunction	StrokeFunc Style Title TitleStyle UnsetEnv
    syn keyword fvwmFunction	UpdateDecor UpdateStyles Wait WarpToWindow
    syn keyword fvwmFunction	WindowFont WindowId WindowList WindowShade
    syn keyword fvwmFunction	WindowShadeAnimate WindowsDesk Xinerama
    syn keyword fvwmFunction	XineramaPrimaryScreen XineramaSls XineramaSlsSize
    syn keyword fvwmFunction	XorPixmap XorValue

    syn keyword fvwmKeyword	Active ActiveColorset ActiveDown
    syn keyword fvwmKeyword	ActiveFore ActiveForeOff ActivePlacement
    syn keyword fvwmKeyword	ActivePlacementHonorsStartsOnPage
    syn keyword fvwmKeyword	ActivePlacementIgnoresStartsOnPage ActiveUp All
    syn keyword fvwmKeyword	AllowRestack Alphabetic Anim Animated Animation
    syn keyword fvwmKeyword	AnimationOff AutomaticHotkeys AutomaticHotkeysOff
    syn keyword fvwmKeyword	BGradient BackColor Background BackingStore
    syn keyword fvwmKeyword	BackingStoreOff BorderColorset BorderWidth
    syn keyword fvwmKeyword	Bottom Button Button0 Button1 Button2 Button3
    syn keyword fvwmKeyword	Button4 Button5 Button6 Button7 Button8
    syn keyword fvwmKeyword	Button9 CGradient CaptureHonorsStartsOnPage
    syn keyword fvwmKeyword	CaptureIgnoresStartsOnPage CascadePlacement
    syn keyword fvwmKeyword	Centered CirculateHit CirculateHitIcon
    syn keyword fvwmKeyword	CirculateHitShaded CirculateSkip
    syn keyword fvwmKeyword	CirculateSkipIcon CirculateSkipShaded Clear
    syn keyword fvwmKeyword	ClickToFocus ClickToFocusDoesntPassClick
    syn keyword fvwmKeyword	ClickToFocusDoesntRaise ClickToFocusPassesClick
    syn keyword fvwmKeyword	ClickToFocusPassesClickOff ClickToFocusRaises
    syn keyword fvwmKeyword	ClickToFocusRaisesOff Color Colorset Context
    syn keyword fvwmKeyword	CurrentDesk CurrentPage CurrentPageAnyDesk
    syn keyword fvwmKeyword	DGradient DecorateTransient Default
    syn keyword fvwmKeyword	DepressableBorder Desk DontLowerTransient
    syn keyword fvwmKeyword	DontRaiseTransient DontStackTransientParent
    syn keyword fvwmKeyword	DoubleClickTime Down DumbPlacement DynamicMenu
    syn keyword fvwmKeyword	DynamicPopDownAction DynamicPopUpAction
    syn keyword fvwmKeyword	East Expect FVWM FirmBorder Fixed
    syn keyword fvwmKeyword	FixedPosition Flat FlickeringMoveWorkaround
    syn keyword fvwmKeyword	FlickeringQtDialogsWorkaround FocusFollowsMouse
    syn keyword fvwmKeyword	FollowsFocus FollowsMouse Font ForeColor
    syn keyword fvwmKeyword	Foreground Function Fvwm FvwmBorder
    syn keyword fvwmKeyword	FvwmButtons GNOMEIgnoreHints GNOMEUseHints
    syn keyword fvwmKeyword	GrabFocus GrabFocusOff GrabFocusTransient
    syn keyword fvwmKeyword	GrabFocusTransientOff Greyed GreyedColorset
    syn keyword fvwmKeyword	HGradient HandleWidth Handles Height
    syn keyword fvwmKeyword	HiddenHandles Hilight3DOff Hilight3DThick
    syn keyword fvwmKeyword	Hilight3DThickness Hilight3DThin HilightBack
    syn keyword fvwmKeyword	HilightBackOff HilightBorderColorset
    syn keyword fvwmKeyword	HilightColorset HilightFore HintOverride
    syn keyword fvwmKeyword	HoldSubmenus Icon IconBox IconFill IconFont
    syn keyword fvwmKeyword	IconGrid IconOverride IconTitle Iconic
    syn keyword fvwmKeyword	IconifyWindowGroups IconifyWindowGroupsOff
    syn keyword fvwmKeyword	Icons IgnoreRestack Inactive Interior Item
    syn keyword fvwmKeyword	ItemFormat KeepWindowGroupsOnDesk Layer Left
    syn keyword fvwmKeyword	LeftJustified Lenience LowerTransient MWM
    syn keyword fvwmKeyword	MWMBorder MWMButtons MWMDecor MWMDecorMax
    syn keyword fvwmKeyword	MWMDecorMenu MWMDecorMin MWMFunctions
    syn keyword fvwmKeyword	ManualPlacement ManualPlacementHonorsStartsOnPage
    syn keyword fvwmKeyword	ManualPlacementIgnoresStartsOnPage MaxWindowSize
    syn keyword fvwmKeyword	Maximized Menu MenuColorset MenuFace
    syn keyword fvwmKeyword	MinOverlapPercentPlacement MinOverlapPlacement
    syn keyword fvwmKeyword	MiniIcon MixedVisualWorkaround ModalityIsEvil
    syn keyword fvwmKeyword	ModuleSynchronous Mouse MouseFocus
    syn keyword fvwmKeyword	MouseFocusClickDoesntRaise MouseFocusClickRaises
    syn keyword fvwmKeyword	MouseFocusClickRaisesOff Move Mwm MwmBorder
    syn keyword fvwmKeyword	MwmButtons MwmDecor MwmFunctions NakedTransient
    syn keyword fvwmKeyword	Never NeverFocus NoActiveIconOverride NoButton
    syn keyword fvwmKeyword	NoDecorHint NoDeskSort NoFuncHint NoGeometry
    syn keyword fvwmKeyword	NoGeometryWithInfo NoHandles NoHotkeys NoIcon
    syn keyword fvwmKeyword	NoIconOverride NoIconPosition NoIconTitle
    syn keyword fvwmKeyword	NoIcons NoInset NoLenience NoNormal
    syn keyword fvwmKeyword	NoOLDecor NoOnBottom NoOnTop NoOverride
    syn keyword fvwmKeyword	NoPPosition NoResizeOverride NoSticky
    syn keyword fvwmKeyword	NoStipledTitles NoTitle NoTransientPPosition
    syn keyword fvwmKeyword	NoTransientUSPosition NoUSPosition
    syn keyword fvwmKeyword	NoWarp Normal North Northeast Northwest
    syn keyword fvwmKeyword	NotAlphabetic OLDecor OnBottom OnTop Once
    syn keyword fvwmKeyword	OnlyIcons OnlyListSkip OnlyNormal OnlyOnBottom
    syn keyword fvwmKeyword	OnlyOnTop OnlySticky Opacity ParentalRelativity
    syn keyword fvwmKeyword	Pixmap PopdownDelayed PopdownDelay PopupDelay
    syn keyword fvwmKeyword	PopupAsRootMenu PopupAsSubmenu PopdownImmediately
    syn keyword fvwmKeyword	PopupDelayed PopupImmediately PopupOffset
    syn keyword fvwmKeyword	Quiet RGradient RaiseOverNativeWindows
    syn keyword fvwmKeyword	RaiseOverUnmanaged RaiseTransient
    syn keyword fvwmKeyword	Raised Read RecaptureHonorsStartsOnPage
    syn keyword fvwmKeyword	RecaptureIgnoresStartsOnPage Rectangle
    syn keyword fvwmKeyword	RemoveSubmenus Reset Resize ResizeHintOverride
    syn keyword fvwmKeyword	ResizeOpaque ResizeOutline ReverseOrder
    syn keyword fvwmKeyword	Right RightJustified Root SGradient SameType
    syn keyword fvwmKeyword	SaveUnder SaveUnderOff ScatterWindowGroups
    syn keyword fvwmKeyword	Screen SelectInPlace SelectOnRelease
    syn keyword fvwmKeyword	SelectWarp SeparatorsLong SeparatorsShort
    syn keyword fvwmKeyword	ShowMapping SideColor SidePic Simple
    syn keyword fvwmKeyword	SkipMapping Slippery SlipperyIcon SloppyFocus
    syn keyword fvwmKeyword	SmartPlacement SmartPlacementIsNormal
    syn keyword fvwmKeyword	SmartPlacementIsReallySmart Solid South
    syn keyword fvwmKeyword	Southeast Southwest StackTransientParent
    syn keyword fvwmKeyword	StartIconic StartNormal StartsAnyWhere
    syn keyword fvwmKeyword	StartsLowered StartsOnDesk StartsOnPage
    syn keyword fvwmKeyword	StartsOnPageIgnoresTransients
    syn keyword fvwmKeyword	StartsOnPageIncludesTransients StartsOnScreen
    syn keyword fvwmKeyword	StartsRaised StaysOnBottom StaysOnTop StaysPut
    syn keyword fvwmKeyword	Sticky StickyIcon StipledTitles StippledTitle
    syn keyword fvwmKeyword	StippledTitleOff SubmenusLeft SubmenusRight Sunk
    syn keyword fvwmKeyword	This TileCascadePlacement TileManualPlacement
    syn keyword fvwmKeyword	TiledPixmap Timeout Title TitleAtBottom
    syn keyword fvwmKeyword	TitleAtTop TitleUnderlines0 TitleUnderlines1
    syn keyword fvwmKeyword	TitleUnderlines2 TitleWarp TitleWarpOff Top
    syn keyword fvwmKeyword	Transient TrianglesRelief TrianglesSolid
    syn keyword fvwmKeyword	Up UseBorderStyle UseDecor UseIconName
    syn keyword fvwmKeyword	UseIconPosition UseListSkip UsePPosition
    syn keyword fvwmKeyword	UseStyle UseTitleStyle UseTransientPPosition
    syn keyword fvwmKeyword	UseTransientUSPosition UseUSPosition VGradient
    syn keyword fvwmKeyword	VariablePosition Vector VerticalItemSpacing
    syn keyword fvwmKeyword	VerticalTitleSpacing WIN Wait Warp WarpTitle
    syn keyword fvwmKeyword	West Win Window WindowListHit WindowListSkip
    syn keyword fvwmKeyword	WindowShadeScrolls WindowShadeShrinks
    syn keyword fvwmKeyword	WindowShadeSteps Windows XineramaRoot YGradient
    syn keyword fvwmKeyword	bottomright default pointer prev quiet
    syn keyword fvwmKeyword	True False Toggle

    syn keyword fvwmConditionName	AcceptsFocus CurrentDesk CurrentGlobalPage
    syn keyword fvwmConditionName	CurrentGlobalPageAnyDesk CurrentPage
    syn keyword fvwmConditionName	CurrentPageAnyDesk CurrentScreen Iconic Layer
    syn keyword fvwmConditionName	Maximized PlacedByButton3 PlacedByFvwm Raised
    syn keyword fvwmConditionName	Shaded Sticky Transient Visible

    syn keyword fvwmContextName	BOTTOM BOTTOM_EDGE BOTTOM_LEFT BOTTOM_RIGHT
    syn keyword fvwmContextName	DEFAULT DESTROY LEFT LEFT_EDGE MENU MOVE
    syn keyword fvwmContextName	RESIZE RIGHT RIGHT_EDGE ROOT SELECT STROKE SYS
    syn keyword fvwmContextName	TITLE TOP TOP_EDGE TOP_LEFT TOP_RIGHT WAIT
    syn keyword fvwmContextName	POSITION

    syn keyword fvwmFunctionName	contained FvwmAnimate FvwmAudio FvwmAuto
    syn keyword fvwmFunctionName	contained FvwmBacker FvwmBanner FvwmButtons
    syn keyword fvwmFunctionName	contained FvwmCascade FvwmCommandS
    syn keyword fvwmFunctionName	contained FvwmConsole FvwmConsoleC FvwmCpp
    syn keyword fvwmFunctionName	contained FvwmDebug FvwmDragWell FvwmEvent
    syn keyword fvwmFunctionName	contained FvwmForm FvwmGtk FvwmIconBox
    syn keyword fvwmFunctionName	contained FvwmIconMan FvwmIdent FvwmM4
    syn keyword fvwmFunctionName	contained FvwmPager FvwmRearrange FvwmSave
    syn keyword fvwmFunctionName	contained FvwmSaveDesk FvwmScript FvwmScroll
    syn keyword fvwmFunctionName	contained FvwmTalk FvwmTaskBar FvwmTheme
    syn keyword fvwmFunctionName	contained FvwmTile FvwmWharf FvwmWinList

    syn keyword fvwmFunctionName	StartFunction InitFunction RestartFunction
    syn keyword fvwmFunctionName	ExitFunction SessionInitFunction
    syn keyword fvwmFunctionName	SessionRestartFunction SessionExitFunction
    syn keyword fvwmFunctionName	MissingSubmenuFunction
endif

if version >= 508 || !exists("did_fvwm_syntax_inits")
    if version < 508
	let did_fvwm_syntax_inits = 1
	command -nargs=+ HiLink hi link <args>
    else
	command -nargs=+ HiLink hi def link <args>
    endif

    HiLink fvwmComment		Comment
    HiLink fvwmEnvVar		Macro
    HiLink fvwmExec		Function
    HiLink fvwmFunction		Function
    HiLink fvwmFunctionName	Special
    HiLink fvwmContextName	Function
    HiLink fvwmConditionName	Function
    HiLink fvwmIcon		Comment
    HiLink fvwmKey		Function
    HiLink fvwmKeyword		Keyword
    HiLink fvwmMenuString	String
    HiLink fvwmModConf		Macro
    HiLink fvwmModule		Function
    HiLink fvwmModuleName	Special
    HiLink fvwmRGBValue		Type
    HiLink fvwmShortcutKey	SpecialChar
    HiLink fvwmString		String

    if exists("rgb_file")
	HiLink fvwmColors	Type
    endif

    delcommand HiLink
endif

let b:current_syntax = "fvwm"
" vim: sts=4 sw=4 ts=8
