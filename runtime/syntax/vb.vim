" Vim syntax file
" Language:	Visual Basic
" Maintainer:	Tim Chase <vb.vim@tim.thechases.com>
" Former Maintainer:	Robert M. Cortopassi <cortopar@mindspring.com>
"	(tried multiple times to contact, but email bounced)
" Last Change:	2004 May 25
"   2004 May 30  Added a few keywords

" This was thrown together after seeing numerous requests on the
" VIM and VIM-DEV mailing lists.  It is by no means complete.
" Send comments, suggestions and requests to the maintainer.

" For version 5.x: Clear all syntax items
" For version 6.x: Quit when a syntax file was already loaded
if version < 600
	syntax clear
elseif exists("b:current_syntax")
	finish
endif

" VB is case insensitive
syn case ignore

syn keyword vbStatement Alias AppActivate As Base Beep Call Case
syn keyword vbStatement ChDir ChDrive Const Declare DefBool DefByte
syn keyword vbStatement DefCur DefDate DefDbl DefDec DefInt
syn keyword vbStatement DefLng DefObj DefSng DefStr Deftype
syn keyword vbStatement DefVar DeleteSetting Dim Do Each Else
syn keyword vbStatement ElseIf End Enum Erase Event Exit Explicit
syn keyword vbStatement FileCopy For ForEach Function Get GoSub
syn keyword vbStatement GoTo If Implements Kill Let Lib LineInput
syn keyword vbStatement Lock Loop LSet MkDir Name Next OnError On
syn keyword vbStatement Option Preserve Private Property Public Put
syn keyword vbStatement RaiseEvent Randomize ReDim Reset Resume
syn keyword vbStatement Return RmDir RSet SavePicture SaveSetting
syn keyword vbStatement SendKeys Select SetAttr Static Step Sub
syn keyword vbStatement Then Type Unlock Until Wend While Width
syn keyword vbStatement With Write

syn keyword vbFunction Abs Array Asc AscB AscW Atn Avg CBool
syn keyword vbFunction CByte CCur CDate CDbl Cdec Choose Chr
syn keyword vbFunction ChrB ChrW CInt CLng Command Cos Count
syn keyword vbFunction CreateObject CSng CStr CurDir CVar
syn keyword vbFunction CVDate CVErr DateAdd DateDiff DatePart
syn keyword vbFunction DateSerial DateValue Day DDB Dir
syn keyword vbFunction DoEvents Environ EOF Error Exp FileAttr
syn keyword vbFunction FileDateTime FileLen Fix Format FreeFile
syn keyword vbFunction FV GetAllStrings GetAttr
syn keyword vbFunction GetAutoServerSettings GetObject
syn keyword vbFunction GetSetting Hex Hour IIf IMEStatus Input
syn keyword vbFunction InputB InputBox InStr InstB Int IPmt
syn keyword vbFunction IsArray IsDate IsEmpty IsError IsMissing
syn keyword vbFunction IsNull IsNumeric IsObject LBound LCase
syn keyword vbFunction Left LeftB Len LenB LoadPicture Loc LOF
syn keyword vbFunction Log LTrim Max Mid MidB Min Minute MIRR
syn keyword vbFunction Month MsgBox Now NPer NPV Oct Partition
syn keyword vbFunction Pmt PPmt PV QBColor Rate RGB Right
syn keyword vbFunction RightB Rnd RTrim Second Seek Sgn Shell
syn keyword vbFunction Sin SLN Space Spc Sqr StDev StDevP Str
syn keyword vbFunction StrComp StrConv String Switch Sum SYD
syn keyword vbFunction Tab Tan Time Timer TimeSerial TimeValue
syn keyword vbFunction Trim TypeName UBound UCase Val Var VarP
syn keyword vbFunction VarType Weekday Year

