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

#include <qscrollbar.h>
#include <qcstring.h>
#include <qdatetime.h>
#include <qcursor.h>
#include <qfontmetrics.h>
#include <qpaintdevice.h>
#include <qclipboard.h>
#include <qregexp.h>
#include <kaboutkde.h>
#include <kiconloader.h>
#include <kfontdialog.h>
#include <kmessagebox.h>
#include <dcopclient.h>
#include <kwin.h>
#include <kmenubar.h>
#include <kconfig.h>
#if (QT_VERSION>=300)
#include <qnamespace.h>
#include <ktip.h>
#endif
#include <qpopupmenu.h>
#include <qpainter.h>
#include <qtextcodec.h>
#include <qfontmetrics.h>
#include <qfont.h>


#include "gui_kde_wid.h"


extern "C" {
#include "vim.h"
#include "version.h"
}

#include <stdio.h>

/*
 * global variable for KDE, we can't put them in Gui, cause there are C++ types
 */
VimMainWindow	*vmw = 0;
SBPool		*sbpool = 0;
QString		*argServerName = 0;

#ifdef FEAT_MOUSESHAPE
/* The last set mouse pointer shape is remembered, to be used when it goes
 * from hidden to not hidden. */
static int last_shape = 0;
#endif

/*
 * Arguments handled by KDE internally.
 */

#if QT_VERSION>=300
static int	tip = 0;    // 1 no dialog, 0 use it if enabled in conf,
			    // 2 force the tip
#endif
static int	reverse = 0; // 0 bg : white, 1 : bg : black
QString		*startfont;
QSize		*startsize;
static int	gui_argc = 0;
static char	**gui_argv = NULL;

/*
 * Parse the GUI related command-line arguments.  Any arguments used are
 * deleted from argv, and *argc is decremented accordingly.  This is called
 * when vim is started, whether or not the GUI has been started.
 */
    void
gui_mch_prepare(int *argc, char **argv)// {{{
{
    // copy args for KDE/Qt
    gui_argc = 0;

    // this one is not really good as all options are not for KDE/Qt ...
    gui_argv = (char **)lalloc((long_u)(*argc * sizeof(char *)), FALSE);
    if (gui_argv == NULL)
	return;
    gui_argv[gui_argc++] = argv[0];

    int found = 0;
    for (int i = 1; i < *argc ; i++)
    {
	if (found != 2)
	    found = 0;
	else
	{
	    found = 0;
	    // remove from the list of argv
	    if (--*argc > i)
	    {
		mch_memmove(&argv[i], &argv[i + 1],
			(*argc - i) * sizeof(char *));
	    }
	    i--;
	    continue;
	}

	if (strcmp(argv[i], "--servername") == 0)
	{
	    argServerName = new QString(argv[i+1]); // to get the serverName now
	}
#if QT_VERSION>+300
	if (strcmp(argv[i], "-tip") == 0 )
	{
	    tip = 2;
	    found = 1;
	}
	if (strcmp(argv[i], "-notip") == 0 )
	{
	    tip = 1;
	    found = 1;
	}
#endif
	if (strcmp(argv[i], "-black") == 0 )
	{
	    reverse = 1;
	    found = 1;
	}
	/* replaced by -black */
	/*		if (strcmp(argv[i], "-rv") == 0 )
	 *		{
	 reverse = 1;
	 found = 1;
	 }*/
	if (strcmp(argv[i], "-font") == 0 || strcmp(argv[i], "-fn") == 0)
	{
	    startfont = new QString(argv[i+1]);
	    found = 2;
	}
	if (strcmp(argv[i], "-geometry") == 0 || strcmp(argv[i], "-geom") == 0)
	{
	    found = 2;
	    QString text(argv[i + 1]);
	    QStringList list = QStringList::split(QChar('x'), text);
	    startsize = new QSize(list[0].toInt(), list[1].toInt());
	}
	if (strcmp(argv[i], "-display") == 0) //XXX: this does not work,
	    // too many -display options in main.c !
	    // ask Bram ...
	    {
		gui_argv[gui_argc++] = strdup("--display");
		gui_argv[gui_argc++] = argv[i+1];
		found = 0;
	    }
	if (strcmp(argv[i], "--display") == 0 )
	{
	    gui_argv[gui_argc++] = argv[i];
	    gui_argv[gui_argc++] = argv[i+1];
	    found = 2;
	}
	//KDE/Qt options with no args
	if (strcmp(argv[i], "--help-kde") == 0
		|| strcmp(argv[i], "--help-qt") == 0
		|| strcmp(argv[i], "--help-all") == 0
		|| strcmp(argv[i], "--reverse") == 0
		|| strcmp(argv[i], "--author") == 0
		//	|| strcmp(argv[i], "--version") == 0 //disabled we need these for kcmvim
		//	|| strcmp(argv[i], "-v") == 0
		|| strcmp(argv[i], "--license") == 0
		|| strcmp(argv[i], "--cmap") == 0
		|| strcmp(argv[i], "--nograb") == 0
		|| strcmp(argv[i], "--dograb") == 0
		|| strcmp(argv[i], "--sync") == 0
		|| strcmp(argv[i], "--noxim") == 0
		|| strcmp(argv[i], "--nocrashhandler") == 0
		|| strcmp(argv[i], "--waitforwm") == 0
	   )
	{
	    gui_argv[gui_argc++] = argv[i];
	    found = 1;
	}
	//this outputs KDE and Vim versions :)
	if (strcmp(argv[i], "--version") == 0
		|| strcmp(argv[i], "-v") == 0)
	{
	    gui_argv[gui_argc++] = argv[i];
	}


	// KDE/Qt options with one arg
	if (strcmp(argv[i], "--session") == 0
		|| strcmp(argv[i], "--ncols") == 0
		|| strcmp(argv[i], "--bg") == 0
		|| strcmp(argv[i], "--background") == 0
		|| strcmp(argv[i], "--fg") == 0
		|| strcmp(argv[i], "--foreground") == 0
		|| strcmp(argv[i], "--btn") == 0
		|| strcmp(argv[i], "--name") == 0
		|| strcmp(argv[i], "--title") == 0
		|| strcmp(argv[i], "--inputstyle") == 0
		|| strcmp(argv[i], "--im") == 0
		|| strcmp(argv[i], "--caption") == 0
		|| strcmp(argv[i], "--icon") == 0
		|| strcmp(argv[i], "--miniicon") == 0
		|| strcmp(argv[i], "--config") == 0
		|| strcmp(argv[i], "--dcopserver") == 0
		|| strcmp(argv[i], "--style") == 0
		|| strcmp(argv[i], "--geometry") == 0
		|| strcmp(argv[i], "--smkey") == 0
		|| strcmp(argv[i], "-smkey") == 0
		|| strcmp(argv[i], "-session") == 0
	   )
	{
	    gui_argv[gui_argc++] = argv[i];
	    gui_argv[gui_argc++] = argv[i + 1];
	    found = 2;
	}

	// remove from the list of argv
	if (found >= 1 && --*argc > i)
	{
	    mch_memmove(&argv[i], &argv[i + 1], (*argc - i) * sizeof(char *));
	    i--;
	}
    }
    KCmdLineArgs::init(gui_argc, gui_argv, "kvim",
			      I18N_NOOP("Vim inside KDE"), VIM_VERSION_SHORT);
}// }}}

