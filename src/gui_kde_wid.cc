/* vi:set ts=8 sts=4 sw=4:
 *
 * VIM - Vi IMproved		by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 */

/*
 * Porting to KDE(2) was done by
 *
 *  (C) 2000 by Thomas Capricelli <orzel@freehackers.org>
 *
 *  Please visit http://freehackers.org/kvim for other vim- or
 *  kde-related coding.
 *
 *  $Id$
 *
 */

#include <assert.h>
#include <qpainter.h>
#include <qevent.h>
#include <qpushbutton.h>
#include <qscrollbar.h>
#include <qlayout.h>
#include <qclipboard.h>
#include <qdragobject.h>
#include <qstrlist.h>
#include <qmenubar.h>
#include <qtextcodec.h>
#if QT_VERSION>=300
#include <qptrlist.h>
#include <ktip.h>
#endif
#include <kglobal.h>
#include <kconfig.h>
#include <kaboutapplication.h>
#include <dcopclient.h>
#include <kaboutkde.h>
#include <kbugreport.h>
#include <kurldrag.h>
#include <kmenubar.h>
#include <ktoolbar.h>
#include <kstandarddirs.h>
#include "gui_kde_wid.h"
#include <qxembed.h>

extern "C"
{
#include "version.h"
}

// Pixmap for dialog
#ifdef FEAT_GUI_DIALOG
# include "../../pixmaps/alert.xpm"
# include "../../pixmaps/error.xpm"
# include "../../pixmaps/generic.xpm"
# include "../../pixmaps/info.xpm"
# include "../../pixmaps/quest.xpm"
#endif

/**
 * Keycodes recognized by vim.
 */
struct special_key {//{{{
    int qtkey;
    char_u code0;
    char_u code1;
} special_keys[] =
{
    { Qt::Key_Up,		'k', 'u' },
    { Qt::Key_Down,		'k', 'd' },
    { Qt::Key_Left,		'k', 'l' },
    { Qt::Key_Right,		'k', 'r' },
    { Qt::Key_F1,		'k', '1' },
    { Qt::Key_F2,		'k', '2' },
    { Qt::Key_F3,		'k', '3' },
    { Qt::Key_F4,		'k', '4' },
    { Qt::Key_F5,		'k', '5' },
    { Qt::Key_F6,		'k', '6' },
    { Qt::Key_F7,		'k', '7' },
    { Qt::Key_F8,		'k', '8' },
    { Qt::Key_F9,		'k', '9' },
    { Qt::Key_F10,		'k', ';' },
    { Qt::Key_F11,		'F', '1' },
    { Qt::Key_F12,		'F', '2' },
    { Qt::Key_F13,		'F', '3' },
    { Qt::Key_F14,		'F', '4' },
    { Qt::Key_F15,		'F', '5' },
    { Qt::Key_F16,		'F', '6' },
    { Qt::Key_F17,		'F', '7' },
    { Qt::Key_F18,		'F', '8' },
    { Qt::Key_F19,		'F', '9' },
    { Qt::Key_F20,		'F', 'A' },
    { Qt::Key_F21,		'F', 'B' },
    { Qt::Key_F22,		'F', 'C' },
    { Qt::Key_F23,		'F', 'D' },
    { Qt::Key_F24,		'F', 'E' },
    { Qt::Key_F25,		'F', 'F' },
    { Qt::Key_F26,		'F', 'G' },
    { Qt::Key_F27,		'F', 'H' },
    { Qt::Key_F28,		'F', 'I' },
    { Qt::Key_F29,		'F', 'J' },
    { Qt::Key_F30,		'F', 'K' },
    { Qt::Key_F31,		'F', 'L' },
    { Qt::Key_F32,		'F', 'M' },
    { Qt::Key_F33,		'F', 'N' },
    { Qt::Key_F34,		'F', 'O' },
    { Qt::Key_F35,		'F', 'P' },
    { Qt::Key_Help,		'%', '1' },
    //    { Qt::Key_Undo,		'&', '8' }, <= hmmm ?
    { Qt::Key_BackSpace,	'k', 'b' },
    { Qt::Key_Insert,		KS_EXTRA, KE_KINS },
    { Qt::Key_Delete,		KS_EXTRA, KE_KDEL },
    { Qt::Key_Home,		'K', '1' },
    { Qt::Key_End,		'K', '4' },
    { Qt::Key_Prior,		'K', '3' },
    { Qt::Key_Next,		'K', '5' },
    { Qt::Key_Print,		'%', '9' },

    { Qt::Key_Plus,	'K', '6'},
    { Qt::Key_Minus,	'K', '7'},
    { Qt::Key_Slash,	'K', '8'},
    { Qt::Key_multiply,	'K', '9'},
    { Qt::Key_Enter,	'K', 'A'},
    { Qt::Key_Period,	'K', 'B'},

    { Qt::Key_0,	'K', 'C'},
    { Qt::Key_1,	'K', 'D'},
    { Qt::Key_2,	'K', 'E'},
    { Qt::Key_3,	'K', 'F'},
    { Qt::Key_4,	'K', 'G'},
    { Qt::Key_5,	'K', 'H'},
    { Qt::Key_6,	'K', 'I'},
    { Qt::Key_7,	'K', 'J'},
    { Qt::Key_8,	'K', 'K'},
    { Qt::Key_9,	'K', 'L'},
    /* End of list marker: */
    { 0, 0, 0 }
};//}}}

#ifdef FEAT_CLIENTSERVER
typedef int (*QX11EventFilter) (XEvent*);
extern QX11EventFilter qt_set_x11_event_filter (QX11EventFilter filter);
static QX11EventFilter oldFilter = 0;
static int kvim_x11_event_filter( XEvent* e);
#endif
void gui_keypress(QKeyEvent *e);

/*
 * Return OK if the key with the termcap name "name" is supported.
 */
    int
