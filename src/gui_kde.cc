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
#include <errno.h>
#include <string.h>
#include <kmenubar.h>
#include <kfiledialog.h>
#include <kiconloader.h>

#include <qscrollbar.h>
#include <qcursor.h>
#include <qmessagebox.h>
#include <qiconset.h>
#include <qtextcodec.h>
#include "gui_kde_wid.h"

extern "C" {
#include "vim.h"
}

#undef dbf
#undef db
#undef mputs

#if 1
#define dbf(format, args...) { printf("%s" " : " format "\n" , __FUNCTION__ , ## args ); fflush(stdout); }
#define db()       { printf("%s\n", __FUNCTION__ );fflush(stdout); }
#else
#define dbf(format, args... )
#define db()
#endif


#ifdef FEAT_TOOLBAR
#ifndef FEAT_KDETOOLBAR
/*
 * Icons used by the toolbar code.
 *///{{{
#include "../pixmaps/tb_new.xpm"
#include "../pixmaps/tb_open.xpm"
#include "../pixmaps/tb_close.xpm"
#include "../pixmaps/tb_save.xpm"
#include "../pixmaps/tb_print.xpm"
#include "../pixmaps/tb_cut.xpm"
#include "../pixmaps/tb_copy.xpm"
#include "../pixmaps/tb_paste.xpm"
#include "../pixmaps/tb_find.xpm"
#include "../pixmaps/tb_find_next.xpm"
#include "../pixmaps/tb_find_prev.xpm"
#include "../pixmaps/tb_find_help.xpm"
#include "../pixmaps/tb_exit.xpm"
#include "../pixmaps/tb_undo.xpm"
#include "../pixmaps/tb_redo.xpm"
#include "../pixmaps/tb_help.xpm"
#include "../pixmaps/tb_macro.xpm"
#include "../pixmaps/tb_make.xpm"
#include "../pixmaps/tb_save_all.xpm"
#include "../pixmaps/tb_jump.xpm"
#include "../pixmaps/tb_ctags.xpm"
#include "../pixmaps/tb_load_session.xpm"
#include "../pixmaps/tb_save_session.xpm"
#include "../pixmaps/tb_new_session.xpm"
#include "../pixmaps/tb_blank.xpm"
#include "../pixmaps/tb_maximize.xpm"
#include "../pixmaps/tb_split.xpm"
#include "../pixmaps/tb_minimize.xpm"
#include "../pixmaps/tb_shell.xpm"
#include "../pixmaps/tb_replace.xpm"
#include "../pixmaps/tb_vsplit.xpm"
#include "../pixmaps/tb_maxwidth.xpm"
#include "../pixmaps/tb_minwidth.xpm"
//}}}
/*
 * These are the pixmaps used for the default buttons.
 * Order must exactly match toolbar_names[] in menu.c!
 *///{{{
static char **(built_in_pixmaps[]) =
{
	tb_new_xpm,
	tb_open_xpm,
	tb_save_xpm,
	tb_undo_xpm,
	tb_redo_xpm,
	tb_cut_xpm,
	tb_copy_xpm,
	tb_paste_xpm,
	tb_print_xpm,
	tb_help_xpm,
	tb_find_xpm,
	tb_save_all_xpm,
	tb_save_session_xpm,
	tb_new_session_xpm,
	tb_load_session_xpm,
	tb_macro_xpm,
	tb_replace_xpm,
	tb_close_xpm,
	tb_maximize_xpm,
	tb_minimize_xpm,
	tb_split_xpm,
	tb_shell_xpm,
	tb_find_prev_xpm,
	tb_find_next_xpm,
	tb_find_help_xpm,
	tb_make_xpm,
	tb_jump_xpm,
	tb_ctags_xpm,
	tb_vsplit_xpm,
	tb_maxwidth_xpm,
	tb_minwidth_xpm,
	tb_exit_xpm
};//}}}
#else
const char *kdeicons[] = {
	"filenew",
	"fileopen",
	"filesave",
	"undo",
	"redo",
	"editcut",
	"editcopy",
	"editpaste",
	"fileprint",
	"contents2",
	"filefind",
	"save_all",
	"fileexport",
	"filenew",
	"fileimport",
	"run",
	"edit",
	"fileclose",
	"",
	"",
	"split",
	"openterm",
	"previous",
	"next",
	"help",
	"make",
	"goto",
	"run",
	"vsplit",
	"maxwidth",
	"minwidth",
	"quit"
};
#endif
/*
 * creates a blank pixmap using tb_blank
 */
    QPixmap
pixmap_create_from_xpm(char **xpm)//{{{
{
    return (QPixmap((const char **)xpm));
}//}}}

/*
 * creates a pixmap by using a built-in number
 */
    QPixmap