/****************************************************************************
 * Focus handlers:
 */

/*
 * Initialises time intervals for the cursor blinking
 */
    void
gui_mch_set_blinking(long waittime, long on, long off)//{{{
{
    gui.w->set_blink_time(waittime, on, off);
}//}}}

/*
 * Stop the cursor blinking.  Show the cursor if it wasn't shown.
 */
    void
gui_mch_stop_blink()//{{{
{
    gui.w->stop_cursor_blinking();
}//}}}

/*
 * Start the cursor blinking.  If it was already blinking, this restarts the
 * waiting time and shows the cursor.
 */
    void
gui_mch_start_blink()//{{{
{
    gui.w->start_cursor_blinking();
}//}}}

#ifdef FEAT_MZSCHEME
    void
mzscheme_kde_start_timer()//{{{
{
    gui.w->enable_mzscheme_threads();
}//}}}
    void
mzscheme_kde_stop_timer()//{{{
{
    gui.w->disable_mzscheme_threads();
}//}}}
#endif

/*
 * Check if the GUI can be started.  Called before gvimrc is sourced.
 * Return OK or FAIL.
 */
    int
gui_mch_init_check(void)//{{{
{
    gui.dpy = qt_xdisplay();
    return OK;
}//}}}

/*
 * Initialise the X GUI.  Create all the windows, set up all the call-backs etc.
 * Returns OK for success, FAIL when the GUI can't be started.
 */
    int
gui_mch_init()//{{{
{
    (void) new KApplication();
    KApplication::kApplication()->dcopClient()->registerAs(
				 KApplication::kApplication()->name(), false);
    //    dbf("%s %s", KGlobal::locale()->language().latin1(), KLocale::defaultLanguage().latin1());

    vmw = new VimMainWindow("KVim", 0);
    vmw->setFrameBorderWidth(0);
    kapp->setMainWidget(vmw);
    kapp->setTopWidget(vmw);

    sbpool = new SBPool;

#if QT_VERSION>=300
    vmw->connect(kapp->clipboard(), SIGNAL(selectionChanged()),
				     vmw, SLOT(clipboard_selection_update()));
#endif
    vmw->connect(kapp->clipboard(), SIGNAL(dataChanged()),
					  vmw, SLOT(clipboard_data_update()));
    clip_lose_selection(&clip_plus);
    clip_lose_selection(&clip_star);

    gui.in_focus = FALSE; // will be updated

    if (reverse)
    {
	gui.def_norm_pixel = gui_get_color((char_u *)"White");
	gui.def_back_pixel = gui_get_color((char_u *)"Black");
#if QT_VERSION>=300
	gui.w->setEraseColor(QColor(Qt::black));
#else
	gui.w->setBackgroundColor(QColor(Qt::black));
#endif
    }
    else
    {
	gui.def_norm_pixel = gui_get_color((char_u *)"Black");
	gui.def_back_pixel = gui_get_color((char_u *)"White");
#if QT_VERSION>=300
	gui.w->setEraseColor(QColor(Qt::white));
#else
	gui.w->setBackgroundColor(QColor(Qt::white));
#endif
    }

    gui.norm_pixel = gui.def_norm_pixel;
    gui.back_pixel = gui.def_back_pixel;

    gui.border_width  = 1;
    gui.border_offset = 1;//gui.border_width;
    gui.scrollbar_width = SB_DEFAULT_WIDTH;
    gui.scrollbar_height = SB_DEFAULT_WIDTH;

    //gui.menu_height = vmw->menuBar()->height()+1;
    //gui.toolbar_height = vmw->toolBar()->height();

    return OK;
}//}}}