syn keyword vbMethods Accept Activate Add AddCustom AddFile
syn keyword vbMethods AddFromFile AddFromTemplate AddItem
syn keyword vbMethods AddNew AddToAddInToolbar
syn keyword vbMethods AddToolboxProgID Append AppendChunk
syn keyword vbMethods Arrange Assert AsyncRead BatchUpdate
syn keyword vbMethods BeginTrans Bind Cancel CancelAsyncRead
syn keyword vbMethods CancelBatch CancelUpdate
syn keyword vbMethods CanPropertyChange CaptureImage CellText
syn keyword vbMethods CellValue Circle Clear ClearFields
syn keyword vbMethods ClearSel ClearSelCols Clone Close Cls
syn keyword vbMethods ColContaining ColumnSize CommitTrans
syn keyword vbMethods CompactDatabase Compose Connect Copy
syn keyword vbMethods CopyQueryDef CreateDatabase
syn keyword vbMethods CreateDragImage CreateEmbed CreateField
syn keyword vbMethods CreateGroup CreateIndex CreateLink
syn keyword vbMethods CreatePreparedStatement CreatePropery
syn keyword vbMethods CreateQuery CreateQueryDef
syn keyword vbMethods CreateRelation CreateTableDef CreateUser
syn keyword vbMethods CreateWorkspace Customize Delete
syn keyword vbMethods DeleteColumnLabels DeleteColumns
syn keyword vbMethods DeleteRowLabels DeleteRows DoVerb Drag
syn keyword vbMethods Draw Edit EditCopy EditPaste EndDoc
syn keyword vbMethods EnsureVisible EstablishConnection
syn keyword vbMethods Execute ExtractIcon Fetch FetchVerbs
syn keyword vbMethods Files FillCache Find FindFirst FindItem
syn keyword vbMethods FindLast FindNext FindPrevious Forward
syn keyword vbMethods GetBookmark GetChunk GetClipString
syn keyword vbMethods GetData GetFirstVisible GetFormat
syn keyword vbMethods GetHeader GetLineFromChar GetNumTicks
syn keyword vbMethods GetRows GetSelectedPart GetText
syn keyword vbMethods GetVisibleCount GoBack GoForward Hide
syn keyword vbMethods HitTest HoldFields Idle InitializeLabels
syn keyword vbMethods InsertColumnLabels InsertColumns
syn keyword vbMethods InsertObjDlg InsertRowLabels InsertRows
syn keyword vbMethods Item KillDoc Layout Line LinkExecute
syn keyword vbMethods LinkPoke LinkRequest LinkSend Listen
syn keyword vbMethods LoadFile LoadResData LoadResPicture
syn keyword vbMethods LoadResString LogEvent MakeCompileFile
syn keyword vbMethods MakeReplica MoreResults Move MoveData
syn keyword vbMethods MoveFirst MoveLast MoveNext MovePrevious
syn keyword vbMethods NavigateTo NewPage NewPassword
syn keyword vbMethods NextRecordset OLEDrag OnAddinsUpdate
syn keyword vbMethods OnConnection OnDisconnection
syn keyword vbMethods OnStartupComplete Open OpenConnection
syn keyword vbMethods OpenDatabase OpenQueryDef OpenRecordset
syn keyword vbMethods OpenResultset OpenURL Overlay
syn keyword vbMethods PaintPicture Paste PastSpecialDlg
syn keyword vbMethods PeekData Play Point PopulatePartial
syn keyword vbMethods PopupMenu Print PrintForm
syn keyword vbMethods PropertyChanged PSet Quit Raise
syn keyword vbMethods RandomDataFill RandomFillColumns
syn keyword vbMethods RandomFillRows rdoCreateEnvironment
syn keyword vbMethods rdoRegisterDataSource ReadFromFile
syn keyword vbMethods ReadProperty Rebind ReFill Refresh
syn keyword vbMethods RefreshLink RegisterDatabase Reload
syn keyword vbMethods Remove RemoveAddInFromToolbar RemoveItem
syn keyword vbMethods Render RepairDatabase Reply ReplyAll
syn keyword vbMethods Requery ResetCustom ResetCustomLabel
syn keyword vbMethods ResolveName RestoreToolbar Resync
syn keyword vbMethods Rollback RollbackTrans RowBookmark
syn keyword vbMethods RowContaining RowTop Save SaveAs
syn keyword vbMethods SaveFile SaveToFile SaveToolbar
syn keyword vbMethods SaveToOle1File Scale ScaleX ScaleY
syn keyword vbMethods Scroll SelectAll SelectPart SelPrint
syn keyword vbMethods Send SendData Set SetAutoServerSettings
syn keyword vbMethods SetData SetFocus SetOption SetSize
syn keyword vbMethods SetText SetViewport Show ShowColor
syn keyword vbMethods ShowFont ShowHelp ShowOpen ShowPrinter
syn keyword vbMethods ShowSave ShowWhatsThis SignOff SignOn
syn keyword vbMethods Size Span SplitContaining StartLabelEdit
syn keyword vbMethods StartLogging Stop Synchronize TextHeight
syn keyword vbMethods TextWidth ToDefaults TwipsToChartPart
syn keyword vbMethods TypeByChartType Update UpdateControls
syn keyword vbMethods UpdateRecord UpdateRow Upto
syn keyword vbMethods WhatsThisMode WriteProperty ZOrder