pixmap_create_by_num(int pixmap_num)//{{{
{
#ifdef FEAT_KDETOOLBAR
    if (pixmap_num >= 0 && (unsigned)pixmap_num < (sizeof(kdeicons)
						   / sizeof(kdeicons[0])) - 1)
    {

	KIconLoader *il = kapp->iconLoader(); //new KIconLoader();
	QString icon;
	icon = QString(kdeicons[pixmap_num]);
	return il->loadIcon(icon, KIcon::MainToolbar);
    }
    return QPixmap();
#else
    if (pixmap_num >= 0 && (unsigned)pixmap_num < (sizeof(built_in_pixmaps)
					   / sizeof(built_in_pixmaps[0])) - 1)
	return pixmap_create_from_xpm(built_in_pixmaps[pixmap_num]);
    else
	return QPixmap();
#endif
}//}}}

/*
 * Creates a pixmap by using the pixmap "name" found in 'runtimepath'/bitmaps/
 */
    QPixmap
pixmap_create_by_dir(char_u *name)//{{{
{
    char_u full_pathname[MAXPATHL + 1];

    if (gui_find_bitmap(name, full_pathname, "xpm") == OK)
	return QPixmap((const char *)full_pathname);
    else
	return QPixmap();
}//}}}


    QPixmap
pixmap_create_from_file(char_u *file)
{
    return QPixmap((const char *)file);
}
#endif

    void
gui_mch_add_menu(vimmenu_T *menu, int idx)//{{{
{
#ifdef FEAT_MENU
    QPopupMenu *me;
    vimmenu_T *parent = menu->parent;

    if (menu_is_popup(menu->name))
    {
	menu->widget = new QPopupMenu(vmw , QSTR(menu->name));
	QObject::connect(menu->widget, SIGNAL(activated(int)), vmw,
						   SLOT(menu_activated(int)));
	return;
    }

    if (!menu_is_menubar(menu->name))
	return;

    if (parent)
    {
	idx++; // for tearoffs to be first in menus
	me = new QPopupMenu(parent->widget, QSTR(menu->name));
	parent->widget->insertItem(QSTR(menu->name), me, (int)me, idx);
    }
    else
    {
	me = new QPopupMenu(vmw->menuBar(), QSTR(menu->name));
	vmw->menuBar()->insertItem(QSTR(menu->name), me, (int)me, idx);
    }

    me->setCaption((const char *)(menu->dname));
    if (vmw->have_tearoff)
	me->insertTearOffHandle(0, 0);
    QObject::connect(me, SIGNAL(activated(int)), vmw,
						   SLOT(menu_activated(int)));
    menu->widget = me;
#endif
}//}}}


    void
gui_mch_add_menu_item(vimmenu_T *menu, int idx)//{{{
{
#ifdef FEAT_MENU
    vimmenu_T *parent = menu->parent;
#ifdef FEAT_TOOLBAR
    if (menu_is_toolbar(parent->name))
    {
	QPixmap pix;

	if (menu_is_separator(menu->name))
	{
	    vmw->toolBar()->insertSeparator();
	    return;
	}
	if (menu->iconfile != NULL)
	{
	    pix = pixmap_create_from_file(menu->iconfile);
	}
	if (!menu->icon_builtin)
	{
	    pix = pixmap_create_by_dir(menu->name);
	}
	if (pix.isNull() && menu->iconidx >= 0)
	{
	    pix = pixmap_create_by_num(menu->iconidx);
	}
#ifndef FEAT_KDETOOLBAR
	if (pix.isNull())
	{
	    pix = pixmap_create_from_xpm(tb_blank_xpm);
	}
#endif
	if (pix.isNull())
	    return; // failed
	vmw->toolBar()->insertButton(
		pix,
		(int)menu, // id
		true,
		QSTR(menu->strings[MENU_INDEX_TIP]), // tooltip or text
		idx);
	menu->parent=parent;
	return;
    }
#endif // FEAT_TOOLBAR

    idx++;
    if (menu_is_separator(menu->name))
    {
	parent->widget->insertSeparator();
	return;
    }
    parent->widget->insertItem(QSTR(menu->name), (int)menu, idx);
#endif
}//}}}


    void
gui_mch_set_text_area_pos(int x, int y, int w, int h)//{{{
{
    int X = 0;
    int Y = 0;

    if (vmw->menuBar()->isVisible() && vmw->menuBar()->isEnabled()
#if QT_VERSION>=300
	    && !vmw->menuBar()->isTopLevelMenu()
#endif
       )
	Y += vmw->menuBar()->height();
#ifdef FEAT_TOOLBAR
    if (vmw->toolBar()->isVisible() && vmw->toolBar()->isEnabled()
				   && vmw->toolBar()->barPos()==KToolBar::Top)
	Y += vmw->toolBar()->height();

    if (vmw->toolBar()->isVisible() && vmw->toolBar()->isEnabled()
				  && vmw->toolBar()->barPos()==KToolBar::Left)
	X += vmw->toolBar()->width();
#endif // FEAT_TOOLBAR

    gui.w->setGeometry(x + X, y + Y, w, h);
}//}}}