/*
 * Called when the foreground or background color has been changed.
 */
    void
gui_mch_new_colors()//{{{
{
    QColor rgb;
    rgb.setRgb(gui.back_pixel);
#if QT_VERSION>=300
    gui.w->setEraseColor(rgb);
#else
    gui.w->setBackgroundColor(rgb);
#endif
}//}}}

/*
 * Open the GUI window which was created by a call to gui_mch_init().
 */
    int
gui_mch_open()//{{{
{
    gui.dpy = qt_xdisplay();
    set_normal_colors();

    /* Check that none of the colors are the same as the background color */
    gui_check_colors();

    /* Get the colors for the highlight groups (gui_check_colors() might have
     * changed them).
     */
    highlight_gui_started();    /* re-init colors and fonts */
#ifdef FEAT_MENU
    vmw->w->menu = new QPopupMenu(vmw);

#if QT_VERSION>=300
    vmw->w->menu->insertItem(SmallIcon("ktip"), i18n("&Tip of the day..."),
						vmw, SLOT(showTipOfTheDay()));
    vmw->w->menu->insertSeparator();
#endif
    if (vmw->have_tearoff)
	vmw->w->menu->insertTearOffHandle(0, 0);
    vmw->w->menu->insertItem(i18n("&Report Bug ..."),
						  vmw, SLOT(showBugReport()));
    vmw->w->menu->insertSeparator();
    vmw->w->menu->insertItem(SmallIcon("kvim"), i18n("&About KVim..."),
					   vmw, SLOT(showAboutApplication()));
    vmw->w->menu->insertItem(SmallIcon("about_kde"), i18n("About &KDE..."),
						   vmw, SLOT(showAboutKDE()));
    vmw->menuBar()->insertItem("&KVim", vmw->w->menu);
#endif
    if (startfont != NULL)
	gui_mch_init_font((char_u*)startfont->latin1(), FALSE);

    if (startsize != NULL)
	vmw->resize(startsize->width(), startsize->height());

    gui_mch_update_codec();

    if (kapp->isRestored())
	if (KMainWindow::canBeRestored(1))
	    vmw->restore(1);

    vmw->show();
#if QT_VERSION>=300
    if (tip == 2)
	KTipDialog::showTip(vmw, QString::null, true);
    else if (tip == 0)
	KTipDialog::showTip(vmw);
#endif

    return OK;
}//}}}

    void
gui_mch_exit(int rc)//{{{
{
    kapp->quit();
}//}}}

/*
 * Get the position of the top left corner of the window.
 */
    int
gui_mch_get_winpos(int *x, int *y)//{{{
{
    *x = vmw->x();
    *y = vmw->y();
    return OK;
}//}}}

/*
 * Set the position of the top left corner of the window to the given
 * coordinates.
 */
    void
gui_mch_set_winpos(int x, int y)//{{{
{
    vmw->move(x, y);
}//}}}

/*
 * Set the windows size.
 * ->resize VimWidget
 * ->resize vmw (block any events generated from here)
 */
    void
gui_mch_set_shellsize(int width, int height,//{{{
		int min_width, int min_height,
		int base_width, int base_height)
{
    //resize VimWidget
    vmw->w->resize(width, height);

    //resize vmw
    int vheight, vwidth;
    vheight = height;
    vwidth = width;

    if (gui.which_scrollbars[SBAR_LEFT])
	vwidth += gui.scrollbar_width;
    if (gui.which_scrollbars[SBAR_RIGHT])
	vwidth += gui.scrollbar_width;
    if (gui.which_scrollbars[SBAR_BOTTOM])
	vheight += gui.scrollbar_height;

    if (vmw->menuBar()->isVisible() && vmw->menuBar()->isEnabled()
#if QT_VERSION>=300
	    && !vmw->menuBar()->isTopLevelMenu()
#endif
       )
	vheight += vmw->menuBar()->height();
#ifdef FEAT_TOOLBAR
    if (vmw->toolBar()->isVisible() && vmw->toolBar()->isEnabled()
	    && (vmw->toolBar()->barPos() == KToolBar::Top
		|| vmw->toolBar()->barPos() == KToolBar::Bottom))
	vheight += vmw->toolBar()->height();

    if (vmw->toolBar()->isVisible() && vmw->toolBar()->isEnabled()
	    && (vmw->toolBar()->barPos() == KToolBar::Left
		|| vmw->toolBar()->barPos() == KToolBar::Right))
	vwidth += vmw->toolBar()->width();
#endif
    vmw->lock();
    vmw->resize(vwidth, vheight);
    gui_mch_update();
    //size should be nearly perfect, update baseSize and sizeIncrement
    vmw->setBaseSize(base_width, vmw->menuBar()->height() + 1
			    + vmw->toolBar()->height() + gui.char_height * 2);
    vmw->setSizeIncrement((( int )(gui.char_width / 2) * 2), gui.char_height);
    vmw->unlock();
}//}}}


/*
 * The screen size is used to make sure the initial window doesn't get bigger
 * then the screen.  This subtracts some room for menubar, toolbar and window
 * decorations.
 */
    void
gui_mch_get_screen_dimensions(int *screen_w, int *screen_h)//{{{
{
    *screen_w = kapp->desktop()->width();
    *screen_h = kapp->desktop()->height();
}//}}}