syn keyword vbEvents AccessKeyPress AfterAddFile
syn keyword vbEvents AfterChangeFileName AfterCloseFile
syn keyword vbEvents AfterColEdit AfterColUpdate AfterDelete
syn keyword vbEvents AfterInsert AfterLabelEdit
syn keyword vbEvents AfterRemoveFile AfterUpdate
syn keyword vbEvents AfterWriteFile AmbienChanged
syn keyword vbEvents ApplyChanges Associate AsyncReadComplete
syn keyword vbEvents AxisActivated AxisLabelActivated
syn keyword vbEvents AxisLabelSelected AxisLabelUpdated
syn keyword vbEvents AxisSelected AxisTitleActivated
syn keyword vbEvents AxisTitleSelected AxisTitleUpdated
syn keyword vbEvents AxisUpdated BeforeClick BeforeColEdit
syn keyword vbEvents BeforeColUpdate BeforeConnect
syn keyword vbEvents BeforeDelete BeforeInsert
syn keyword vbEvents BeforeLabelEdit BeforeLoadFile
syn keyword vbEvents BeforeUpdate ButtonClick ButtonCompleted
syn keyword vbEvents ButtonGotFocus ButtonLostFocus Change
syn keyword vbEvents ChartActivated ChartSelected
syn keyword vbEvents ChartUpdated Click ColEdit Collapse
syn keyword vbEvents ColResize ColumnClick Compare
syn keyword vbEvents ConfigChageCancelled ConfigChanged
syn keyword vbEvents ConnectionRequest DataArrival
syn keyword vbEvents DataChanged DataUpdated DblClick
syn keyword vbEvents Deactivate DeviceArrival
syn keyword vbEvents DeviceOtherEvent DeviceQueryRemove
syn keyword vbEvents DeviceQueryRemoveFailed
syn keyword vbEvents DeviceRemoveComplete DeviceRemovePending
syn keyword vbEvents DevModeChange Disconnect DisplayChanged
syn keyword vbEvents Dissociate DoGetNewFileName Done
syn keyword vbEvents DonePainting DownClick DragDrop DragOver
syn keyword vbEvents DropDown EditProperty EnterCell
syn keyword vbEvents EnterFocus ExitFocus Expand
syn keyword vbEvents FootnoteActivated FootnoteSelected
syn keyword vbEvents FootnoteUpdated GotFocus HeadClick
syn keyword vbEvents InfoMessage Initialize IniProperties
syn keyword vbEvents ItemActivated ItemAdded ItemCheck
syn keyword vbEvents ItemClick ItemReloaded ItemRemoved
syn keyword vbEvents ItemRenamed ItemSeletected KeyDown
syn keyword vbEvents KeyPress KeyUp LeaveCell LegendActivated
syn keyword vbEvents LegendSelected LegendUpdated LinkClose
syn keyword vbEvents LinkError LinkNotify LinkOpen Load
syn keyword vbEvents LostFocus MouseDown MouseMove MouseUp
syn keyword vbEvents NodeClick ObjectMove OLECompleteDrag
syn keyword vbEvents OLEDragDrop OLEDragOver OLEGiveFeedback
syn keyword vbEvents OLESetData OLEStartDrag OnAddNew OnComm
syn keyword vbEvents Paint PanelClick PanelDblClick
syn keyword vbEvents PathChange PatternChange PlotActivated
syn keyword vbEvents PlotSelected PlotUpdated PointActivated
syn keyword vbEvents PointLabelActivated PointLabelSelected
syn keyword vbEvents PointLabelUpdated PointSelected
syn keyword vbEvents PointUpdated PowerQuerySuspend
syn keyword vbEvents PowerResume PowerStatusChanged
syn keyword vbEvents PowerSuspend QueryChangeConfig
syn keyword vbEvents QueryComplete QueryCompleted
syn keyword vbEvents QueryTimeout QueryUnload ReadProperties
syn keyword vbEvents Reposition RequestChangeFileName
syn keyword vbEvents RequestWriteFile Resize ResultsChanged
syn keyword vbEvents RowColChange RowCurrencyChange RowResize
syn keyword vbEvents RowStatusChanged SelChange
syn keyword vbEvents SelectionChanged SendComplete
syn keyword vbEvents SendProgress SeriesActivated
syn keyword vbEvents SeriesSelected SeriesUpdated
syn keyword vbEvents SettingChanged SplitChange StateChanged
syn keyword vbEvents StatusUpdate SysColorsChanged Terminate
syn keyword vbEvents TimeChanged TitleActivated TitleSelected
syn keyword vbEvents TitleActivated UnboundAddData
syn keyword vbEvents UnboundDeleteRow
syn keyword vbEvents UnboundGetRelativeBookmark
syn keyword vbEvents UnboundReadData UnboundWriteData Unload
syn keyword vbEvents UpClick Updated Validate ValidationError
syn keyword vbEvents WillAssociate WillChangeData
syn keyword vbEvents WillDissociate WillExecute
syn keyword vbEvents WillUpdateRows WriteProperties