#if defined(FEAT_MENU) || defined(PROTO)
/*
 * Enable or disable mnemonics for the toplevel menus.
 */
    void
gui_gtk_set_mnemonics(int enable)//{{{ // TO BE REMOVED
{
}//}}}

    void
toggle_tearoffs(vimmenu_T *menu, int enable)//{{{
{
    while (menu != NULL)
    {
	if (!menu_is_popup(menu->name))
	{
	    if (menu->widget != 0)
	    {
		if (enable)
		    menu->widget->insertTearOffHandle(0,0);
		else
		    menu->widget->removeItem(0);
	    }
	    toggle_tearoffs(menu->children, enable);
	}
	menu = menu->next;
    }
}//}}}

	void
gui_mch_toggle_tearoffs(int enable)//{{{
{
    vmw->have_tearoff=enable;
    toggle_tearoffs(root_menu, enable);
}//}}}
#endif


#if defined(FEAT_MENU) || defined(PROTO)
/*
 * Destroy the machine specific menu widget.
 */
    void
gui_mch_destroy_menu(vimmenu_T *menu)//{{{
{
#ifdef FEAT_TOOLBAR
    if (menu->parent && menu_is_toolbar(menu->parent->name))
    {
	vmw->toolBar()->removeItem((int)menu);
	return;
    }
#endif
    if (menu->parent)
	menu->parent->widget->removeItem((int)menu);
    if (menu->widget)
	delete menu->widget;
    menu->widget = 0;
}//}}}
#endif /* FEAT_MENU */


/*
 * Scrollbar stuff.
 */

    void
gui_mch_set_scrollbar_thumb(scrollbar_T *sb, long val, long size, long max)//{{{
{
    if (!sb->w)
	return;

    sb->w->setRange(0, max + 1 - size);
    sb->w->setValue(val);

    sb->w->setLineStep(1);
    sb->w->setPageStep(size);
}//}}}

    void
gui_mch_set_scrollbar_pos(scrollbar_T *sb, int x, int y, int w, int h)//{{{
{
    if (!sb->w)
	return;

    //we add the menubar and toolbar height/width
    int X = 0;
    int Y = 0;

    if (vmw->menuBar()->isVisible() && vmw->menuBar()->isEnabled()
#if QT_VERSION>=300
					  && !vmw->menuBar()->isTopLevelMenu()
#endif
       )
	Y += vmw->menuBar()->height();
#ifdef FEAT_TOOLBAR
    if (vmw->toolBar()->isVisible() && vmw->toolBar()->isEnabled()
				   && vmw->toolBar()->barPos()==KToolBar::Top)
	Y += vmw->toolBar()->height();

    if (vmw->toolBar()->isVisible() && vmw->toolBar()->isEnabled()
				  && vmw->toolBar()->barPos()==KToolBar::Left)
	X += vmw->toolBar()->width();
#endif //FEAT_TOOLBAR
    if (sb->w->orientation() == Qt::Vertical)
    {
	bool leftscroll=gui.which_scrollbars[SBAR_LEFT];
	bool rightscroll=gui.which_scrollbars[SBAR_RIGHT];

	if (x < 20)
	    leftscroll = true;
	else
	    rightscroll = true;
	if (x < 20)
	    sb->w->setGeometry(X, y+Y, w, h);
	else
	    sb->w->setGeometry(vmw->width() - w - 1 + X, y + Y, w, h);
    }
    else
    {
	sb->w->setGeometry(x + X, y + Y, w, h);
    }
}//}}}

/* SBAR_VERT or SBAR_HORIZ */
    void
gui_mch_create_scrollbar(scrollbar_T *sb, int orient)//{{{
{
    sbpool->create(sb,orient);
    if (orient == SBAR_VERT)
	gui.scrollbar_width = sb->w->sizeHint().width();
    else
	gui.scrollbar_height = sb->w->sizeHint().height();
}//}}}

    void
gui_mch_destroy_scrollbar(scrollbar_T *sb)//{{{
{
    sbpool->destroy(sb);
}//}}}

#if defined(FEAT_BROWSE) || defined(PROTO)
/*
 * Implementation of the file selector related stuff
 */

/*
 * Convert the Vim-style filter specification 's' to the KDE-style
 * filter specification.
 *      Vim-style:      {label}\t{pattern1};{pattern2}\n
 *      KDE-style:      {pattern1} {pattern2}|{label}\n
 *
 * The newly constructed filter is returned in allocated memory and
 * must be freed by the calling program.
 */
    static char *