#if defined(FEAT_MENU) || defined(PROTO)
    void
gui_mch_enable_menu(int showit)//{{{
{
    if (showit)
	vmw->menuBar()->show();
    else
	vmw->menuBar()->hide();
    vmw->resize(vmw->width(), vmw->height());
}//}}}
#endif


#if defined(FEAT_TOOLBAR) || defined(PROTO)
    void
gui_mch_show_toolbar(int showit)//{{{
{
    if (showit)
	vmw->toolBar()->show();
    else
	vmw->toolBar()->hide();
    vmw->resize(vmw->width(), vmw->height());
}//}}}
#endif

/*
 * Put up a font dialog and return the selected font name in allocated memory.
 * "oldval" is the previous value.
 * Return NULL when cancelled.
 */

    char_u *
gui_mch_font_dialog(char_u *oldval)//{{{
{
    QFont myFont(vmw->w->font());
    if (gui.fontname)
	gui.fontname = NULL;

    int result = KFontDialog::getFont(myFont, true);
    if (result != KFontDialog::Accepted)
	return NULL;

    //	myFont.setFixedPitch(true);
#if QT_VERSION>=300
    QString n = myFont.toString();
#else
    QString n = KVimUtils::toString(&myFont);
#endif
    n.replace(QRegExp(","), "/");
    gui.fontname = (char_u *)strdup((const char *)n);
    n.replace(QRegExp(" "), "\\ ");
    n = QString("To set this font as your default font for KVim, edit your ~/.gvimrc file and add the following lines : \nif has(\"gui_kde\")\nset guifont=")+n+QString("\nendif");// \n OR \n use the control center of KDE and choose the correct fixed font");

    //display a message box which explains how to save your font settings
    KMessageBox::information(vmw, n, "Font Selection", "kvimselectfont");

    return vim_strsave(gui.fontname);
}//}}}

/*
 * Initialise vim to use the font with the given name.
 * Return FAIL if the font could not be loaded, OK otherwise.
 */
    int
gui_mch_init_font(char_u * font_name, int fontset)//{{{
{
    QString fontname;
    GuiFont font = NULL;

    if (font_name == NULL)
    {
#if 0
#if QT_VERSION>=300
	KConfig *base = KGlobal::config();
#else
	KConfigBase *base = KGlobal::config();
#endif
	base->setGroup("General");
	if (!base->hasKey("fixed"))
	{
	    KMessageBox::error(KApplication::kApplication()->mainWidget(),"Cannot load default fixed font\n\nConfigure fonts in KDE Control Center.\n(Just click 'Choose...', 'OK' and then 'Apply')");
	    return FAIL;
	}
#if QT_VERSION>=300
	QString f = base->readEntry("fixed");
#else
	QFont ft = base->readFontEntry("fixed", NULL);
	QString f = KVimUtils::toString(&ft);
#endif
	font_name = (char_u*)strdup(f.latin1()); //latin1 ?
#else
	font_name = (char_u*)strdup("misc-fixed/10/-1/5/50/0/0/0/1/0");
#endif
    }
    fontname = (const char *)font_name;
    /*	fontname.replace(QRegExp("/"), ",");
	font = new QFont();
	font->fromString( fontname );
	*/
#ifdef FEAT_XFONTSET
    if (fontset)
	font = gui_mch_get_fontset(font_name, TRUE, TRUE);
    if (font == NULL)
#endif
	font = gui_mch_get_font(font_name, FALSE);

    if (font == NULL)
	return FAIL;
    if (fontname.contains('*') && fontname.contains('-'))
	return FAIL;

    gui_mch_free_font(gui.norm_font);
#ifdef FEAT_XFONTSET
    gui_mch_free_fontset(gui.fontset);
    gui.fontset = NOFONTSET;
    if (fontset)
    {
	gui.fontset = font;
	gui.norm_font = NOFONT;
    }
    else
#endif
	gui.norm_font = font;

    /* Compute the width of the character cell.  Some fonts include
     * double-width characters.  Use the width of ASCII characters to find
     * out if this is so. */
    QFontMetrics f(*font);
    int width_max = 0;
    for (char c = 32; c < 127; c++)
	if (width_max < f.width((QChar)c))
	    width_max = f.width((QChar)c);
    if (width_max <= f.maxWidth() / 2)
	width_max = f.maxWidth() / 2;
    gui.char_width  = width_max;
    gui.char_height = f.height() + p_linespace;
    gui.char_ascent = f.ascent() + p_linespace;

    //check values, just to make sure and avoid a crash
    if (gui.char_width <= 0)
	gui.char_width = 8;
    if (gui.char_height <= 0)
	gui.char_height = 1;

    hl_set_font_name(font_name);

    vmw->w->setFont(*font);

    return OK;
}//}}}

    GuiFont
gui_mch_get_font(char_u * name, int report_error)//{{{
{
    QString fontname((const char *)name);
    if (!gui.in_use || name == NULL)
	return NOFONT;
    if (fontname.contains('*') && fontname.contains('-'))
	return NOFONT; // XFLD names not allowed anymore
    QFont *myFont = new QFont();
    fontname.replace(QRegExp("/"), ",");
    //	myFont->setRawMode(TRUE);

#if QT_VERSION>=300
    myFont->fromString(fontname);
#else
    KVimUtils::fromString(myFont, fontname);
#endif
    myFont->setFixedPitch(true);
    if (!myFont->fixedPitch())
	dbf("Non fixed-width font");
    return (GuiFont) myFont;
}//}}}