syn keyword vbTypes Boolean Byte Currency Date Decimal
syn keyword vbTypes Double Empty Integer Long Single String

syn match vbOperator "[()+.,\-/*=&]"
syn match vbOperator "[<>]=\="
syn match vbOperator "<>"
syn match vbOperator "\s\+_$"
syn keyword vbOperator And Or Not Xor Mod In Is Imp Eqv
syn keyword vbOperator To ByVal ByRef
syn keyword vbConst True False Null Nothing

syn keyword vbTodo contained TODO

"integer number, or floating point number without a dot.
syn match vbNumber "\<\d\+\>"
"floating point number, with dot
syn match vbNumber "\<\d\+\.\d*\>"
"floating point number, starting with a dot
syn match vbNumber "\.\d\+\>"

" String and Character contstants
syn region vbString start=+"+ end=+"+
syn region vbComment start="\<REM\>" end="$" contains=vbTodo
syn region vbComment start="'" end="$" contains=vbTodo
syn region vbLineNumber	start="^\d" end="\s"
syn match vbTypeSpecifier "[a-zA-Z0-9][\$%&!#]"ms=s+1

" Define the default highlighting.
" For version 5.7 and earlier: only when not done already
" For version 5.8 and later: only when an item doesn't have highlighting yet
if version >= 508 || !exists("did_vb_syntax_inits")
	if version < 508
		let did_vb_syntax_inits = 1
		command -nargs=+ HiLink hi link <args>
	else
		command -nargs=+ HiLink hi def link <args>
	endif

	HiLink vbLineNumber	Comment
	HiLink vbNumber		Number
	HiLink vbConst		Constant
	HiLink vbError		Error
	HiLink vbStatement	Statement
	HiLink vbString		String
	HiLink vbComment	Comment
	HiLink vbTodo		Todo
	HiLink vbFunction	Identifier
	HiLink vbMethods	PreProc
	HiLink vbEvents		Special
	HiLink vbTypeSpecifier	Type
	HiLink vbTypes		Type
	HiLink vbOperator	Operator

	delcommand HiLink
endif

let b:current_syntax = "vb"

" vim: ts=8
