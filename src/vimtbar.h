/* vi:set ts=8 sts=4 sw=4:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *			Win16 Toolbar by Vince Negri - External Header
 *			(Based on MS Sample Code)
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 */
//
// Button Structure
//
typedef struct tagTBBUTTON
{
    int  iBitmap;	 // index into bitmap of this button's picture
    int  idCommand;	 // WM_COMMAND menu ID that this button sends
    BYTE fsState;	 // button's state
    BYTE fsStyle;	 // button's style
} TBBUTTON;

typedef TBBUTTON NEAR* PTBBUTTON;
typedef TBBUTTON FAR* LPTBBUTTON;
typedef const TBBUTTON FAR* LPCTBBUTTON;


//
// Styles
//
#define TBSTYLE_BUTTON		0x00	// this entry is button
#define TBSTYLE_SEP		0x01	// this entry is a separator
#define TBSTYLE_CHECK		0x02	// this is a check button (it stays down)
#define TBSTYLE_GROUP		0x04	// this is a check button (it stays down)
#define TBSTYLE_CHECKGROUP	(TBSTYLE_GROUP | TBSTYLE_CHECK) // this group is a member of a group radio group

//
// States
//
#define TBSTATE_CHECKED		0x01	// radio button is checked
#define TBSTATE_PRESSED		0x02	// button is being depressed (any style)
#define TBSTATE_ENABLED		0x04	// button is enabled
#define TBSTATE_HIDDEN		0x08	// button is hidden
#define TBSTATE_INDETERMINATE   0x10    // button is indeterminate
#define TBSTATE_SELECTED	0x20	// mouse hovering over button (for coolbar look)



typedef struct tagADJUSTINFO
{
    TBBUTTON tbButton;
    char szDescription[1];
} ADJUSTINFO;

typedef ADJUSTINFO NEAR* PADJUSTINFO;
typedef ADJUSTINFO FAR* LPADJUSTINFO;


HWND WINAPI CreateToolbar(HWND hwnd, DWORD ws, WORD wID, int nBitmaps,
	HINSTANCE hBMInst, WORD wBMID,
	LPCTBBUTTON lpButtons, int iNumButtons);

#define CMB_DISCARDABLE 0x01    // create bitmap as discardable
#define CMB_MASKED      0x02    // create image/mask pair in bitmap


#define TB_ENABLEBUTTON	(WM_USER + 1)
// wParam: UINT, button ID
// lParam: BOOL LOWORD, enable if nonzero; HIWORD not used, 0
// return: not used
//

#define TB_CHECKBUTTON	(WM_USER + 2)
// wParam: UINT, button ID
// lParam: BOOL LOWORD, check if nonzero; HIWORD not used, 0
// return: not used
//

#define TB_PRESSBUTTON	(WM_USER + 3)
// wParam: UINT, button ID
// lParam: BOOL LOWORD, press if nonzero; HIWORD not used, 0
// return: not used
//

#define TB_HIDEBUTTON	(WM_USER + 4)
// wParam: UINT, button ID
// lParam: BOOL LOWORD, hide if nonzero; HIWORD not used, 0
// return: not used
//
#define TB_INDETERMINATE	(WM_USER + 5)
// wParam: UINT, button ID
// lParam: BOOL LOWORD, make indeterminate if nonzero; HIWORD not used, 0
// return: not used
//

#define TB_ISBUTTONENABLED	(WM_USER + 9)
// wParam: UINT, button ID
// lParam: not used, 0
// return: BOOL LOWORD, enabled if nonzero; HIWORD not used
//

#define TB_ISBUTTONCHECKED	(WM_USER + 10)
// wParam: UINT, button ID
// lParam: not used, 0
// return: BOOL LOWORD, checked if nonzero; HIWORD not used
//

#define TB_ISBUTTONPRESSED	(WM_USER + 11)
// wParam: UINT, button ID
// lParam: not used, 0
// return: BOOL LOWORD, pressed if nonzero; HIWORD not used
//

#define TB_ISBUTTONHIDDEN	(WM_USER + 12)
// wParam: UINT, button ID
// lParam: not used, 0
// return: BOOL LOWORD, hidden if nonzero; HIWORD not used
//

#define TB_ISBUTTONINDETERMINATE	(WM_USER + 13)
// wParam: UINT, button ID
// lParam: not used, 0
// return: BOOL LOWORD, indeterminate if nonzero; HIWORD not used
//

#define TB_SETSTATE		(WM_USER + 17)
// wParam: UINT, button ID
// lParam: UINT LOWORD, state bits; HIWORD not used, 0
// return: not used
//

#define TB_GETSTATE		(WM_USER + 18)
// wParam: UINT, button ID
// lParam: not used, 0
// return: UINT LOWORD, state bits; HIWORD not used
//

#define TB_ADDBITMAP		(WM_USER + 19)
// wParam: UINT, number of button graphics in bitmap
// lParam: one of:
//	   HINSTANCE LOWORD, module handle; UINT HIWORD, resource id
//	   HINSTANCE LOWORD, NULL; HBITMAP HIWORD, bitmap handle
// return: one of:
//	   int LOWORD, index for first new button; HIWORD not used
//	   int LOWORD, -1 indicating error; HIWORD not used
//

#define TB_ADDBUTTONS		(WM_USER + 20)
// wParam: UINT, number of buttons to add
// lParam: LPTBBUTTON, pointer to array of TBBUTTON structures
// return: not used
//

#define TB_INSERTBUTTON		(WM_USER + 21)
// wParam: UINT, index for insertion (appended if index doesn't exist)
// lParam: LPTBBUTTON, pointer to one TBBUTTON structure
// return: not used
//

#define TB_DELETEBUTTON		(WM_USER + 22)
// wParam: UINT, index of button to delete
// lParam: not used, 0
// return: not used
//

#define TB_GETBUTTON		(WM_USER + 23)
// wParam: UINT, index of button to get
// lParam: LPTBBUTTON, pointer to TBBUTTON buffer to receive button
// return: not used
//

#define TB_BUTTONCOUNT		(WM_USER + 24)
// wParam: not used, 0
// lParam: not used, 0
// return: UINT LOWORD, number of buttons; HIWORD not used
//

#define TB_COMMANDTOINDEX	(WM_USER + 25)
// wParam: UINT, command id
// lParam: not used, 0
// return: UINT LOWORD, index of button (-1 if command not found);
//	   HIWORD not used
//


#define TBN_BEGINDRAG	0x0201
#define TBN_ENDDRAG	0x0203