#if defined(FEAT_EVAL) || defined(PROTO)
/*
 * Return the name of font "font" in allocated memory.
 * Don't know how to get the actual name, thus use the provided name.
 */
    char_u *
gui_mch_get_fontname(GuiFont font, char_u *name)//{{{
{
    if (name == NULL)
	return NULL;
    return vim_strsave(name);
}//}}}
#endif

/*
 * Set the current text font.
 * Since we create all GC on demand, we use just gui.current_font to
 * indicate the desired current font.
 */
    void
gui_mch_set_font(GuiFont font)//{{{
{
    gui.current_font = font;
    gui.w->painter->setFont(*(gui.current_font));
}//}}}

/*
 * If a font is not going to be used, free its structure.
 */
    void
gui_mch_free_font(GuiFont font)//{{{
{
    if (font)
	delete font; // this is a QFont , we can delete it :)
}//}}}

    GuiFontset
gui_mch_get_fontset(char_u *name, int report_error, int fixed_width)
{
    return (GuiFontset)gui_mch_get_font(name, report_error);
}

    void
gui_mch_set_fontset(GuiFontset fontset)
{
    gui_mch_set_font((GuiFont)fontset);
}

    void
gui_mch_free_fontset(GuiFontset fontset)
{
    if (fontset)
	delete fontset;
}

    void
gui_mch_settitle(char_u *title, char_u *icon)//{{{
{
    if (!gui.in_use)		/* can't do this when GUI not running */
	return;
    vmw->setPlainCaption((const char *)title);
    QPixmap p((const char *)icon);
    vmw->setIcon(p); //FIXME
}//}}}

/*
 * Return the Pixel value (color) for the given color name.  This routine was
 * pretty much taken from example code in the Silicon Graphics OSF/Motif
 * Programmer's Guide.
 * Return -1 for error.
 */
    guicolor_T
gui_mch_get_color(char_u * name)//{{{
{
    int i;
    static char *(vimnames[][2]) =
    {
	/* A number of colors that some X11 systems don't have */
	{"LightRed", "#FFA0A0"},
	{"LightGreen", "#80FF80"},
	{"LightMagenta", "#FFA0FF"},
	{"DarkCyan", "#008080"},
	{"DarkBlue", "#0000C0"},
	{"DarkRed", "#C00000"},
	{"DarkMagenta", "#C000C0"},
	{"DarkGrey", "#C0C0C0"},
	{NULL, NULL}
    };

    if (!gui.in_use)		/* can't do this when GUI not running */
	return (guicolor_T)(-1);

    QColor _color((const char *)name);

    if (_color.isValid())
    {
	// return (_color.red() << 16) + ((_color.green() << 8))
							 // + (_color.blue());
	return _color.rgb();
	// return (guicolor_T) _color.pixel();
    }

    /* add a few builtin names */
    for (i = 0;; ++i)
    {
	if (vimnames[i][0] == NULL)
	    return (guicolor_T)(-1);
	if (STRICMP(name, vimnames[i][0]) == 0)
	{
	    name = (char_u *) vimnames[i][1];
	    return gui_mch_get_color(name);
	}
    }

    return (guicolor_T)(-1); // dead code, should not be reached..
}//}}}

/*
 * Set the current text foreground color.
 */
    void
gui_mch_set_fg_color(guicolor_T color)//{{{
{
    QColor rgb;
    rgb.setRgb(color);
    gui.w->painter->setPen(rgb);
}//}}}

/*
 * Set the current text background color.
 */
    void
gui_mch_set_bg_color(guicolor_T color)//{{{
{
    QColor rgb;
    rgb.setRgb(color);
    gui.w->painter->setBackgroundColor(rgb);
}//}}}

/*
 * Use the blank mouse pointer or not.
 *
 * hide: TRUE = use blank ptr, FALSE = use parent ptr
 */
    void
gui_mch_mousehide(int hide)//{{{
{
    if (hide == gui.pointer_hidden)
	return;
    //#ifdef FEAT_MOUSESHAPE
    //	if (!hide) mch_set_mouse_shape(last_shape);
    //#else
# if (QT_VERSION<300)
    gui.w->setCursor((hide)?BlankCursor:ArrowCursor);
# else
    gui.w->setCursor((hide)?Qt::BlankCursor:Qt::ArrowCursor);
# endif
    //#endif
    gui.pointer_hidden = hide;
}//}}}

    void
gui_mch_update_codec()
{
#ifdef FEAT_MBYTE
    if (!gui.in_use)
	return;
    vmw->codec = QTextCodec::codecForName((const char *)p_enc);
    if (vmw->codec == NULL)
	vmw->codec = QTextCodec::codecForName(
		KVimUtils::convertEncodingName(QString((const char*)p_enc)));
    if (vmw->codec == NULL)
	vmw->codec = QTextCodec::codecForLocale();
#else
    vmw->codec = QTextCodec::codecForLocale();
#endif
    if (vmw->codec == NULL)
	vmw->codec = QTextCodec::codecForName("ISO-8859-1"); //fallback
}

    void
gui_mch_draw_string(int row, int col, char_u * s, int len, int flags)//{{{
{
    QString text = vmw->codec->toUnicode((const char *)s, len);
    gui.w->draw_string(TEXT_X(col), TEXT_Y(row), text, text.length(), flags);
}//}}}