gui_mch_haskey(char_u * name)//{{{
{
    for (int i = 0; special_keys[i].qtkey != 0; i++)
	if (name[0] == special_keys[i].code0
					  && name[1] == special_keys[i].code1)
	    return OK;
    return FAIL;
}//}}}

/*
 * custom Frame for drawing ...
 */
void VimWidget::paintEvent(QPaintEvent *e)//{{{
{
    QRect r = e->rect();
    gui_redraw(r.x(), r.y(), r.width(), r.height());
}//}}}

void VimWidget::draw_string(int x, int y, QString s, int len, int flags)//{{{
{
    gui.current_font->setBold(flags & DRAW_BOLD);
    gui.current_font->setUnderline(flags & DRAW_UNDERL);
    gui.current_font->setItalic(flags & DRAW_ITALIC);
    painter->setBackgroundMode(flags & DRAW_TRANSP ? Qt::TransparentMode : Qt::OpaqueMode);
    painter->setFont(*(gui.current_font));
    painter->drawText(x, y, s, len);
}//}}}

void VimWidget::mousePressEvent(QMouseEvent *event)//{{{
{
    int button=0;
    int modifiers=0;
    ButtonState state = event->state();
    ButtonState buttons = event->button();

    //Look at button states
    if (buttons & QMouseEvent::LeftButton)
	button |= MOUSE_LEFT;
    if (buttons & QMouseEvent::RightButton)
	button |= MOUSE_RIGHT;
    if (buttons & QMouseEvent::MidButton)
	button |= MOUSE_MIDDLE;
    //Look for keyboard modifiers
    if (state & QMouseEvent::ShiftButton)
	modifiers |= MOUSE_SHIFT;
    if (state & QMouseEvent::ControlButton)
	modifiers |= MOUSE_CTRL;
    if (state & QMouseEvent::AltButton)
	modifiers |= MOUSE_ALT;
    gui_send_mouse_event(button,event->x(),event->y(),FALSE,modifiers);
#if QT_VERSION>=300
    QByteArray params;
    QDataStream stream(params, IO_WriteOnly);
    stream << kapp->dcopClient()->appId() << button << modifiers << gui.row << gui.col;
    kapp->dcopClient()->emitDCOPSignal("mousePEvent(QCString,int,int,int,int)", params);
#endif
    event->accept();
}//}}}

#if defined(FEAT_SESSION)
void VimMainWindow::saveGlobalProperties (KConfig *conf)
{
    //we write a mksession file to a file written in the user's ~/.kde/share/config/
    //the name of the file in saved in 'conf'
    //when restoring app, we source this file
#if 0 //disabled for release
    QString filename = KGlobal::dirs()->localkdedir() + KGlobal::dirs()->kde_default("config") + kapp->randomString(10);
    QString cmd("mksession ");
    cmd+=filename;
    do_cmdline_cmd((char_u*)cmd.latin1());
    conf->writePathEntry("sessionfile", filename);
    conf->sync();
#endif
}

void VimMainWindow::readGlobalProperties (KConfig *conf)
{
#if 0
    QString filename = conf->readPathEntry("sessionfile");
    if (filename.isNull()) return;
    QString cmd("source ");
    cmd+=filename;
    do_cmdline_cmd((char_u*)cmd.latin1());
#endif
}
#endif

void VimMainWindow::wheelEvent (QWheelEvent *event)//{{{
{
    ButtonState state = event->state();
    int button=0;
    int modifiers=0;

    if (event->delta()>0)
	button|=MOUSE_4;
    else button|=MOUSE_5;

    if (state & ShiftButton)
	modifiers|=MOUSE_SHIFT;
    if (state & ControlButton)
	modifiers|=MOUSE_CTRL;
    if (state & AltButton)
	modifiers|=MOUSE_ALT;

    gui_send_mouse_event(button,event->x(),event->y(),FALSE,modifiers);
#if QT_VERSION>=300
    QByteArray params;
    QDataStream stream(params, IO_WriteOnly);
    stream << kapp->dcopClient()->appId() << button << modifiers << gui.row << gui.col;
    kapp->dcopClient()->emitDCOPSignal("mouseWhlEvent(QCString, int, int,int,int)", params);
#endif
    event->accept();
}//}}}

void VimWidget::mouseDoubleClickEvent(QMouseEvent *event)//{{{
{
    ButtonState state = event->state();
    ButtonState buttons = event->button();
    int modifiers=0;
    int button=0;

    //Look at button states
    if (buttons & LeftButton)
	button|=MOUSE_LEFT;
    if (buttons & RightButton)
	button|=MOUSE_RIGHT;
    if (buttons & MidButton)
	button|=MOUSE_MIDDLE;

    //Look for keyboard modifiers
    if (state & ShiftButton)
	modifiers|=MOUSE_SHIFT;
    if (state & ControlButton)
	modifiers|=MOUSE_CTRL;
    if (state & AltButton)
	modifiers|=MOUSE_ALT;

    gui_send_mouse_event(button,event->x(),event->y(),TRUE,modifiers);
#if QT_VERSION>=300
    QByteArray params;
    QDataStream stream(params, IO_WriteOnly);
    stream << kapp->dcopClient()->appId() << button << modifiers << gui.row << gui.col;
    kapp->dcopClient()->emitDCOPSignal("mouseDblClickEvent(QCString, int, int,int,int)", params);
#endif
    event->accept();
}//}}}

void VimWidget::mouseMoveEvent(QMouseEvent *event){//{{{
    ButtonState state = event->state();
    int modifiers=0;
    int button=0;

    gui_mch_mousehide(FALSE);

    //Look at button states
    //warning: we use state here, this is important !
    if (state & QMouseEvent::LeftButton || state & QMouseEvent::RightButton || state & QMouseEvent::MidButton)
	button|=MOUSE_DRAG;

    //Look for keyboard modifiers
    if (state & ShiftButton)
	modifiers|=MOUSE_SHIFT;
    if (state & ControlButton)
	modifiers|=MOUSE_CTRL;
    if (state & AltButton)
	modifiers|=MOUSE_ALT;
    if (button!=MOUSE_DRAG)
	gui_mouse_moved(event->x(),event->y());
    else
	gui_send_mouse_event(MOUSE_DRAG,event->x(),event->y(),FALSE,modifiers);
}//}}}