convert_filter(char_u *s)
{
    char	*res;
    unsigned	i;
    unsigned	pattern_len;
    char	*filter_label;
    char	*filter_pattern;

    // The conversion generates a string of equal length to the original
    // pattern, so allocate enough memory to hold the original string.
    res = new char[STRLEN(s) + 1];
    s = vim_strsave(s);
    if (res != NULL && s != NULL)
    {
	// Make sure the first byte is a NUL so that strcat()
	// will append at the beginning of the string.
	res[0] = '\0';
	filter_label = strtok((char *) s, "\t");
	while (filter_label != NULL)
	{
	    filter_pattern = strtok( 0L, "\n");
	    if (filter_pattern != NULL)
	    {
		pattern_len = (unsigned) STRLEN(filter_pattern);
		for (i = 0; i < pattern_len; ++i)
		    if (filter_pattern[i] == ';')
			filter_pattern[i] = ' ';

		strcat(res, filter_pattern);
		strcat(res, "|");
		strcat(res, filter_label);
		strcat(res, "\n");
	    }
	    filter_label = strtok(0L, "\t");
	}
    }
    if (s)
	vim_free(s);
    return res;
}

/*
 * Put up a file requester.
 * Returns the selected name in allocated memory, or NULL for Cancel.
 * saving,			select file to write
 * title			title for the window
 * dflt				default name
 * ext				not used (extension added)
 * initdir			initial directory, NULL for current dir
 * filter			not used (file name filter)
 */
/*ARGSUSED*/
    char_u *
gui_mch_browse(int saving,//{{{
		char_u *title,
		char_u *dflt,
		char_u *ext,
		char_u *initdir,
		char_u *filter)
{
    char *filt_glob;

    filt_glob = convert_filter(filter);

    gui_mch_mousehide(FALSE);

    QString s;
    if (!saving)
	s = KFileDialog::getOpenFileName(QSTR(initdir), QSTR(filt_glob),
							  vmw, QSTR(title));
    else
	s = KFileDialog::getSaveFileName();

    if (filt_glob)
	delete filt_glob;

    if (s.isNull())
	return NULL;
    QCString unistring = vmw->codec->fromUnicode(s);
    char_u *s2 = (char_u *)(const char *)unistring;
    if (s2)
	s2 = vim_strsave(s2);

    return s2;
}//}}}

#endif	/* FEAT_BROWSE */

#ifdef FEAT_GUI_DIALOG

/* ARGSUSED */
    int
gui_mch_dialog(int type,		/* type of dialog *///{{{
		char_u *title,		/* title of dialog */
		char_u *message,	/* message text */
		char_u *buttons,	/* names of buttons */
		int    def_but,		/* default button */
		char_u *textfield)
{
    gui_mch_mousehide(FALSE);
    VimDialog vd(type, title, message, buttons, def_but,textfield);
    int ret = vd.exec();
    return ret;
}//}}}


#endif	/* FEAT_GUI_DIALOG */

#if defined(FEAT_MENU) || defined(PROTO)
    void
gui_mch_show_popupmenu(vimmenu_T *menu)//{{{
{
    menu->widget->popup(QCursor::pos());
}//}}}

void
gui_make_popup (char_u *pathname)//{{{
{
    vimmenu_T *menu = gui_find_menu(pathname);

    if (menu != NULL)
	menu->widget->popup(QCursor::pos());
}//}}}
#endif



/* Find and Replace implementations */
    void
gui_mch_find_dialog(exarg_T *eap)//{{{
{
    // char_u* entry_text;
    //int exact_word=FALSE;
    //    entry_text = get_find_dialog_text(eap->arg,&exact_word);

    vmw->finddlg->setCaseSensitive(true);

    /*    if (entry_text!=NULL)
     *    {
	  vmw->finddlg->setText(QString((char *)entry_text));
    // exact match should go there, hopefully KDE old KEdFind/KEdReplace will be replaced in KDE 4 as pple wanted KDE 3's Find/Replace to be kept
    }*/ // Don't use it, KDE keeps old search in memory and vim give \\Csearch, which is difficult to handle
    //   vim_free(entry_text);

    vmw->finddlg->show();
}//}}}

    void
gui_mch_replace_dialog(exarg_T *eap)//{{{
{
    //  char_u* entry_text;
    //int exact_word=FALSE;

    //    entry_text = get_find_dialog_text(eap->arg,&exact_word);

    /*    if (entry_text!=NULL)
     *    {
     vmw->repldlg->setText(QString((char *)entry_text));
    // exact match should go there, hopefully KDE old KEdFind/KEdReplace will be replaced in KDE 4 as pple wanted KDE 3's Find/Replace to be kept
    }*/
    //vim_free(entry_text);

    vmw->repldlg->show();
}//}}}

    void
ex_helpfind(exarg_T *eap)//{{{
{
    do_cmdline_cmd((char_u *)"emenu ToolBar.FindHelp");
}//}}}