#if defined(FEAT_TITLE) || defined(PROTO)
/*
 * Return the text window-id and display.  Only required for X-based GUI's
 */
    int
gui_get_x11_windis(Window * win, Display ** dis)//{{{
{
    *win = /*vmw*/gui.w->winId();
    *dis = qt_xdisplay();
    return OK;
}//}}}
#endif

    void
gui_mch_beep()//{{{
{
    kapp->beep();
}//}}}

    void
gui_mch_flash(int msec)//{{{
{
    gui.w->flash();
}//}}}

/*
 * Invert a rectangle from row r, column c, for nr rows and nc columns.
 */
    void
gui_mch_invert_rectangle(int r, int c, int nr, int nc)//{{{
{
    bitBlt(gui.w,
	    FILL_X(c), FILL_Y(r),
	    gui.w,
	    FILL_X(c), FILL_Y(r),
	    (nc) * gui.char_width,
	    (nr) * gui.char_height,
	    Qt::NotROP,		// raster Operation
	    true);		// ignoreMask
}//}}}

/*
 * Iconify the GUI window.
 */
    void
gui_mch_iconify()//{{{
{
    vmw->showMinimized();
}//}}}

/*
 * Draw a cursor without focus.
 */
    void
gui_mch_draw_hollow_cursor(guicolor_T color)//{{{
{
    QPainter p(gui.w);
    p.setPen(color);

    p.drawRect(FILL_X(gui.col), FILL_Y(gui.row), gui.char_width - 1,
							 gui.char_height - 1);
    p.end();
}//}}}

/*
 * Draw part of a cursor, "w" pixels wide, and "h" pixels high, using
 * color "color".
 */
    void
gui_mch_draw_part_cursor(int w, int h, guicolor_T color)//{{{
{
    QPainter p(gui.w);
    p.setPen(color);
    p.fillRect(
	    FILL_X(gui.col),
	    FILL_Y(gui.row) + gui.char_height - h + 1,
	    w, h - 2, QColor( color, color));
    p.drawRect(FILL_X(gui.col), FILL_Y(gui.row) + gui.char_height - h
						       + (int)p_linespace / 2,
	    w, h - (int)p_linespace);
}//}}}


/*
 * Catch up with any queued X11 events.  This may put keyboard input into the
 * input buffer, call resize call-backs, trigger timers etc.  If there is
 * nothing in the X11 event queue (& no timers pending), then we return
 * immediately.
 */
    void
gui_mch_update()//{{{
{
    kapp->processEvents();
}//}}}


/*
 * GUI input routine called by gui_wait_for_chars().  Waits for a character
 * from the keyboard.
 *  wtime == -1     Wait forever.
 *  wtime == 0      This should never happen.
 *  wtime > 0       Wait wtime milliseconds for a character.
 * Returns OK if a character was found to be available within the given time,
 * or FAIL otherwise.
 */
    int
gui_mch_wait_for_chars(long wtime)//{{{
{
    // malte@kde.org's  gift to KVim ;), thanks to him :) for this hard to find bug
    if (wtime > 0)
    {
	gui.w->wait(wtime);
	while (vim_is_input_buf_empty() && !gui.w->wait_done)
	    kapp->processOneEvent();
	return vim_is_input_buf_empty() ? FAIL : OK;
    }
    else
	while (vim_is_input_buf_empty())
	    kapp->processOneEvent();

    return OK;
}//}}}


/****************************************************************************
 * Output drawing routines.
 ****************************************************************************/


/* Flush any output to the screen */
    void
gui_mch_flush()//{{{
{
    kapp->flushX();
}//}}}

/*
 * Clear a rectangular region of the screen from text pos (row1, col1) to
 * (row2, col2) inclusive.
 */
    void
gui_mch_clear_block(int row1, int col1, int row2, int col2)//{{{
{
    gui.w->erase(FILL_X(col1), FILL_Y(row1),
	    (col2 - col1 + 1) * gui.char_width + (col2 == Columns - 1),
	    (row2 - row1 + 1) * gui.char_height);
}//}}}

    void
gui_mch_clear_all(void)//{{{
{
    gui.w->erase();
}//}}}


/*
 * Delete the given number of lines from the given row, scrolling up any
 * text further down within the scroll region.
 */
    void
gui_mch_delete_lines(int row, int num_lines)//{{{
{
    if (num_lines <= 0)
	return;

    if (row + num_lines > gui.scroll_region_bot)
    {
	/* Scrolled out of region, just blank the lines out */
	gui_clear_block(row, gui.scroll_region_left, gui.scroll_region_bot,
						     gui.scroll_region_right);
    }
    else
    {
	bitBlt(gui.w,
		FILL_X(gui.scroll_region_left), FILL_Y(row),
		gui.w,
		FILL_X(gui.scroll_region_left), FILL_Y(row + num_lines),
		gui.char_width * (gui.scroll_region_right
					    - gui.scroll_region_left + 1) + 1,
		gui.char_height * (gui.scroll_region_bot - row - num_lines + 1),
		Qt::CopyROP,	    // raster Operation
		true);		    // ignoreMask

	/* Update gui.cursor_row if the cursor scrolled or copied over */
	if (gui.cursor_row >= row)
	{
	    if (gui.cursor_row < row + num_lines)
		gui.cursor_is_valid = FALSE;
	    else if (gui.cursor_row <= gui.scroll_region_bot)
		gui.cursor_row -= num_lines;
	}

	gui_clear_block(gui.scroll_region_bot - num_lines + 1,
		gui.scroll_region_left,
		gui.scroll_region_bot, gui.scroll_region_right);
    }
}//}}}