void VimWidget::mouseReleaseEvent(QMouseEvent *event)//{{{
{
    ButtonState state = event->state();
    int modifiers=0;

    //Look for keyboard modifiers
    if (state & ShiftButton)
	modifiers|=MOUSE_SHIFT;
    if (state & ControlButton)
	modifiers|=MOUSE_CTRL;
    if (state & AltButton)
	modifiers|=MOUSE_ALT;

    gui_send_mouse_event(MOUSE_RELEASE,event->x(),event->y(),FALSE,modifiers);
    event->accept();
}//}}}

/*
 *  The main widget (everything but toolbar/menubar)
 */
    VimWidget::VimWidget( QWidget *parent, const char *name, WFlags f )//{{{
:QWidget(parent, name, f)
    ,DCOPObject("KVim")
#ifdef FEAT_MZSCHEME
    ,mzscheme_timer_id(-1)
#endif
{
    //to be able to show/hide the cursor when moving the mouse
    setMouseTracking(true);
    painter=new QPainter(this);

    setKeyCompression(true);
    setFocusPolicy( QWidget::StrongFocus );
    setAcceptDrops(TRUE); // DND
    blink_state = BLINK_NONE;
    blink_on_time = 700;
    blink_off_time = 400;
    blink_wait_time = 250;
    connect( &blink_timer, SIGNAL( timeout() ), SLOT( blink_cursor() ));
    connect( &wait_timer, SIGNAL( timeout() ), SLOT ( wait_timeout() ));
}//}}}

void VimWidget::execNormal(QString command)//{{{
{
    QString cmd("execute 'normal ");
    cmd+=command;
    cmd+="'";
    QCString unistring = vmw->codec->fromUnicode(cmd);
    do_cmdline_cmd((char_u *)(const char*)unistring);
    gui_update_screen();
}//}}}

void VimWidget::execInsert(QString command)//{{{
{
    QString cmd("execute 'normal i");
    cmd+=command;
    cmd+="'";
    QCString unistring = vmw->codec->fromUnicode(cmd);
    do_cmdline_cmd((char_u *)(const char*)unistring);
    gui_update_screen();
}//}}}

void VimWidget::execRaw(QString command)//{{{
{
    QString cmd("execute '");
    cmd+=command;
    cmd+="'";
    QCString unistring = vmw->codec->fromUnicode(cmd);
    do_cmdline_cmd((char_u *)(const char*)unistring);
    gui_update_screen();
}//}}}

void VimWidget::execCmd(QString command)//{{{
{
    QCString unistring = vmw->codec->fromUnicode(command);
    do_cmdline_cmd((char_u *)(const char*)unistring);
    gui_update_screen();
}//}}}

QString VimWidget::eval(QString expr)//{{{
{
#ifdef FEAT_EVAL
    QCString unistring = vmw->codec->fromUnicode(expr);
    QString val((const char *)eval_to_string((char_u *)(const char*)unistring,NULL));
    return val;
#else
    return QString::null;
#endif
}//}}}

void VimWidget::wait(long wtime)//{{{
{
    if ( wait_timer.isActive() ) wait_timer.stop();
    wait_done = false;
    wait_timer.start( wtime, true);
}//}}}

void VimWidget::wait_timeout() //{{{
{
    wait_done = true;
}//}}}

void VimWidget::dragEnterEvent (QDragEnterEvent *e)//{{{
{
#if (defined(FEAT_WINDOWS) && defined(HAVE_DROP_FILE)) || defined(PROTO)
    e->accept(QUriDrag::canDecode(e));
#else
    e->ignore();
#endif
}//}}}

