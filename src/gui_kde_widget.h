/* vi:set ts=8 sts=0 sw=8:
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

#ifndef GUI_KDE_WIDGET
#define GUI_KDE_WIDGET

#if 1
#define dbf( format, args... ) { printf( "%s" " : " format "\n" , __FUNCTION__ , ## args ); fflush(stdout); }
#define db()       { printf( "%s\n", __FUNCTION__ );fflush(stdout); }
#else
#define dbf(format, args... )
#define db()
#endif

#define UNIX	    // prevent a warning : a symbol is defined twice in X and Qt

#include <qdialog.h>
#include <qlabel.h>
#include <qsignalmapper.h>
#include <qtimer.h>
#include <qmainwindow.h>
#include <qapplication.h>
#include <qevent.h>
#include <qlineedit.h>
#include <qpainter.h>
#include <qwidget.h>
#include <qpopupmenu.h>
#include <klocale.h>
#include <kapp.h>
#include <kcmdlineargs.h>
#include <kaboutdata.h>
#include <keditcl.h>
#include <kaboutdata.h>
#if (KDE_VERSION>=290)
#include <kmainwindow.h>
#else
#include <ktmainwindow.h>
#endif
#include <kparts/part.h>
#include <kurl.h>
#include "kvim_iface.h"

#undef UNIX	    // prevent a warning
extern "C" {
#include "vim.h"
}

class QPushButton;
class QDialog;
class QLineEdit;
class QSignalMapper;
class QPaintEvent;

enum BlinkState {
	BLINK_NONE,
	BLINK_ON,
	BLINK_OFF
};

class VimWidget : public QWidget, virtual public KVim
{
	Q_OBJECT

public:
	VimWidget( QWidget *parent=0, const char *name=0, WFlags f=0 );
	virtual void paintEvent( QPaintEvent *);
	void	draw_string(int x, int y, QString s, int len, int flags);

	/** Init the blinking time */
	void set_blink_time( long, long, long );
	void start_cursor_blinking();
	void stop_cursor_blinking();
	void wait(long);
#ifdef FEAT_CLIENTSERVER
	void serverActivate(WId id);
#endif
#ifdef FEAT_MZSCHEME
	void enable_mzscheme_threads();
	void disable_mzscheme_threads();
#endif
	void flash();
	
	/** DCOP */
	void execNormal(QString command);
	void execInsert(QString command);
	void execRaw(QString command);
	void execCmd(QString command);
	QString eval(QString expr);
	
	bool wait_done;
	BlinkState blink_state;
	QPainter *painter;
	QPopupMenu *menu;

protected:
	virtual void keyPressEvent( QKeyEvent * );
	virtual void mousePressEvent( QMouseEvent *);
	virtual void mouseDoubleClickEvent( QMouseEvent *);
	virtual void mouseReleaseEvent( QMouseEvent *);
	virtual void mouseMoveEvent( QMouseEvent *);
	virtual void focusInEvent( QFocusEvent * );
	virtual void focusOutEvent( QFocusEvent * );
	virtual void dragEnterEvent (QDragEnterEvent *);
	virtual void dropEvent (QDropEvent *);
#ifdef FEAT_XIM
	virtual void imStartEvent ( QIMEvent * );
	virtual void imEndEvent ( QIMEvent * );
	virtual void imComposeEvent ( QIMEvent * );
#endif
#ifdef FEAT_MZSCHEME
	virtual void timerEvent( QTimerEvent * );
#endif

	/* cursor blinking stuff */
	QTimer blink_timer;
	long blink_wait_time, blink_on_time, blink_off_time;

	/* wait for input */
	QTimer	wait_timer;
	
#ifdef FEAT_MZSCHEME
	int	mzscheme_timer_id;
#endif

public slots:
	void    blink_cursor();
	void	wait_timeout();
};

class VimMainWindow : public KMainWindow
{
	Q_OBJECT

public:
	VimMainWindow ( const char *name = 0L, WFlags f = WDestructiveClose );

	/** called when the widget closes */
//	bool close(bool alsoDelete);
	VimWidget	*w;
        KEdFind         *finddlg;
        KEdReplace      *repldlg;
	int		have_tearoff;
	QTextCodec      *codec;

public slots:
	void    menu_activated(int dx);
	void 	clipboard_selection_update();
	void 	clipboard_data_update();
        void    slotSearch();
        void    slotFind();
        void    slotReplace();
        void    slotReplaceAll();
	void    showAboutApplication();
	void    showAboutKDE();
	void    showBugReport();
	void    showTipOfTheDay();
	void    buffersToolbar();
	bool    isLocked();
	void    lock();
	void    unlock();

protected:
	virtual void wheelEvent (QWheelEvent *);
	virtual void resizeEvent ( QResizeEvent *e );

#if defined(FEAT_SESSION)
	void saveGlobalProperties (KConfig *conf);
	void readGlobalProperties (KConfig *conf);
#endif
	bool queryClose();
	bool queryExit();
	bool locked;
};


class VimDialog : public QDialog
{
	Q_OBJECT
public:
	VimDialog (int type,		/* type of dialog */
	       unsigned char * title,		/* title of dialog */
	       unsigned char * message,	/* message text */
	       unsigned char * buttons,	/* names of buttons */
	       int def_but,		/* default button */
	       char_u *textfield);		/* input text */
private:
	QSignalMapper	mapper;
	QLineEdit	*entry;
	char_u		*ret;
	int		butNb;

protected slots:
	void done(int);
};


/*
 * QScrollBar  pool
 */
struct GuiScrollbar;

class SBPool : public QObject
{
	Q_OBJECT
public:
	SBPool(void);
	void create(GuiScrollbar * sb, int orient);
	void destroy(GuiScrollbar * sb);
public slots:
	void sbUsed(int who);
private:
	QSignalMapper mapper;
};

class KVimUtils {
public:
	static QString convertEncodingName(QString);
#if QT_VERSION<300
	static bool fromString(QFont*,QString);
	static QString toString(QFont*);
#endif
};

extern VimMainWindow	*vmw;
extern SBPool		*sbpool;
extern QString          *argServerName;

#endif // GUI_KDE_WIDGET