/*
 * Insert the given number of lines before the given row, scrolling down any
 * following text within the scroll region.
 */
    void
gui_mch_insert_lines(int row, int num_lines)//{{{
{
    if (num_lines <= 0)
	return;

    if (row + num_lines > gui.scroll_region_bot)
    {
	/* Scrolled out of region, just blank the lines out */
	gui_clear_block(row, gui.scroll_region_left, gui.scroll_region_bot,
						 gui.scroll_region_right - 1);
    }
    else
    {
	bitBlt(gui.w,
		FILL_X(gui.scroll_region_left), FILL_Y(row + num_lines),
		gui.w,
		FILL_X(gui.scroll_region_left), FILL_Y(row),
		gui.char_width * ( gui.scroll_region_right
					   - gui.scroll_region_left + 1 ) + 1,
		gui.char_height * (gui.scroll_region_bot - row - num_lines + 1),
		Qt::CopyROP,	    // raster Operation
		true);		    // ignoreMask

	/* Update gui.cursor_row if the cursor scrolled or copied over */
	if (gui.cursor_row >= gui.row)
	{
	    if (gui.cursor_row <= gui.scroll_region_bot - num_lines)
		gui.cursor_row += num_lines;
	    else if (gui.cursor_row <= gui.scroll_region_bot)
		gui.cursor_is_valid = FALSE;
	}

	gui_clear_block(row, gui.scroll_region_left, row + num_lines - 1,
						     gui.scroll_region_right);
    }
}//}}}

/*
 * X Selection stuff, for cutting and pasting text to other windows.
 */
    void
clip_mch_request_selection(VimClipboard *cbd)//{{{
{
#if QT_VERSION>=300
    if (cbd == &clip_star)
	kapp->clipboard()->setSelectionMode(true);
#endif
    QString selection = kapp->clipboard()->text();

    QCString unistring = vmw->codec->fromUnicode(selection);
    clip_yank_selection(MCHAR, (char_u *)(const char*)unistring,
					      (long)unistring.length(), cbd);
#if QT_VERSION>=300
    if (cbd == &clip_star)
	kapp->clipboard()->setSelectionMode(false);
#endif
}//}}}

    void
clip_mch_lose_selection(VimClipboard *cbd)//{{{
{
    // Don't need to do anything here
    gui_mch_update();
}//}}}

/*
 * Check whatever we allready own the selection.
 */
    int
clip_mch_own_selection(VimClipboard *cbd)//{{{
{
    if (kapp->clipboard()->ownsSelection())
	return OK;
    else
    {
#if QT_VERSION>=300
	kapp->clipboard()->setSelectionMode(true);
#endif
	return OK;
    }
}//}}}

/*
 * Send the current selection to the clipboard.
 */
    void
clip_mch_set_selection(VimClipboard *cbd)//{{{
{
    char_u *data;
    long_u length;

    clip_get_selection(cbd);
    if (clip_convert_selection(&data, &length, cbd) < 0)
	return;

    QString selection((const char *)data);
    // We must turncate the string because it is not
    // null terminated
    selection.truncate((uint) length);

#if QT_VERSION>=300
    if (cbd == &clip_star)
	kapp->clipboard()->setSelectionMode(true);
#endif
    kapp->clipboard()->setText(selection);
#if QT_VERSION>=300
    kapp->clipboard()->setSelectionMode(false);
#endif
}//}}}


#if defined(FEAT_MENU) || defined(PROTO)
/*
 * Make a menu item appear either active or not active (grey or not grey).
 */
    void
gui_mch_menu_grey(vimmenu_T * menu, int grey)//{{{
{
    if (!menu || !menu->parent || !menu->parent->widget)
	return;
    menu->parent->widget->setItemEnabled((long)menu, !grey);
    gui_mch_update();
}//}}}

/*
 * Make menu item hidden or not hidden.
 */
    void
gui_mch_menu_hidden(vimmenu_T * menu, int hidden)//{{{
{
    // FIXME: cannot be fixed AFAIK
    // it's hard to remove an item in a QPopupMenu
    gui_mch_menu_grey(menu, hidden);
}//}}}

/*
 * This is called after setting all the menus to grey/hidden or not.
 */
    void
gui_mch_draw_menubar()//{{{
{
    // nothing to do under kde
}//}}}
#endif

/*
 * Scrollbar stuff.
 */
    void