void VimWidget::dropEvent (QDropEvent *e) // {{{
{
#if (defined(FEAT_WINDOWS) && defined(HAVE_DROP_FILE)) || defined(PROTO)
    QStrList  urls;

    char_u	**fnames;
    int		redo_dirs = FALSE;
    int		i;
    int		n;
    int		nfiles;
    int		url = FALSE;

    /* Count how many items there may be and normalize delimiters. */

    if (QUriDrag::decode(e, urls))
    {
	n = urls.count();
        fnames = (char_u **)lalloc((n+1) * sizeof(char_u *), TRUE);
	nfiles = 0;
#if QT_VERSION>=300
	QPtrListIterator<char> it(urls);
	for (; it.current(); ++it )
	{
	    KURL u(*it);
#else
	    for (i=0;i<urls.count();++i)
	    {
		KURL u(urls.at(i));
#endif
		if ( !u.isLocalFile() )
		    url = TRUE;
		else
		{
		    fnames[nfiles] = (char_u *)strdup((const char *)u.path());
		    ++nfiles;
		}
	    }
	    /* Real files (i.e. not http and not ftp) */
	    if (url == FALSE)
	    {
		if (nfiles == 1)
		{
		    if (mch_isdir(fnames[0]))
		    {
			/* Handle dropping a directory on Vim. */
			if (mch_chdir((char *)fnames[0]) == 0)
			{
			    free(fnames[0]);
			    fnames[0] = NULL;
			    redo_dirs = TRUE;
			}
		    }
		}
		else
		{
		    /* Ignore any directories */
		    for (i = 0; i < nfiles; ++i)
		    {
			if (mch_isdir(fnames[i]))
			{
			    vim_free(fnames[i]);
			    fnames[i] = NULL;
			}
		    }
		}

		if (0)
		{
		    /* Shift held down, change to first file's directory */
		    if (fnames[0] != NULL && vim_chdirfile(fnames[0]) == OK)
			redo_dirs = TRUE;
		}
		else
		{
		    char_u	dirname[MAXPATHL];
		    char_u	*s;
		    if (mch_dirname(dirname, MAXPATHL) == OK)
			for (i = 0; i < nfiles; ++i)
			    if (fnames[i] != NULL)
			    {
				s = shorten_fname(fnames[i], dirname);
				if (s != NULL && (s = vim_strsave(s)) != NULL)
				{
				    vim_free(fnames[i]);
				    fnames[i] = s;
				}
			    }
		}
	    }

	    /* Handle the drop, :edit or :split to get to the file */
	    handle_drop(nfiles, fnames, FALSE);

	    if (redo_dirs)
		shorten_fnames(TRUE);
	}

	/* Update the screen display */
	update_screen(NOT_VALID);
#ifdef FEAT_MENU
	gui_update_menus(0);
#endif
	setcursor();
	out_flush();
	gui_update_cursor(FALSE, FALSE);
	gui_mch_flush();
#endif
} // }}}

void VimWidget::keyPressEvent( QKeyEvent *e ) // {{{
{
    gui_keypress(e);
} // }}}

void gui_keypress(QKeyEvent *e) { // {{{
    int key = (int)e->key();
    int modifiers = 0,i;
    uchar string[256],string2[256];
    uchar *s,*d;
    Qt::ButtonState state = e->state();

    QCString unistring = vmw->codec->fromUnicode(e->text());
    if (unistring.length()>0)
	strncpy((char*)string, (const char*)unistring,unistring.length());
    string[unistring.length()] = 0;
    int len=unistring.length();

    // ignore certain keys
    if (key == Qt::Key_Shift || key == Qt::Key_Alt || key == Qt::Key_Control || key == Qt::Key_Meta
	    || key == Qt::Key_CapsLock || key == Qt::Key_NumLock || key == Qt::Key_ScrollLock )
    {
	e->ignore();
	return;
    }

#ifdef FEAT_MBYTE
    if (input_conv.vc_type != CONV_NONE)
    {
	mch_memmove(string2, string, len);
	len = convert_input(string2, len, sizeof(string2));
	s = string2;
    }
    else
#endif
	s = string;
    d = string;
    for (i = 0; i < len; ++i)
    {
	*d++ = s[i];
	if (d[-1] == CSI && d + 2 < string + sizeof(string))
	{
	    /* Turn CSI into K_CSI. */
	    *d++ = KS_EXTRA;
	    *d++ = (int)KE_CSI;
	}
    }
    len = d - string;


    // change shift-tab (backtab) into S_TAB
    if ( key == Qt::Key_BackTab && state & Qt::ShiftButton)
	key = Qt::Key_Tab;

    // Change C-@ and C-2 in NUL ? Gtk does this
    if ( (key == Qt::Key_2 || key == Qt::Key_At)
	    && state & Qt::ControlButton )
    {
	string[0] = NUL;
	len = 1;
    }
    else if (len == 0 && (key == Qt::Key_Space || key == Qt::Key_Tab))
    {
	/* When there are modifiers, these keys get zero length; we need the
	 * original key here to be able to add a modifier below. */
	string[0] = (key & 0xff);
	len = 1;
    }
    /* Check for Alt/Meta key (Mod1Mask), but not for a BS, DEL or character
     * that already has the 8th bit set.
     * Don't do this for <S-M-Tab>, that should become K_S_TAB with ALT. */
    if (len == 1
	    && (key != Qt::Key_BackSpace && key != Qt::Key_Delete)
	    && (string[0] & 0x80) == 0
	    && (state & Qt::AltButton)
	    && !(key == Qt::Key_Tab && (state & Qt::ShiftButton)))
    {
	string[0] |= 0x80;
#ifdef FEAT_MBYTE
	if (enc_utf8) // convert to utf-8
	{
	    string[1] = string[0] & 0xbf;
	    string[0] = ((unsigned)string[0] >> 6) + 0xc0;
	    if (string[1] == CSI)
	    {
		string[2] = KS_EXTRA;
		string[3] = (int)KE_CSI;
		len = 4;
	    }
	    else
		len = 2;
	}
#endif
    }

    /* Check for special keys, making sure BS and DEL are recognised. */
    if (len == 0 || key == Qt::Key_BackSpace || key == Qt::Key_Delete)
    {
	while (special_keys[i].qtkey != 0 && special_keys[i].qtkey != key ) i++;
	if (special_keys[i].qtkey != 0)
	{
		string[0] = CSI;
		string[1] = special_keys[i].code0;
		string[2] = special_keys[i].code1;
		len = -3;
	}
/*
	for (i = 0; special_keys[i].qtkey != 0 ; i++)
	{
	    if (special_keys[i].qtkey == key )
	    {
		string[0] = CSI;
		string[1] = special_keys[i].code0;
		string[2] = special_keys[i].code1;
		len = -3;
		break;
	    }
	}*/
    }

    if (len == 0)
    {
	//no need to dump that, that's a QT problem, we can't do anything
	//dbf("Unrecognised Key : %X %s", key, e->text().latin1());
	e->ignore();
	return;
    }


    /* Special keys (and a few others) may have modifiers */
    if (len == -3 || key == Qt::Key_Space || key == Qt::Key_Tab ||
	    key == Qt::Key_Return || key == Qt::Key_Enter ||
	    key == Qt::Key_Escape)
    {

	modifiers = 0;
	if (state & Qt::ShiftButton) modifiers |= MOD_MASK_SHIFT;
	if (state & Qt::ControlButton) modifiers |= MOD_MASK_CTRL;
	if (state & Qt::AltButton) modifiers |= MOD_MASK_ALT;

	/*
	 * For some keys a shift modifier is translated into another key
	 * code. Do we need to handle the case where len != 1 and
	 * string[0] != CSI?
	 */
	if (len == -3)
	    key = TO_SPECIAL(string[1], string[2]);
	else
	    key = string[0];

	key = simplify_key(key, &modifiers);
	if (key == CSI) key=K_CSI;

	if (IS_SPECIAL(key))
	{
	    string[0] = CSI;
	    string[1] = K_SECOND(key);
	    string[2] = K_THIRD(key);
	    len = 3;
	}
	else
	{
	    string[0] = key;
	    len = 1;
	}


	if (modifiers!=0)
	{
	    uchar string2[10];
	    string2[0] = CSI;
	    string2[1] = KS_MODIFIER;
	    string2[2] = modifiers;
	    add_to_input_buf(string2, 3);
	}

    } /* special keys */

    if (len == 1 && ((string[0] == Ctrl_C && ctrl_c_interrupts)
		|| (string[0] == intr_char && intr_char != Ctrl_C)))
    {
	trash_input_buf();
	got_int = TRUE;
    }

    add_to_input_buf(string, len);
    if (p_mh)
	gui_mch_mousehide(TRUE);

    //DCOP Embedding stuff
    //if we are here then the user has type something in the window, thus we can easily imagine that :
    // 1 - text has changed (emit textChanged())
    // 2 - characters were interactively inserted (emit charactersInteractivelyInserted())
    // 3 - cursor position has changed ( emit cursorPositionChanged() )
    // 4 - selection has changed ? dunno yet //XXX
    // 5 - undo changed too ? (each character typed in makes the undo changes anyway)
    // conclusion : this makes a lot of things to send to the vim kpart, maybe too much
    // for now i'll just send : keyboardEvent to the kpart with the event string as parameter,
    // with current current position
    // i'll do the same for mouseEvents
#if QT_VERSION>=300
    QByteArray params;
    QDataStream stream(params, IO_WriteOnly);
    stream << kapp->dcopClient()->appId() << unistring << gui.row << gui.col;
    kapp->dcopClient()->emitDCOPSignal("keyboardEvent(QCString, QCString,int,int)", params);
#endif
    e->ignore();
} // }}}

#ifdef FEAT_CLIENTSERVER
void VimWidget::serverActivate(WId id) //{{{
{
    if (serverName == NULL && serverDelayedStartName != NULL)
    {
	commWindow = id;
	(void)serverRegisterName(qt_xdisplay(), serverDelayedStartName);
    }
    else
	serverChangeRegisteredWindow( qt_xdisplay(), id);
}//}}}
#endif

#ifdef FEAT_XIM
void VimWidget::imStartEvent(QIMEvent *e)
{
    e->accept();
}

void VimWidget::imEndEvent(QIMEvent *e)
{
    uchar string[256];

    QCString unistring = vmw->codec->fromUnicode(e->text());
    if (unistring.length()>0)
	strncpy((char*)string, (const char*)unistring,unistring.length());
    string[unistring.length()] = 0;
    int len=unistring.length();

    add_to_input_buf(string, len);
    e->accept();
}

void VimWidget::imComposeEvent(QIMEvent *e)
{
    //i should do something here, displaying the text somewhere ... (status area ?)
    e->accept();
}
#endif


void VimMainWindow::lock()
{
    locked=true;
}

void VimMainWindow::unlock()
{
    locked=false;
}

bool VimMainWindow::isLocked()
{
    return locked;
}

// ->resize VimWidget if not locked
//
void VimMainWindow::resizeEvent ( QResizeEvent *e ) //{{{
{
    if ( vmw->isLocked() ) return;
    //remove toolbar and menubar height
    int height = e->size().height();
    int width = e->size().width();

    if (vmw->menuBar()->isVisible() && vmw->menuBar()->isEnabled()
#if QT_VERSION>=300
	    && !vmw->menuBar()->isTopLevelMenu()
#endif
	    )
	height -= vmw->menuBar()->height();
#ifdef FEAT_TOOLBAR
    if (vmw->toolBar()->isVisible() && vmw->toolBar()->isEnabled() &&
	    (vmw->toolBar()->barPos()==KToolBar::Top ||
	     vmw->toolBar()->barPos()==KToolBar::Bottom))
	height -= vmw->toolBar()->height();

    if (vmw->toolBar()->isVisible() && vmw->toolBar()->isEnabled() &&
	    (vmw->toolBar()->barPos()==KToolBar::Left ||
	     vmw->toolBar()->barPos()==KToolBar::Right))
	width -= vmw->toolBar()->width();
#endif
    height = ( ((int)(height/gui.char_height))*gui.char_height );
    if (!vmw->isLocked()) gui_resize_shell(width,height);
}//}}}

void VimWidget::focusInEvent( QFocusEvent * fe ) // {{{
{
    gui_focus_change(true);

    if (blink_state == BLINK_NONE)
	gui_mch_start_blink();
} // }}}

void VimWidget::focusOutEvent( QFocusEvent * fe )//{{{
{
    gui_focus_change(false);

    if (blink_state != BLINK_NONE)
	gui_mch_stop_blink();
}//}}}

void VimWidget::set_blink_time( long wait, long on, long off)//{{{
{
    blink_wait_time = wait;
    blink_on_time = on;
    blink_off_time = off;
}//}}}

void VimWidget::start_cursor_blinking()//{{{
{
    if (blink_timer.isActive()) blink_timer.stop();

    /* Only switch blinking on if none of the times is zero */
    if (blink_wait_time && blink_on_time && blink_off_time && gui.in_focus)
    {
	blink_state = BLINK_ON;
	gui_update_cursor(TRUE, FALSE);
	// The first blink appears after wait_time
	blink_timer.start( blink_wait_time, true);
    }
}//}}}

void VimWidget::blink_cursor()//{{{
{
    if (blink_state == BLINK_ON)
    {
	// set cursor off
	gui_undraw_cursor();
	blink_state = BLINK_OFF;
	blink_timer.start( blink_off_time, true);
    }
    else
    {
	// set cursor on
	gui_update_cursor(TRUE, FALSE);
	blink_state = BLINK_ON;
	blink_timer.start( blink_on_time, true);
    }
}//}}}

void VimWidget::stop_cursor_blinking()//{{{
{
    if (blink_timer.isActive()) blink_timer.stop();

    if (blink_state == BLINK_OFF)
	gui_update_cursor(TRUE, FALSE);

    blink_state = BLINK_NONE;
}//}}}

#ifdef FEAT_MZSCHEME
void VimWidget::timerEvent( QTimerEvent * evnt)//{{{
{
    if (evnt->timerId() == mzscheme_timer_id)
	timer_proc();
}//}}}

void VimWidget::enable_mzscheme_threads()//{{{
{
    mzscheme_timer_id = startTimer(p_mzq);
}//}}}

void VimWidget::disable_mzscheme_threads()//{{{
{
    killTimer(mzscheme_timer_id);
}//}}}
#endif

void VimWidget::flash()//{{{
{
    QPainter p(this);

    p.setRasterOp(Qt::XorROP);
    p.fillRect(geometry(),QColor(0xFF,0xFF,0xFF));
    p.flush();
    //FIXME: Make this a little smarter. Maybe add a timer or something
    usleep(19000);
    p.fillRect(geometry(),QColor(0xFF,0xFF,0xFF));
    p.flush();
    p.end();
}//}}}


/*
 *  The main Window
 */
    VimMainWindow::VimMainWindow ( const char *name , WFlags f)//{{{
:KMainWindow(0L, name,f)
{
#ifdef FEAT_CLIENTSERVER
    oldFilter = qt_set_x11_event_filter( kvim_x11_event_filter );
#endif
    if (echo_wid_arg== 1)
    {
	fprintf(stderr, "WID: %ld\n", (long)winId());
	fflush(stderr);
    }

    w = new VimWidget(this, "main vim widget");
    gui.w = w;
    setFocusProxy(w);
    w->setFocus();
    have_tearoff=0;

    finddlg=new KEdFind (this,0,false);
    repldlg=new KEdReplace (this,0,false);
    QObject::connect( finddlg, SIGNAL(search()), this, SLOT(slotSearch()) );
    QObject::connect( repldlg, SIGNAL(find()), this, SLOT(slotFind()) );
    QObject::connect( repldlg, SIGNAL(replace()), this, SLOT(slotReplace()) );
    QObject::connect( repldlg, SIGNAL(replaceAll()), this, SLOT(slotReplaceAll()) );

#ifdef FEAT_TOOLBAR
    connect(toolBar(), SIGNAL(clicked(int)), this, SLOT(menu_activated(int)));
#endif
#ifdef FEAT_CLIENTSERVER
    w->serverActivate(winId());

    if (serverName!=NULL)
        kapp->dcopClient()->registerAs(QCString((const char*)serverName),false);
    else if (serverDelayedStartName!=NULL)
        kapp->dcopClient()->registerAs(QCString((const char*)serverDelayedStartName),false);
    else if (argServerName!=NULL)
        kapp->dcopClient()->registerAs(argServerName->utf8(),false);
#else
    if (argServerName!=NULL)
        kapp->dcopClient()->registerAs(argServerName->utf8(),false);
#endif
    QXEmbed::initialize();

}//{{{

bool VimMainWindow::queryClose()//{{{
{
    gui_shell_closed();
    return true;
}//}}}

bool VimMainWindow::queryExit()//{{{
{
    return true;
}//}}}

void VimMainWindow::menu_activated(int dx)//{{{
{
#ifdef FEAT_MENU
    if (!dx) {	// tearoff
	return;
    }
    gui_mch_set_foreground();
    gui_menu_cb((VimMenu *) dx);
#endif
}//}}}


void VimMainWindow::clipboard_selection_update(){//{{{
    if (kapp->clipboard()->ownsSelection())
	clip_own_selection(&clip_star);
    else
	clip_lose_selection(&clip_star);
}//}}}

void VimMainWindow::clipboard_data_update(){//{{{
#if QT_VERSION>=300
    if (kapp->clipboard()->ownsClipboard())
	clip_own_selection(&clip_plus);
    else
	clip_lose_selection(&clip_plus);
#else
    if (kapp->clipboard()->ownsSelection())
	clip_own_selection(&clip_star);
    else
	clip_lose_selection(&clip_star);
#endif
}//}}}

void VimMainWindow::slotSearch()//{{{
{
    QString	find_text;
    bool	direction_down = TRUE;
    bool	casesensitive = TRUE;
    int		flags = FRD_FINDNEXT;

    find_text = finddlg->getText();
    direction_down = !(finddlg->get_direction());
    casesensitive = finddlg->case_sensitive();
    //    if (casesensitive) find_text = "\\C" + find_text;
    //    else find_text = "\\c" + find_text;
    if (casesensitive) flags|=FRD_MATCH_CASE;
    QCString unistring = vmw->codec->fromUnicode(find_text);
    gui_do_findrepl(flags, (char_u *)(const char *)unistring, NULL,(int)direction_down);
}//}}}

void VimMainWindow::slotFind()//{{{
{
    QString	find_text;
    bool	direction_down=TRUE;
    bool	casesensitive = TRUE;
    int		flags = FRD_R_FINDNEXT;

    find_text=repldlg->getText();
    direction_down = !(repldlg->get_direction());
    casesensitive = repldlg->case_sensitive();
    //    if (casesensitive) find_text = "\\C" + find_text;
    //    else find_text = "\\c" + find_text;
    if (casesensitive) flags|=FRD_MATCH_CASE;

    QCString unistring = vmw->codec->fromUnicode(find_text);
    gui_do_findrepl(flags, (char_u *)(const char *)unistring, NULL,(int)direction_down);
}//}}}

void VimMainWindow::slotReplace()//{{{
{
    QString	find_text;
    QString     repl_text;
    bool	direction_down=TRUE;
    bool	casesensitive = TRUE;
    int		flags = FRD_REPLACE;

    find_text=repldlg->getText();
    repl_text=repldlg->getReplaceText();
    direction_down = !(repldlg->get_direction());
    //if (casesensitive) find_text = "\\C" + find_text;
    //else find_text = "\\c" + find_text;
    if (casesensitive) flags|=FRD_MATCH_CASE;

    QCString unistring = vmw->codec->fromUnicode(find_text);
    QCString unistring2 = vmw->codec->fromUnicode(repl_text);
    gui_do_findrepl(flags, (char_u *)(const char *)unistring,(char_u *)(const char*)unistring2,(int)direction_down);
}//}}}

void VimMainWindow::slotReplaceAll()//{{{
{
    QString	find_text;
    QString     repl_text;
    bool	direction_down=TRUE;
    bool	casesensitive = TRUE;
    int		flags = FRD_REPLACEALL;

    find_text=repldlg->getText();
    repl_text=repldlg->getReplaceText();
    direction_down = !(repldlg->get_direction());
    casesensitive = repldlg->case_sensitive();
    //    if (casesensitive) find_text = "\\C" + find_text;
    //    else find_text = "\\c" + find_text;
    if (casesensitive) flags|=FRD_MATCH_CASE;
    QCString unistring = vmw->codec->fromUnicode(find_text);
    QCString unistring2 = vmw->codec->fromUnicode(repl_text);
    gui_do_findrepl(flags, (char_u *)(const char *)unistring,(char_u *)(const char*)unistring2,(int)direction_down);
}//}}}

void VimMainWindow::showAboutKDE()
{
    KAboutKDE *kde = new KAboutKDE(this);
    kde->show();
}

void VimMainWindow::showAboutApplication()//{{{
{
    KAboutData *aboutData = new KAboutData (
	    "kvim"
	    , I18N_NOOP("KVim")
	    , VIM_VERSION_SHORT
	    , I18N_NOOP("Vim in a KDE interface")
	    , 0
	    , "(c) Vim Team, \":help credits\" for more infos.\nType \":help iccf\" to see how you can help the children in Uganda"
	    , 0l
	    , "http://freehackers.org/kvim"
	    , "kvim-dev@freenux.org"
	    );

    aboutData->addAuthor("Bram Moolenaar",
	    I18N_NOOP("Main vim author"),
	    "Bram@vim.org",
	    "http://www.vim.org/");
    aboutData->addAuthor("Thomas Capricelli",
	    I18N_NOOP("KDE porting"),
	    "orzel@freehackers.org",
	    "http://orzel.freehackers.org");
    aboutData->addAuthor("Philippe Fremy",
	    I18N_NOOP("KDE porting"),
	    "pfremy@chez.com",
	    "http://www.freehackers.org/kvim");
    aboutData->addAuthor("Mark Westcott",
	    I18N_NOOP("Qtopia porting, maintainer of the Qtopia part"),
	    "mark@houseoffish.org",
	    "http://houseoffish.org");
    aboutData->addAuthor("Mickael Marchand",
	    I18N_NOOP("KDE porting, maintainer"),
	    "marchand@kde.org",
	    "http://freenux.org");
    aboutData->addAuthor("Many other people",
	    I18N_NOOP("type :help credits for more infos")
	    );
    aboutData->addCredit("Vince Negri",
	    I18N_NOOP("Antialiasing support, Color fixes"),
	    "vnegri@asl-electronics.co.uk");
    aboutData->addCredit("Malte Starostik",
	    I18N_NOOP("Patch for performance improvement"),
	    "malte@kde.org");
    aboutData->addCredit("Mark Stosberg",
	    I18N_NOOP("Provided a FreeBSD box to debug KVim on BSD"),
	    "mark@summersault.com"
	    );
    aboutData->addCredit("Henrik Skott",
	    I18N_NOOP("Font patch when KDE not configured"),
	    "henrik.skott@hem.utfors.se"
	    );
    aboutData->addCredit("Kailash Sethuraman",
	    I18N_NOOP("NetBSD configure/compilation fixes")
	    );
    aboutData->setLicenseText(
"KVim as an extension of Vim follows Vim license.\n\
You can read it with \":help license\"\n\
Or read the file $VIMRUNTIME/doc/uganda.txt.");

    KAboutApplication *about = new KAboutApplication(aboutData);
    about->show();
}//}}}

void VimMainWindow::showTipOfTheDay()
{
#if QT_VERSION>=300
    KTipDialog::showTip (vmw,QString::null,true);
#endif
}

void VimMainWindow::buffersToolbar()
{

}

void VimMainWindow::showBugReport()
{
    KBugReport *bug= new KBugReport(this,true);
    bug->show();
}
/*
 *   Vim Dialog
 *
 * Returns:
 *  0: Cancel
 *  1- : nb of the pressed button
 */

VimDialog::VimDialog (int type,		/* type of dialog *///{{{
	char_u * title,		/* title of dialog */
	char_u * message,	/* message text */
	char_u * buttons,	/* names of buttons */
	int def_but,		/* default button */
	char_u *textfield )	/* input field */
:QDialog(vmw, "vim generic dialog", true), // true is for "modal"
    mapper(this, "dialog signal mapper")
{
    /*
     * Create Icon
     */
    char ** icon_data;
    switch (type)
    {
	case VIM_GENERIC:
	    icon_data = generic_xpm;
	    break;
	case VIM_ERROR:
	    icon_data = error_xpm;
	    break;
	case VIM_WARNING:
	    icon_data = alert_xpm;
	    break;
	case VIM_INFO:
	    icon_data = info_xpm;
	    break;
	case VIM_QUESTION:
	    icon_data = quest_xpm;
	    break;
	default:
	    icon_data = generic_xpm;
    };
    QLabel * icon = new QLabel( this );
    icon->setPixmap( QPixmap( (const char **) icon_data ) );
    icon->setFixedSize( icon->sizeHint() );

    QLabel * text = new QLabel( (const char *)message, this );
    text->setAlignment( AlignHCenter | AlignVCenter | ExpandTabs );

    QStringList buttonText = QStringList::split( DLG_BUTTON_SEP, (char *) buttons);
    int butNb = buttonText.count();

    /*
     *  Layout
     */

    QVBoxLayout * vly = new QVBoxLayout( this, 5, 5 );
    QHBoxLayout * hly1 = new QHBoxLayout( vly, 5);
    hly1->addWidget( icon );
    hly1->addWidget( text );
    QHBoxLayout * hly3 = new QHBoxLayout ( vly , 5);
    if (textfield!=NULL)
    {
	entry = new QLineEdit((const char *)textfield,this);
	entry->setText((const char *)textfield);
	hly3->addWidget( entry );
	ret=textfield;
    }
    else
	entry=NULL;

    QHBoxLayout * hly2 = new QHBoxLayout( vly, 15);
    QString s;
    QPushButton * pushButton = 0L;
    for (int i=0; i<butNb; i++)
    {
	s = buttonText[i];
	pushButton = new QPushButton(s, this );
	if (s.find('&') != -1)
	    pushButton->setAccel(s.at(s.find('&')+1).latin1());

	hly2->addWidget( pushButton );
	if (i == def_but-1)
	{
	    pushButton->setDefault( true );
	    pushButton->setAutoDefault( true );
	    setResult( i+1 );
	}
	connect(pushButton, SIGNAL(clicked()), &mapper, SLOT(map()));
	mapper.setMapping(pushButton, i+1);
    }
    connect( &mapper, SIGNAL(mapped(int)), this, SLOT(done(int)));

    setCaption((const char *) title);

    vly->activate();
}//}}}

void VimDialog::done(int r)
{
    if (entry!=NULL)
    {
        if (r)
	{
	   QCString unistring=vmw->codec->fromUnicode(entry->text());
	   STRCPY(ret,(const char*)unistring);
	}
	else
	    *ret=NUL;
    }
    QDialog::done(r);
}

/*
 * ScrollBar pool handling
 */
SBPool::SBPool(void)//{{{
    :mapper(this, "SBPool signal mapper")
{
    connect(&mapper, SIGNAL(mapped(int)), this, SLOT(sbUsed(int)));
}//}}}


void SBPool::create(GuiScrollbar * sb, int orient)//{{{
{
    switch(orient)
    {
	case SBAR_HORIZ:
	    sb->w = new QScrollBar(QScrollBar::Horizontal, vmw);
	    break;
	case SBAR_VERT:
	    sb->w = new QScrollBar(QScrollBar::Vertical, vmw);
	    break;
	default:
	    sb->w = 0;
	    return;
    }

    connect(sb->w, SIGNAL(valueChanged(int)), &mapper, SLOT(map()));
    mapper.setMapping(sb->w, (int)sb);
}//}}}


void SBPool::sbUsed(int who)//{{{
{
    GuiScrollbar *sb = (GuiScrollbar*)who;
    gui_drag_scrollbar( sb, sb->w->value(), FALSE);
}//}}}


void SBPool::destroy(GuiScrollbar * sb)//{{{
{
    if (!sb->w) return;

    delete sb->w;
    sb->w = 0;
}//}}}

#ifdef FEAT_CLIENTSERVER
static int kvim_x11_event_filter( XEvent* e)//{{{
{
    if (e->xproperty.type == PropertyNotify
	    && e->xproperty.atom == commProperty
	    && e->xproperty.window == commWindow
	    && e->xproperty.state == PropertyNewValue)
	serverEventProc(qt_xdisplay(), e);

    if (oldFilter) return oldFilter( e );
    return FALSE;
}//}}}
#endif

//add some QT 3 fonts usefull functions
#if QT_VERSION<300
QString KVimUtils::toString(QFont *f)
{
    QStringList l;
    l.append(f->family());
    l.append(QString::number(f->pointSize()));
    l.append(QString::number(f->pixelSize()));
    l.append(QString::number((int)f->styleHint()));
    l.append(QString::number(f->weight()));
    l.append(QString::number((int)f->italic()));
    l.append(QString::number((int)f->underline()));
    l.append(QString::number((int)f->strikeOut()));
    l.append(QString::number((int)f->fixedPitch()));
    l.append(QString::number((int)f->rawMode()));
    return l.join(",");
}

bool KVimUtils::fromString(QFont *f, QString descrip)
{
   QStringList l(QStringList::split(',', descrip));

    int count = l.count();
    if (count != 10 && count != 9)
        return FALSE;

    f->setFamily(l[0]);
    f->setPointSize(l[1].toInt());
    if ( count == 9 )
    {
	f->setStyleHint((QFont::StyleHint) l[2].toInt());
	f->setWeight(l[3].toInt());
	f->setItalic(l[4].toInt());
	f->setUnderline(l[5].toInt());
	f->setStrikeOut(l[6].toInt());
	f->setFixedPitch(l[7].toInt());
	f->setRawMode(l[8].toInt());
    }
    else
    {
	f->setPixelSize(l[2].toInt());
	f->setStyleHint((QFont::StyleHint) l[3].toInt());
	f->setWeight(l[4].toInt());
	f->setItalic(l[5].toInt());
	f->setUnderline(l[6].toInt());
	f->setStrikeOut(l[7].toInt());
	f->setFixedPitch(l[8].toInt());
	f->setRawMode(l[9].toInt());
    }
    return TRUE;
}
#endif

QString KVimUtils::convertEncodingName(QString name)
{
    if (name.startsWith("ucs") || name.startsWith("utf-16")) return QString("utf16");
    if (name=="cp950") return QString("Big5");
    return QString();
}