gui_mch_enable_scrollbar(scrollbar_T *sb, int flag)//{{{
{
    if (!sb->w)
	return;
    int width = gui.w->width();
    int height = gui.w->height();
    int neww = vmw->width();
    int newh = vmw->height();

    if (gui.which_scrollbars[SBAR_LEFT])
	width += gui.scrollbar_width;
    if (gui.which_scrollbars[SBAR_RIGHT])
	width += gui.scrollbar_width;
    if (gui.which_scrollbars[SBAR_BOTTOM])
	height += gui.scrollbar_height;

    if (vmw->menuBar()->isVisible() && vmw->menuBar()->isEnabled()
#if QT_VERSION>=300
	    && !vmw->menuBar()->isTopLevelMenu()
#endif
       )
	height += vmw->menuBar()->height();
#ifdef FEAT_TOOLBAR
    if (vmw->toolBar()->isVisible() && vmw->toolBar()->isEnabled()
	    && (vmw->toolBar()->barPos() == KToolBar::Top
		|| vmw->toolBar()->barPos() == KToolBar::Bottom))
	height += vmw->toolBar()->height();

    if (vmw->toolBar()->isVisible() && vmw->toolBar()->isEnabled()
	    && (vmw->toolBar()->barPos() == KToolBar::Left
		|| vmw->toolBar()->barPos() == KToolBar::Right))
	width += vmw->toolBar()->width();
#endif
    if (abs(vmw->width() - width) > 5
			 && (sb->type == SBAR_LEFT || sb->type == SBAR_RIGHT))
	neww = width;
    if (abs(vmw->height() - height) > 5 && (sb->type == SBAR_BOTTOM))
	newh = height;

    if (flag)
	sb->w->show();
    else
	sb->w->hide();
    gui_mch_update();
    vmw->lock();
    vmw->resize(neww, newh);
    vmw->unlock();
    gui_mch_update();
}//}}}

/*
 * Return the RGB value of a pixel as "#RRGGBB".
 */
    long_u
gui_mch_get_rgb(guicolor_T pixel)//{{{
{
    //	QColor c(pixel, pixel);
    //	return (c.red() << 16) +  ((c.green() << 8)) + (c.blue());
    return pixel;
    // funny no ? it looks like with Qt we can always use directly the rgb
    // value (i hope i don't break colors again ;p)
}//}}}

/*
 * Get current y mouse coordinate in text window.
 * Return -1 when unknown.
 */
    int
gui_mch_get_mouse_x(void)//{{{
{
    return vmw->mapFromGlobal(QCursor::pos()).x();
}//}}}

    int
gui_mch_get_mouse_y(void)//{{{
{
    return vmw->mapFromGlobal(QCursor::pos()).y();
}//}}}

    void
gui_mch_setmouse(int x, int y)//{{{
{
    QCursor::setPos(vmw->mapToGlobal(QPoint(x, y)));
}//}}}

#if defined(FEAT_MOUSESHAPE) || defined(PROTO)
#if QT_VERSION>=300
static int mshape_ids[] = {//{{{
	Qt::ArrowCursor,		/* arrow */
	Qt::BlankCursor,		/* blank */
	Qt::IbeamCursor,		/* beam */
	Qt::SizeVerCursor,		/* updown */
	Qt::SplitHCursor,		/* udsizing */
	Qt::SizeHorCursor,		/* leftright */
	Qt::SizeHorCursor,		/* lrsizing */
	Qt::WaitCursor,			/* busy */
	Qt::ForbiddenCursor,		/* no */
	Qt::CrossCursor,		/* crosshair */
	Qt::PointingHandCursor,		/* hand1 */
	Qt::PointingHandCursor,		/* hand2 */
	Qt::ArrowCursor,		/* pencil */
	Qt::WhatsThisCursor,		/* question */
	Qt::ArrowCursor,		/* right-arrow */
	Qt::UpArrowCursor,		/* up-arrow */
	Qt::ArrowCursor			/* last one */
};//}}}
#else
static int mshape_ids[] = {//{{{
	ArrowCursor,		/* arrow */
	BlankCursor,		/* blank */
	IbeamCursor,		/* beam */
	SizeVerCursor,		/* updown */
	SplitHCursor,		/* udsizing */
	SizeHorCursor,		/* leftright */
	SizeHorCursor,		/* lrsizing */
	WaitCursor,		/* busy */
	ForbiddenCursor,	/* no */
	CrossCursor,		/* crosshair */
	PointingHandCursor,	/* hand1 */
	PointingHandCursor,	/* hand2 */
	ArrowCursor,		/* pencil */
	ArrowCursor,		/* question */
	ArrowCursor,		/* right-arrow */
	UpArrowCursor,		/* up-arrow */
	ArrowCursor		/* last one */
};//}}}
#endif

    void
mch_set_mouse_shape (int shape)//{{{
{
    int		   id;

    if (shape == MSHAPE_HIDE || gui.pointer_hidden)
#if QT_VERSION>=300
	gui.w->setCursor(Qt::BlankCursor);
#else
	gui.w->setCursor(BlankCursor);
#endif
    else
    {
	if (shape >= MSHAPE_NUMBERED)
	{
	    id = shape - MSHAPE_NUMBERED;
	    /*		if (id >= GDK_NUM_GLYPHS)
			id = GDK_LEFT_PTR;
			else
			id &= ~1;*/	/* they are always even (why?) */
	    id &= -1;
	}
	else
	    id = mshape_ids[shape];

	gui.w->setCursor(id);
    }
    if (shape != MSHAPE_HIDE)
	last_shape = shape;
}//}}}
#endif

    int
gui_mch_adjust_charsize ()//{{{
{
    QFont f(*(gui.current_font));
    QFontMetrics fm(f);
    gui.char_height = fm.height() + p_linespace;
    //gui.char_height = fm.ascent() + fm.descent() + p_linespace;
    gui.char_ascent = fm.ascent() + p_linespace / 2;

    return OK;
}//}}}

    void
gui_mch_set_foreground ()//{{{
{
    KWin::setActiveWindow(vmw->winId());
}//}}}
