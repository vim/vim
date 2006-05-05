/* vi:set ts=8 sts=4 sw=4:
 *
 * VIM - Vi IMproved		by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */

/*
 * Porting to GTK+ was done by:
 *
 * (C) 1998,1999,2000 by Marcin Dalecki <martin@dalecki.de>
 *
 * With GREAT support and continuous encouragements by Andy Kahn and of
 * course Bram Moolenaar!
 *
 * Support for GTK+ 2 was added by:
 *
 * (C) 2002,2003  Jason Hildebrand  <jason@peaceworks.ca>
 *		  Daniel Elstner  <daniel.elstner@gmx.net>
 *
 * Best supporting actor (He helped somewhat, aesthetically speaking):
 * Maxime Romano <verbophobe@hotmail.com>
 */

#ifdef FEAT_GUI_GTK
# include "gui_gtk_f.h"
#endif

/* GTK defines MAX and MIN, but some system header files as well.  Undefine
 * them and don't use them. */
#ifdef MIN
# undef MIN
#endif
#ifdef MAX
# undef MAX
#endif

#include "vim.h"

#ifdef FEAT_GUI_GNOME
/* Gnome redefines _() and N_().  Grrr... */
# ifdef _
#  undef _
# endif
# ifdef N_
#  undef N_
# endif
# ifdef textdomain
#  undef textdomain
# endif
# ifdef bindtextdomain
#  undef bindtextdomain
# endif
# if defined(FEAT_GETTEXT) && !defined(ENABLE_NLS)
#  define ENABLE_NLS	/* so the texts in the dialog boxes are translated */
# endif
# include <gnome.h>
#endif

#if defined(FEAT_GUI_DIALOG) && !defined(HAVE_GTK2)
# include "../pixmaps/alert.xpm"
# include "../pixmaps/error.xpm"
# include "../pixmaps/generic.xpm"
# include "../pixmaps/info.xpm"
# include "../pixmaps/quest.xpm"
#endif

#if defined(FEAT_TOOLBAR) && !defined(HAVE_GTK2)
/*
 * Icons used by the toolbar code.
 */
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
#endif /* FEAT_TOOLBAR && !HAVE_GTK2 */

#ifdef FEAT_GUI_GTK
# include <gdk/gdkkeysyms.h>
# include <gdk/gdk.h>
# ifdef WIN3264
#  include <gdk/gdkwin32.h>
# else
#  include <gdk/gdkx.h>
# endif

# include <gtk/gtk.h>
#else
/* define these items to be able to generate prototypes without GTK */
typedef int GtkWidget;
# define gpointer int
# define guint8 int
# define GdkPixmap int
# define GdkBitmap int
# define GtkIconFactory int
# define GtkToolbar int
# define GtkAdjustment int
# define gboolean int
# define GdkEventKey int
# define CancelData int
#endif

static void entry_activate_cb(GtkWidget *widget, gpointer data);
static void entry_changed_cb(GtkWidget *entry, GtkWidget *dialog);
static void find_replace_cb(GtkWidget *widget, gpointer data);
#ifndef HAVE_GTK2
static void gui_gtk_position_in_parent(GtkWidget *parent, GtkWidget *child, gui_win_pos_T where);
#endif

#if defined(FEAT_TOOLBAR) && defined(HAVE_GTK2)
/*
 * Table from BuiltIn## icon indices to GTK+ stock IDs.  Order must exactly
 * match toolbar_names[] in menu.c!  All stock icons including the "vim-*"
 * ones can be overridden in your gtkrc file.
 */
static const char * const menu_stock_ids[] =
{
    /* 00 */ GTK_STOCK_NEW,
    /* 01 */ GTK_STOCK_OPEN,
    /* 02 */ GTK_STOCK_SAVE,
    /* 03 */ GTK_STOCK_UNDO,
    /* 04 */ GTK_STOCK_REDO,
    /* 05 */ GTK_STOCK_CUT,
    /* 06 */ GTK_STOCK_COPY,
    /* 07 */ GTK_STOCK_PASTE,
    /* 08 */ GTK_STOCK_PRINT,
    /* 09 */ GTK_STOCK_HELP,
    /* 10 */ GTK_STOCK_FIND,
    /* 11 */ "vim-save-all",
    /* 12 */ "vim-session-save",
    /* 13 */ "vim-session-new",
    /* 14 */ "vim-session-load",
    /* 15 */ GTK_STOCK_EXECUTE,
    /* 16 */ GTK_STOCK_FIND_AND_REPLACE,
    /* 17 */ GTK_STOCK_CLOSE,		/* FIXME: fuzzy */
    /* 18 */ "vim-window-maximize",
    /* 19 */ "vim-window-minimize",
    /* 20 */ "vim-window-split",
    /* 21 */ "vim-shell",
    /* 22 */ GTK_STOCK_GO_BACK,
    /* 23 */ GTK_STOCK_GO_FORWARD,
    /* 24 */ "vim-find-help",
    /* 25 */ GTK_STOCK_CONVERT,
    /* 26 */ GTK_STOCK_JUMP_TO,
    /* 27 */ "vim-build-tags",
    /* 28 */ "vim-window-split-vertical",
    /* 29 */ "vim-window-maximize-width",
    /* 30 */ "vim-window-minimize-width",
    /* 31 */ GTK_STOCK_QUIT
};

    static void
add_stock_icon(GtkIconFactory	*factory,
	       const char	*stock_id,
	       const guint8	*inline_data,
	       int		data_length)
{
    GdkPixbuf	*pixbuf;
    GtkIconSet	*icon_set;

    pixbuf = gdk_pixbuf_new_from_inline(data_length, inline_data, FALSE, NULL);
    icon_set = gtk_icon_set_new_from_pixbuf(pixbuf);

    gtk_icon_factory_add(factory, stock_id, icon_set);

    gtk_icon_set_unref(icon_set);
    g_object_unref(pixbuf);
}

    static int
lookup_menu_iconfile(char_u *iconfile, char_u *dest)
{
    expand_env(iconfile, dest, MAXPATHL);

    if (mch_isFullName(dest))
    {
	return vim_fexists(dest);
    }
    else
    {
	static const char   suffixes[][4] = {"png", "xpm", "bmp"};
	char_u		    buf[MAXPATHL];
	unsigned int	    i;

	for (i = 0; i < G_N_ELEMENTS(suffixes); ++i)
	    if (gui_find_bitmap(dest, buf, (char *)suffixes[i]) == OK)
	    {
		STRCPY(dest, buf);
		return TRUE;
	    }

	return FALSE;
    }
}

    static GtkWidget *
load_menu_iconfile(char_u *name, GtkIconSize icon_size)
{
    GtkWidget	    *image = NULL;
    GtkIconSet	    *icon_set;
    GtkIconSource   *icon_source;

    /*
     * Rather than loading the icon directly into a GtkImage, create
     * a new GtkIconSet and put it in there.  This way we can easily
     * scale the toolbar icons on the fly when needed.
     */
    icon_set = gtk_icon_set_new();
    icon_source = gtk_icon_source_new();

    gtk_icon_source_set_filename(icon_source, (const char *)name);
    gtk_icon_set_add_source(icon_set, icon_source);

    image = gtk_image_new_from_icon_set(icon_set, icon_size);

    gtk_icon_source_free(icon_source);
    gtk_icon_set_unref(icon_set);

    return image;
}

    static GtkWidget *
create_menu_icon(vimmenu_T *menu, GtkIconSize icon_size)
{
    GtkWidget	*image = NULL;
    char_u	buf[MAXPATHL];

    /* First use a specified "icon=" argument. */
    if (menu->iconfile != NULL && lookup_menu_iconfile(menu->iconfile, buf))
	image = load_menu_iconfile(buf, icon_size);

    /* If not found and not builtin specified try using the menu name. */
    if (image == NULL && !menu->icon_builtin
				     && lookup_menu_iconfile(menu->name, buf))
	image = load_menu_iconfile(buf, icon_size);

    /* Still not found?  Then use a builtin icon, a blank one as fallback. */
    if (image == NULL)
    {
	const char  *stock_id;
	const int   n_ids = G_N_ELEMENTS(menu_stock_ids);

	if (menu->iconidx >= 0 && menu->iconidx < n_ids)
	    stock_id = menu_stock_ids[menu->iconidx];
	else
	    stock_id = GTK_STOCK_MISSING_IMAGE;

	image = gtk_image_new_from_stock(stock_id, icon_size);
    }

    return image;
}

/*ARGSUSED*/
    static gint
toolbar_button_focus_in_event(GtkWidget *widget, GdkEventFocus *event, gpointer data)
{
    /* When we're in a GtkPlug, we don't have window focus events, only widget focus.
     * To emulate stand-alone gvim, if a button gets focus (e.g., <Tab> into GtkPlug)
     * immediately pass it to mainwin.
     */
    if (gtk_socket_id != 0)
	gtk_widget_grab_focus(gui.drawarea);

    return TRUE;
}
#endif /* FEAT_TOOLBAR && HAVE_GTK2 */

#if (defined(FEAT_TOOLBAR) && defined(HAVE_GTK2)) || defined(PROTO)

    void
gui_gtk_register_stock_icons(void)
{
#   include "../pixmaps/stock_icons.h"
    GtkIconFactory *factory;

    factory = gtk_icon_factory_new();
#   define ADD_ICON(Name, Data) add_stock_icon(factory, Name, Data, (int)sizeof(Data))

    ADD_ICON("vim-build-tags",		  stock_vim_build_tags);
    ADD_ICON("vim-find-help",		  stock_vim_find_help);
    ADD_ICON("vim-save-all",		  stock_vim_save_all);
    ADD_ICON("vim-session-load",	  stock_vim_session_load);
    ADD_ICON("vim-session-new",		  stock_vim_session_new);
    ADD_ICON("vim-session-save",	  stock_vim_session_save);
    ADD_ICON("vim-shell",		  stock_vim_shell);
    ADD_ICON("vim-window-maximize",	  stock_vim_window_maximize);
    ADD_ICON("vim-window-maximize-width", stock_vim_window_maximize_width);
    ADD_ICON("vim-window-minimize",	  stock_vim_window_minimize);
    ADD_ICON("vim-window-minimize-width", stock_vim_window_minimize_width);
    ADD_ICON("vim-window-split",	  stock_vim_window_split);
    ADD_ICON("vim-window-split-vertical", stock_vim_window_split_vertical);

#   undef ADD_ICON
    gtk_icon_factory_add_default(factory);
    g_object_unref(factory);
}

#endif /* FEAT_TOOLBAR && HAVE_GTK2 */


/*
 * Only use accelerators when gtk_menu_ensure_uline_accel_group() is
 * available, which is in version 1.2.1.  That was the first version where
 * accelerators properly worked (according to the change log).
 */
#ifdef GTK_CHECK_VERSION
# if GTK_CHECK_VERSION(1, 2, 1)
#  define GTK_USE_ACCEL
# endif
#endif

#if defined(FEAT_MENU) || defined(PROTO)

/*
 * Translate Vim's mnemonic tagging to GTK+ style and convert to UTF-8
 * if necessary.  The caller must vim_free() the returned string.
 *
 *	Input	Output
 *	_	__
 *	&&	&
 *	&	_	stripped if use_mnemonic == FALSE
 *	<Tab>		end of menu label text
 */
    static char_u *
translate_mnemonic_tag(char_u *name, int use_mnemonic)
{
    char_u  *buf;
    char_u  *psrc;
    char_u  *pdest;
    int	    n_underscores = 0;

# ifdef HAVE_GTK2
    name = CONVERT_TO_UTF8(name);
# endif
    if (name == NULL)
	return NULL;

    for (psrc = name; *psrc != NUL && *psrc != TAB; ++psrc)
	if (*psrc == '_')
	    ++n_underscores;

    buf = alloc((unsigned)(psrc - name + n_underscores + 1));
    if (buf != NULL)
    {
	pdest = buf;
	for (psrc = name; *psrc != NUL && *psrc != TAB; ++psrc)
	{
	    if (*psrc == '_')
	    {
		*pdest++ = '_';
		*pdest++ = '_';
	    }
	    else if (*psrc != '&')
	    {
		*pdest++ = *psrc;
	    }
	    else if (*(psrc + 1) == '&')
	    {
		*pdest++ = *psrc++;
	    }
	    else if (use_mnemonic)
	    {
		*pdest++ = '_';
	    }
	}
	*pdest = NUL;
    }

# ifdef HAVE_GTK2
    CONVERT_TO_UTF8_FREE(name);
# endif
    return buf;
}

# ifdef HAVE_GTK2

    static void
menu_item_new(vimmenu_T *menu, GtkWidget *parent_widget)
{
    GtkWidget	*box;
    char_u	*text;
    int		use_mnemonic;

    /* It would be neat to have image menu items, but that would require major
     * changes to Vim's menu system.  Not to mention that all the translations
     * had to be updated. */
    menu->id = gtk_menu_item_new();
    box = gtk_hbox_new(FALSE, 20);

    use_mnemonic = (p_wak[0] != 'n' || !GTK_IS_MENU_BAR(parent_widget));
    text = translate_mnemonic_tag(menu->name, use_mnemonic);

    menu->label = gtk_label_new_with_mnemonic((const char *)text);
    vim_free(text);

    gtk_box_pack_start(GTK_BOX(box), menu->label, FALSE, FALSE, 0);

    if (menu->actext != NULL && menu->actext[0] != NUL)
    {
	text = CONVERT_TO_UTF8(menu->actext);

	gtk_box_pack_end(GTK_BOX(box),
			 gtk_label_new((const char *)text),
			 FALSE, FALSE, 0);

	CONVERT_TO_UTF8_FREE(text);
    }

    gtk_container_add(GTK_CONTAINER(menu->id), box);
    gtk_widget_show_all(menu->id);
}

# else /* !HAVE_GTK2 */

/*
 * Create a highly customized menu item by hand instead of by using:
 *
 * gtk_menu_item_new_with_label(menu->dname);
 *
 * This is neccessary, since there is no other way in GTK+ 1 to get the
 * not automatically parsed accellerator stuff right.
 */
    static void
menu_item_new(vimmenu_T *menu, GtkWidget *parent_widget)
{
    GtkWidget	*widget;
    GtkWidget	*bin;
    GtkWidget	*label;
    char_u	*name;
    guint	accel_key;

    widget = gtk_widget_new(GTK_TYPE_MENU_ITEM,
			    "GtkWidget::visible", TRUE,
			    "GtkWidget::sensitive", TRUE,
			    /* "GtkWidget::parent", parent->submenu_id, */
			    NULL);
    bin = gtk_widget_new(GTK_TYPE_HBOX,
			 "GtkWidget::visible", TRUE,
			 "GtkWidget::parent", widget,
			 "GtkBox::spacing", 16,
			 NULL);
    label = gtk_widget_new(GTK_TYPE_ACCEL_LABEL,
			   "GtkWidget::visible", TRUE,
			   "GtkWidget::parent", bin,
			   "GtkAccelLabel::accel_widget", widget,
			   "GtkMisc::xalign", 0.0,
			   NULL);
    menu->label = label;

    if (menu->actext)
	gtk_widget_new(GTK_TYPE_LABEL,
		       "GtkWidget::visible", TRUE,
		       "GtkWidget::parent", bin,
		       "GtkLabel::label", menu->actext,
		       "GtkMisc::xalign", 1.0,
			NULL);

    /*
     * Translate VIM accelerator tagging into GTK+'s.  Note that since GTK uses
     * underscores as the accelerator key, we need to add an additional under-
     * score for each understore that appears in the menu name.
     */
#  ifdef GTK_USE_ACCEL
    name = translate_mnemonic_tag(menu->name,
		(p_wak[0] != 'n' || !GTK_IS_MENU_BAR(parent_widget)));
#  else
    name = translate_mnemonic_tag(menu->name, FALSE);
#  endif

    /* let GTK do its thing */
    accel_key = gtk_label_parse_uline(GTK_LABEL(label), (const char *)name);
    vim_free(name);

#  ifdef GTK_USE_ACCEL
    /* Don't add accelator if 'winaltkeys' is "no". */
    if (accel_key != GDK_VoidSymbol)
    {
	if (GTK_IS_MENU_BAR(parent_widget))
	{
	    if (*p_wak != 'n')
		gtk_widget_add_accelerator(widget,
			"activate_item",
			gui.accel_group,
			accel_key, GDK_MOD1_MASK,
			(GtkAccelFlags)0);
	}
	else
	{
	    gtk_widget_add_accelerator(widget,
		    "activate_item",
		    gtk_menu_ensure_uline_accel_group(GTK_MENU(parent_widget)),
		    accel_key, 0,
		    (GtkAccelFlags)0);
	}
    }
#  endif /* GTK_USE_ACCEL */

    menu->id = widget;
}

# endif /* !HAVE_GTK2 */

    void
gui_mch_add_menu(vimmenu_T *menu, int idx)
{
    vimmenu_T	*parent;
    GtkWidget	*parent_widget;

    if (menu->name[0] == ']' || menu_is_popup(menu->name))
    {
	menu->submenu_id = gtk_menu_new();
	return;
    }

    parent = menu->parent;

    if ((parent != NULL && parent->submenu_id == NULL)
	    || !menu_is_menubar(menu->name))
	return;

    parent_widget = (parent != NULL) ? parent->submenu_id : gui.menubar;
    menu_item_new(menu, parent_widget);

    /* since the tearoff should always appear first, increment idx */
    if (parent != NULL && !menu_is_popup(parent->name))
	++idx;

    gtk_menu_shell_insert(GTK_MENU_SHELL(parent_widget), menu->id, idx);

#ifndef HAVE_GTK2
    /*
     * The "Help" menu is a special case, and should be placed at the far
     * right hand side of the menu-bar.  It's detected by its high priority.
     *
     * Right-aligning "Help" is considered bad UI design nowadays.
     * Thus lets disable this for GTK+ 2 to match the environment.
     */
    if (parent == NULL && menu->priority >= 9999)
	gtk_menu_item_right_justify(GTK_MENU_ITEM(menu->id));
#endif

    menu->submenu_id = gtk_menu_new();

    gtk_menu_set_accel_group(GTK_MENU(menu->submenu_id), gui.accel_group);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu->id), menu->submenu_id);

    menu->tearoff_handle = gtk_tearoff_menu_item_new();
    if (vim_strchr(p_go, GO_TEAROFF) != NULL)
	gtk_widget_show(menu->tearoff_handle);
    gtk_menu_prepend(GTK_MENU(menu->submenu_id), menu->tearoff_handle);
}

/*ARGSUSED*/
    static void
menu_item_activate(GtkWidget *widget, gpointer data)
{
    gui_menu_cb((vimmenu_T *)data);

# ifndef HAVE_GTK2
    /* Work around a bug in GTK+ 1: we don't seem to get a focus-in
     * event after clicking a menu item shown via :popup. */
    if (!gui.in_focus)
	gui_focus_change(TRUE);
# endif

    /* make sure the menu action is taken immediately */
    if (gtk_main_level() > 0)
	gtk_main_quit();
}

# if defined(FEAT_TOOLBAR) && !defined(HAVE_GTK2)
/*
 * These are the pixmaps used for the default buttons.
 * Order must exactly match toolbar_names[] in menu.c!
 */
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
};

/*
 * creates a blank pixmap using tb_blank
 */
    static void
pixmap_create_from_xpm(char **xpm, GdkPixmap **pixmap, GdkBitmap **mask)
{
    *pixmap = gdk_pixmap_colormap_create_from_xpm_d(
	    NULL,
	    gtk_widget_get_colormap(gui.mainwin),
	    mask,
	    NULL,
	    xpm);
}

/*
 * creates a pixmap by using a built-in number
 */
    static void
pixmap_create_by_num(int pixmap_num, GdkPixmap **pixmap, GdkBitmap **mask)
{
    if (pixmap_num >= 0 && pixmap_num < (sizeof(built_in_pixmaps)
					    / sizeof(built_in_pixmaps[0])))
	pixmap_create_from_xpm(built_in_pixmaps[pixmap_num], pixmap, mask);
}

/*
 * Creates a pixmap by using the pixmap "name" found in 'runtimepath'/bitmaps/
 */
    static void
pixmap_create_by_dir(char_u *name, GdkPixmap **pixmap, GdkBitmap **mask)
{
    char_u full_pathname[MAXPATHL + 1];

    if (gui_find_bitmap(name, full_pathname, "xpm") == OK)
	*pixmap = gdk_pixmap_colormap_create_from_xpm(
		NULL,
		gtk_widget_get_colormap(gui.mainwin),
		mask,
		&gui.mainwin->style->bg[GTK_STATE_NORMAL],
		(const char *)full_pathname);
}

/*
 * Creates a pixmap by using the pixmap "fname".
 */
    static void
pixmap_create_from_file(char_u *fname, GdkPixmap **pixmap, GdkBitmap **mask)
{
    *pixmap = gdk_pixmap_colormap_create_from_xpm(
		NULL,
		gtk_widget_get_colormap(gui.mainwin),
		mask,
		&gui.mainwin->style->bg[GTK_STATE_NORMAL],
		(const char *)fname);
}

# endif /* FEAT_TOOLBAR && !HAVE_GTK2 */

    void
gui_mch_add_menu_item(vimmenu_T *menu, int idx)
{
    vimmenu_T *parent;

    parent = menu->parent;

# ifdef FEAT_TOOLBAR
    if (menu_is_toolbar(parent->name))
    {
	GtkToolbar *toolbar;

	toolbar = GTK_TOOLBAR(gui.toolbar);
	menu->submenu_id = NULL;

	if (menu_is_separator(menu->name))
	{
	    gtk_toolbar_insert_space(toolbar, idx);
	    menu->id = NULL;
	}
	else
	{
#  ifdef HAVE_GTK2
	    char_u *text;
	    char_u *tooltip;

	    text    = CONVERT_TO_UTF8(menu->dname);
	    tooltip = CONVERT_TO_UTF8(menu->strings[MENU_INDEX_TIP]);
	    if (tooltip != NULL && !utf_valid_string(tooltip, NULL))
		/* Invalid text, can happen when 'encoding' is changed.  Avoid
		 * a nasty GTK error message, skip the tooltip. */
		CONVERT_TO_UTF8_FREE(tooltip);

	    menu->id = gtk_toolbar_insert_item(
		    toolbar,
		    (const char *)text,
		    (const char *)tooltip,
		    NULL,
		    create_menu_icon(menu, gtk_toolbar_get_icon_size(toolbar)),
		    G_CALLBACK(&menu_item_activate),
		    menu,
		    idx);

	    if (gtk_socket_id != 0)
		gtk_signal_connect(GTK_OBJECT(menu->id), "focus_in_event",
			GTK_SIGNAL_FUNC(toolbar_button_focus_in_event), NULL);

	    CONVERT_TO_UTF8_FREE(text);
	    CONVERT_TO_UTF8_FREE(tooltip);

#  else /* !HAVE_GTK2 */

	    GdkPixmap *pixmap = NULL;
	    GdkBitmap *mask = NULL;

	    /* First try user specified bitmap, then builtin, the a blank. */
	    if (menu->iconfile != NULL)
	    {
		char_u buf[MAXPATHL + 1];

		gui_find_iconfile(menu->iconfile, buf, "xpm");
		pixmap_create_from_file(buf, &pixmap, &mask);
	    }
	    if (pixmap == NULL && !menu->icon_builtin)
		pixmap_create_by_dir(menu->name, &pixmap, &mask);
	    if (pixmap == NULL && menu->iconidx >= 0)
		pixmap_create_by_num(menu->iconidx, &pixmap, &mask);
	    if (pixmap == NULL)
		pixmap_create_from_xpm(tb_blank_xpm, &pixmap, &mask);
	    if (pixmap == NULL)
		return; /* should at least have blank pixmap, but if not... */

	    menu->id = gtk_toolbar_insert_item(
				    toolbar,
				    (char *)(menu->dname),
				    (char *)(menu->strings[MENU_INDEX_TIP]),
				    (char *)(menu->dname),
				    gtk_pixmap_new(pixmap, mask),
				    GTK_SIGNAL_FUNC(menu_item_activate),
				    (gpointer)menu,
				    idx);
#  endif /* !HAVE_GTK2 */
	}
    }
    else
# endif /* FEAT_TOOLBAR */
    {
	/* No parent, must be a non-menubar menu */
	if (parent == NULL || parent->submenu_id == NULL)
	    return;

	/* Make place for the possible tearoff handle item.  Not in the popup
	 * menu, it doesn't have a tearoff item. */
	if (!menu_is_popup(parent->name))
	    ++idx;

	if (menu_is_separator(menu->name))
	{
	    /* Separator: Just add it */
	    menu->id = gtk_menu_item_new();
	    gtk_widget_set_sensitive(menu->id, FALSE);
	    gtk_widget_show(menu->id);
	    gtk_menu_insert(GTK_MENU(parent->submenu_id), menu->id, idx);

	    return;
	}

	/* Add textual menu item. */
	menu_item_new(menu, parent->submenu_id);
	gtk_widget_show(menu->id);
	gtk_menu_insert(GTK_MENU(parent->submenu_id), menu->id, idx);

	if (menu->id != NULL)
	    gtk_signal_connect(GTK_OBJECT(menu->id), "activate",
			       GTK_SIGNAL_FUNC(menu_item_activate), menu);
    }
}
#endif /* FEAT_MENU */


    void
gui_mch_set_text_area_pos(int x, int y, int w, int h)
{
    gtk_form_move_resize(GTK_FORM(gui.formwin), gui.drawarea, x, y, w, h);
}


#if defined(FEAT_MENU) || defined(PROTO)
/*
 * Enable or disable accelators for the toplevel menus.
 */
    void
gui_gtk_set_mnemonics(int enable)
{
    vimmenu_T	*menu;
    char_u	*name;
# if !defined(HAVE_GTK2) && defined(GTK_USE_ACCEL)
    guint	accel_key;
# endif

    for (menu = root_menu; menu != NULL; menu = menu->next)
    {
	if (menu->id == NULL)
	    continue;

# if defined(HAVE_GTK2)
	name = translate_mnemonic_tag(menu->name, enable);
	gtk_label_set_text_with_mnemonic(GTK_LABEL(menu->label),
					 (const char *)name);
	vim_free(name);
# else
#  if defined(GTK_USE_ACCEL)
	name = translate_mnemonic_tag(menu->name, TRUE);
	if (name != NULL)
	{
	    accel_key = gtk_label_parse_uline(GTK_LABEL(menu->label),
					      (const char *)name);
	    if (accel_key != GDK_VoidSymbol)
		gtk_widget_remove_accelerator(menu->id, gui.accel_group,
					      accel_key, GDK_MOD1_MASK);
	    if (enable && accel_key != GDK_VoidSymbol)
		gtk_widget_add_accelerator(menu->id, "activate_item",
					   gui.accel_group,
					   accel_key, GDK_MOD1_MASK,
					   (GtkAccelFlags)0);
	    vim_free(name);
	}
	if (!enable)
	{
	    name = translate_mnemonic_tag(menu->name, FALSE);
	    gtk_label_parse_uline(GTK_LABEL(menu->label), (const char *)name);
	    vim_free(name);
	}
#  endif
# endif
    }
}

    static void
recurse_tearoffs(vimmenu_T *menu, int val)
{
    for (; menu != NULL; menu = menu->next)
    {
	if (menu->submenu_id != NULL && menu->tearoff_handle != NULL
		&& menu->name[0] != ']' && !menu_is_popup(menu->name))
	{
	    if (val)
		gtk_widget_show(menu->tearoff_handle);
	    else
		gtk_widget_hide(menu->tearoff_handle);
	}
	recurse_tearoffs(menu->children, val);
    }
}

    void
gui_mch_toggle_tearoffs(int enable)
{
    recurse_tearoffs(root_menu, enable);
}
#endif /* FEAT_MENU */


#if defined(FEAT_TOOLBAR) && !defined(HAVE_GTK2)
/*
 * Seems like there's a hole in the GTK Toolbar API: there's no provision for
 * removing an item from the toolbar.  Therefore I need to resort to going
 * really deeply into the internal widget structures.
 *
 * <danielk> I'm not sure the statement above is true -- at least with
 * GTK+ 2 one can just call gtk_widget_destroy() and be done with it.
 * It is true though that you couldn't remove space items before GTK+ 2
 * (without digging into the internals that is).  But the code below
 * doesn't seem to handle those either.  Well, it's obsolete anyway.
 */
    static void
toolbar_remove_item_by_text(GtkToolbar *tb, const char *text)
{
    GtkContainer *container;
    GList *childl;
    GtkToolbarChild *gtbc;

    g_return_if_fail(tb != NULL);
    g_return_if_fail(GTK_IS_TOOLBAR(tb));
    container = GTK_CONTAINER(&tb->container);

    for (childl = tb->children; childl; childl = childl->next)
    {
	gtbc = (GtkToolbarChild *)childl->data;

	if (gtbc->type != GTK_TOOLBAR_CHILD_SPACE
		&& strcmp(GTK_LABEL(gtbc->label)->label, text) == 0)
	{
	    gboolean was_visible;

	    was_visible = GTK_WIDGET_VISIBLE(gtbc->widget);
	    gtk_widget_unparent(gtbc->widget);

	    tb->children = g_list_remove_link(tb->children, childl);
	    g_free(gtbc);
	    g_list_free(childl);
	    tb->num_children--;

	    if (was_visible && GTK_WIDGET_VISIBLE(container))
		gtk_widget_queue_resize(GTK_WIDGET(container));

	    break;
	}
    }
}
#endif /* FEAT_TOOLBAR && !HAVE_GTK2 */


#if defined(FEAT_TOOLBAR) && defined(HAVE_GTK2)
    static int
get_menu_position(vimmenu_T *menu)
{
    vimmenu_T	*node;
    int		index = 0;

    for (node = menu->parent->children; node != menu; node = node->next)
    {
	g_return_val_if_fail(node != NULL, -1);
	++index;
    }

    return index;
}
#endif /* FEAT_TOOLBAR && HAVE_GTK2 */


#if defined(FEAT_TOOLBAR) || defined(PROTO)
    void
gui_mch_menu_set_tip(vimmenu_T *menu)
{
    if (menu->id != NULL && menu->parent != NULL
	    && gui.toolbar != NULL && menu_is_toolbar(menu->parent->name))
    {
	char_u *tooltip;

# ifdef HAVE_GTK2
	tooltip = CONVERT_TO_UTF8(menu->strings[MENU_INDEX_TIP]);
	if (tooltip == NULL || utf_valid_string(tooltip, NULL))
	    /* Only set the tooltip when it's valid utf-8. */
# else
	tooltip = menu->strings[MENU_INDEX_TIP];
# endif
	gtk_tooltips_set_tip(GTK_TOOLBAR(gui.toolbar)->tooltips,
			     menu->id, (const char *)tooltip, NULL);
# ifdef HAVE_GTK2
	CONVERT_TO_UTF8_FREE(tooltip);
# endif
    }
}
#endif /* FEAT_TOOLBAR */


#if defined(FEAT_MENU) || defined(PROTO)
/*
 * Destroy the machine specific menu widget.
 */
    void
gui_mch_destroy_menu(vimmenu_T *menu)
{
# ifdef FEAT_TOOLBAR
    if (menu->parent != NULL && menu_is_toolbar(menu->parent->name))
    {
#  ifdef HAVE_GTK2
	if (menu_is_separator(menu->name))
	    gtk_toolbar_remove_space(GTK_TOOLBAR(gui.toolbar),
				     get_menu_position(menu));
	else if (menu->id != NULL)
	    gtk_widget_destroy(menu->id);
#  else
	toolbar_remove_item_by_text(GTK_TOOLBAR(gui.toolbar),
				    (const char *)menu->dname);
#  endif
    }
    else
# endif /* FEAT_TOOLBAR */
    {
	if (menu->submenu_id != NULL)
	    gtk_widget_destroy(menu->submenu_id);

	if (menu->id != NULL)
	    gtk_widget_destroy(menu->id);
    }

    menu->submenu_id = NULL;
    menu->id = NULL;
}
#endif /* FEAT_MENU */


/*
 * Scrollbar stuff.
 */
    void
gui_mch_set_scrollbar_thumb(scrollbar_T *sb, long val, long size, long max)
{
    if (sb->id != NULL)
    {
	GtkAdjustment *adjustment;

	adjustment = gtk_range_get_adjustment(GTK_RANGE(sb->id));

	adjustment->lower = 0.0;
	adjustment->value = val;
	adjustment->upper = max + 1;
	adjustment->page_size = size;
	adjustment->page_increment = size < 3L ? 1L : size - 2L;
	adjustment->step_increment = 1.0;

#ifdef HAVE_GTK2
	g_signal_handler_block(GTK_OBJECT(adjustment),
						      (gulong)sb->handler_id);
#else
	gtk_signal_handler_block(GTK_OBJECT(adjustment),
						       (guint)sb->handler_id);
#endif
	gtk_adjustment_changed(adjustment);
#ifdef HAVE_GTK2
	g_signal_handler_unblock(GTK_OBJECT(adjustment),
						      (gulong)sb->handler_id);
#else
	gtk_signal_handler_unblock(GTK_OBJECT(adjustment),
						       (guint)sb->handler_id);
#endif
    }
}

    void
gui_mch_set_scrollbar_pos(scrollbar_T *sb, int x, int y, int w, int h)
{
    if (sb->id != NULL)
	gtk_form_move_resize(GTK_FORM(gui.formwin), sb->id, x, y, w, h);
}

/*
 * Take action upon scrollbar dragging.
 */
    static void
adjustment_value_changed(GtkAdjustment *adjustment, gpointer data)
{
    scrollbar_T	*sb;
    long	value;
    int		dragging = FALSE;

#ifdef FEAT_XIM
    /* cancel any preediting */
    if (im_is_preediting())
	xim_reset();
#endif

    sb = gui_find_scrollbar((long)data);
    value = (long)adjustment->value;
    /*
     * The dragging argument must be right for the scrollbar to work with
     * closed folds.  This isn't documented, hopefully this will keep on
     * working in later GTK versions.
     *
     * FIXME: Well, it doesn't work in GTK2. :)
     * HACK: Get the mouse pointer position, if it appears to be on an arrow
     * button set "dragging" to FALSE.  This assumes square buttons!
     */
    if (sb != NULL)
    {
#ifdef HAVE_GTK2
	dragging = TRUE;

	if (sb->wp != NULL)
	{
	    int			x;
	    int			y;
	    GdkModifierType	state;
	    int			width;
	    int			height;

	    /* vertical scrollbar: need to set "dragging" properly in case
	     * there are closed folds. */
	    gdk_window_get_pointer(sb->id->window, &x, &y, &state);
	    gdk_window_get_size(sb->id->window, &width, &height);
	    if (x >= 0 && x < width && y >= 0 && y < height)
	    {
		if (y < width)
		{
		    /* up arrow: move one (closed fold) line up */
		    dragging = FALSE;
		    value = sb->wp->w_topline - 2;
		}
		else if (y > height - width)
		{
		    /* down arrow: move one (closed fold) line down */
		    dragging = FALSE;
		    value = sb->wp->w_topline;
		}
	    }
	}
#else
	dragging = (GTK_RANGE(sb->id)->scroll_type == GTK_SCROLL_NONE);
#endif
    }

    gui_drag_scrollbar(sb, value, dragging);

    if (gtk_main_level() > 0)
	gtk_main_quit();
}

/* SBAR_VERT or SBAR_HORIZ */
    void
gui_mch_create_scrollbar(scrollbar_T *sb, int orient)
{
    if (orient == SBAR_HORIZ)
	sb->id = gtk_hscrollbar_new(NULL);
    else if (orient == SBAR_VERT)
	sb->id = gtk_vscrollbar_new(NULL);

    if (sb->id != NULL)
    {
	GtkAdjustment *adjustment;

	GTK_WIDGET_UNSET_FLAGS(sb->id, GTK_CAN_FOCUS);
	gtk_form_put(GTK_FORM(gui.formwin), sb->id, 0, 0);

	adjustment = gtk_range_get_adjustment(GTK_RANGE(sb->id));

	sb->handler_id = gtk_signal_connect(
			     GTK_OBJECT(adjustment), "value_changed",
			     GTK_SIGNAL_FUNC(adjustment_value_changed),
			     GINT_TO_POINTER(sb->ident));
	gui_mch_update();
    }
}

#if defined(FEAT_WINDOWS) || defined(PROTO)
    void
gui_mch_destroy_scrollbar(scrollbar_T *sb)
{
    if (sb->id != NULL)
    {
	gtk_widget_destroy(sb->id);
	sb->id = NULL;
    }
    gui_mch_update();
}
#endif

#if defined(FEAT_BROWSE) || defined(PROTO)
/*
 * Implementation of the file selector related stuff
 */
#if defined(HAVE_GTK2) && GTK_CHECK_VERSION(2,4,0)
# define USE_FILE_CHOOSER
#endif

#ifndef USE_FILE_CHOOSER
/*ARGSUSED*/
    static void
browse_ok_cb(GtkWidget *widget, gpointer cbdata)
{
    gui_T *vw = (gui_T *)cbdata;

    if (vw->browse_fname != NULL)
	g_free(vw->browse_fname);

    vw->browse_fname = (char_u *)g_strdup(gtk_file_selection_get_filename(
					GTK_FILE_SELECTION(vw->filedlg)));
    gtk_widget_hide(vw->filedlg);
    if (gtk_main_level() > 0)
	gtk_main_quit();
}

/*ARGSUSED*/
    static void
browse_cancel_cb(GtkWidget *widget, gpointer cbdata)
{
    gui_T *vw = (gui_T *)cbdata;

    if (vw->browse_fname != NULL)
    {
	g_free(vw->browse_fname);
	vw->browse_fname = NULL;
    }
    gtk_widget_hide(vw->filedlg);
    if (gtk_main_level() > 0)
	gtk_main_quit();
}

/*ARGSUSED*/
    static gboolean
browse_destroy_cb(GtkWidget * widget)
{
    if (gui.browse_fname != NULL)
    {
	g_free(gui.browse_fname);
	gui.browse_fname = NULL;
    }
    gui.filedlg = NULL;

    if (gtk_main_level() > 0)
	gtk_main_quit();

    return FALSE;
}
#endif

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
gui_mch_browse(int saving,
	       char_u *title,
	       char_u *dflt,
	       char_u *ext,
	       char_u *initdir,
	       char_u *filter)
{
#ifdef USE_FILE_CHOOSER
    GtkWidget		*fc;
#endif
    char_u		dirbuf[MAXPATHL];
    char_u		*p;

# ifdef HAVE_GTK2
    title = CONVERT_TO_UTF8(title);
# endif

    /* Concatenate "initdir" and "dflt". */
    if (initdir == NULL || *initdir == NUL)
	mch_dirname(dirbuf, MAXPATHL);
    else if (STRLEN(initdir) + 2 < MAXPATHL)
	STRCPY(dirbuf, initdir);
    else
	dirbuf[0] = NUL;
    /* Always need a trailing slash for a directory. */
    add_pathsep(dirbuf);
    if (dflt != NULL && *dflt != NUL
			      && STRLEN(dirbuf) + 2 + STRLEN(dflt) < MAXPATHL)
	STRCAT(dirbuf, dflt);

    /* If our pointer is currently hidden, then we should show it. */
    gui_mch_mousehide(FALSE);

#ifdef USE_FILE_CHOOSER
    /* We create the dialog each time, so that the button text can be "Open"
     * or "Save" according to the action. */
    fc = gtk_file_chooser_dialog_new((const gchar *)title,
	    GTK_WINDOW(gui.mainwin),
	    saving ? GTK_FILE_CHOOSER_ACTION_SAVE
					   : GTK_FILE_CHOOSER_ACTION_OPEN,
	    saving ? GTK_STOCK_SAVE : GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
	    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
	    NULL);
    gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(fc),
						       (const gchar *)dirbuf);

    gui.browse_fname = NULL;
    if (gtk_dialog_run(GTK_DIALOG(fc)) == GTK_RESPONSE_ACCEPT)
    {
	char *filename;

	filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(fc));
	gui.browse_fname = (char_u *)g_strdup(filename);
	g_free(filename);
    }
    gtk_widget_destroy(GTK_WIDGET(fc));

#else

    if (gui.filedlg == NULL)
    {
	GtkFileSelection	*fs;	/* shortcut */

	gui.filedlg = gtk_file_selection_new((const gchar *)title);
	gtk_window_set_modal(GTK_WINDOW(gui.filedlg), TRUE);
	gtk_window_set_transient_for(GTK_WINDOW(gui.filedlg),
						     GTK_WINDOW(gui.mainwin));
	fs = GTK_FILE_SELECTION(gui.filedlg);

	gtk_container_border_width(GTK_CONTAINER(fs), 4);

	gtk_signal_connect(GTK_OBJECT(fs->ok_button),
		"clicked", GTK_SIGNAL_FUNC(browse_ok_cb), &gui);
	gtk_signal_connect(GTK_OBJECT(fs->cancel_button),
		"clicked", GTK_SIGNAL_FUNC(browse_cancel_cb), &gui);
	/* gtk_signal_connect() doesn't work for destroy, it causes a hang */
	gtk_signal_connect_object(GTK_OBJECT(gui.filedlg),
		"destroy", GTK_SIGNAL_FUNC(browse_destroy_cb),
		GTK_OBJECT(gui.filedlg));
    }
    else
	gtk_window_set_title(GTK_WINDOW(gui.filedlg), (const gchar *)title);

    gtk_file_selection_set_filename(GTK_FILE_SELECTION(gui.filedlg),
						      (const gchar *)dirbuf);
# ifndef HAVE_GTK2
    gui_gtk_position_in_parent(GTK_WIDGET(gui.mainwin),
				       GTK_WIDGET(gui.filedlg), VW_POS_MOUSE);
# endif

    gtk_widget_show(gui.filedlg);
    while (gui.filedlg && GTK_WIDGET_DRAWABLE(gui.filedlg))
	gtk_main_iteration_do(TRUE);
#endif

# ifdef HAVE_GTK2
    CONVERT_TO_UTF8_FREE(title);
# endif
    if (gui.browse_fname == NULL)
	return NULL;

    /* shorten the file name if possible */
    mch_dirname(dirbuf, MAXPATHL);
    p = shorten_fname(gui.browse_fname, dirbuf);
    if (p == NULL)
	p = gui.browse_fname;
    return vim_strsave(p);
}

#if defined(HAVE_GTK2) || defined(PROTO)

/*
 * Put up a directory selector
 * Returns the selected name in allocated memory, or NULL for Cancel.
 * title			title for the window
 * dflt				default name
 * initdir			initial directory, NULL for current dir
 */
/*ARGSUSED*/
    char_u *
gui_mch_browsedir(
	       char_u *title,
	       char_u *initdir)
{
# if defined(GTK_FILE_CHOOSER)	    /* Only in GTK 2.4 and later. */
    char_u		dirbuf[MAXPATHL];
    char_u		*p;
    GtkWidget		*dirdlg;	    /* file selection dialog */
    char_u		*dirname = NULL;

    title = CONVERT_TO_UTF8(title);

    dirdlg = gtk_file_chooser_dialog_new(
	    (const gchar *)title,
	    GTK_WINDOW(gui.mainwin),
	    GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
	    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
	    GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
	    NULL);

    CONVERT_TO_UTF8_FREE(title);

    /* if our pointer is currently hidden, then we should show it. */
    gui_mch_mousehide(FALSE);

    /* GTK appears to insist on an absolute path. */
    if (initdir == NULL || *initdir == NUL
	       || vim_FullName(initdir, dirbuf, MAXPATHL - 10, FALSE) == FAIL)
	mch_dirname(dirbuf, MAXPATHL - 10);

    /* Always need a trailing slash for a directory.
     * Also add a dummy file name, so that we get to the directory. */
    add_pathsep(dirbuf);
    STRCAT(dirbuf, "@zd(*&1|");
    gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dirdlg),
						      (const gchar *)dirbuf);

    /* Run the dialog. */
    if (gtk_dialog_run(GTK_DIALOG(dirdlg)) == GTK_RESPONSE_ACCEPT)
	dirname = (char_u *)gtk_file_chooser_get_filename(
						    GTK_FILE_CHOOSER(dirdlg));
    gtk_widget_destroy(dirdlg);
    if (dirname == NULL)
	return NULL;

    /* shorten the file name if possible */
    mch_dirname(dirbuf, MAXPATHL);
    p = shorten_fname(dirname, dirbuf);
    if (p == NULL || *p == NUL)
	p = dirname;
    p = vim_strsave(p);
    g_free(dirname);
    return p;

# else
    /* For GTK 2.2 and earlier: fall back to ordinary file selector. */
    return gui_mch_browse(0, title, NULL, NULL, initdir, NULL);
# endif
}
#endif


#endif	/* FEAT_BROWSE */

#if defined(FEAT_GUI_DIALOG) && !defined(HAVE_GTK2)

static char_u *dialog_textfield = NULL;
static GtkWidget *dialog_textentry;

    static void
dlg_destroy(GtkWidget *dlg)
{
    if (dialog_textfield != NULL)
    {
	const char *text;

	text = gtk_entry_get_text(GTK_ENTRY(dialog_textentry));
	vim_strncpy(dialog_textfield, (char_u *)text, IOSIZE - 1);
    }

    /* Destroy the dialog, will break the waiting loop. */
    gtk_widget_destroy(dlg);
}

# ifdef FEAT_GUI_GNOME
/* ARGSUSED */
    static int
gui_gnome_dialog( int	type,
		char_u	*title,
		char_u	*message,
		char_u	*buttons,
		int	dfltbutton,
		char_u	*textfield)
{
    GtkWidget	*dlg;
    char	*gdtype;
    char_u	*buttons_copy, *p, *next;
    char	**buttons_list;
    int		butcount, cur;

    /* make a copy, so that we can insert NULs */
    if ((buttons_copy = vim_strsave(buttons)) == NULL)
	return -1;

    /* determine exact number of buttons and allocate array to hold them */
    for (butcount = 0, p = buttons; *p; p++)
    {
	if (*p == '\n')
	    butcount++;
    }
    butcount++;
    buttons_list = g_new0(char *, butcount + 1);

    /* Add pixmap */
    switch (type)
    {
    case VIM_ERROR:
	gdtype = GNOME_MESSAGE_BOX_ERROR;
	break;
    case VIM_WARNING:
	gdtype = GNOME_MESSAGE_BOX_WARNING;
	break;
    case VIM_INFO:
	gdtype = GNOME_MESSAGE_BOX_INFO;
	break;
    case VIM_QUESTION:
	gdtype = GNOME_MESSAGE_BOX_QUESTION;
	break;
    default:
	gdtype = GNOME_MESSAGE_BOX_GENERIC;
    };

    p = buttons_copy;
    for (cur = 0; cur < butcount; ++cur)
    {
	for (next = p; *next; ++next)
	{
	    if (*next == DLG_HOTKEY_CHAR)
		mch_memmove(next, next + 1, STRLEN(next));
	    if (*next == DLG_BUTTON_SEP)
	    {
		*next++ = NUL;
		break;
	    }
	}

	/* this should probably go into a table, but oh well */
	if (g_strcasecmp((char *)p, "Ok") == 0)
	    buttons_list[cur] = g_strdup(GNOME_STOCK_BUTTON_OK);
	else if (g_strcasecmp((char *)p, "Cancel") == 0)
	    buttons_list[cur] = g_strdup(GNOME_STOCK_BUTTON_CANCEL);
	else if (g_strcasecmp((char *)p, "Yes") == 0)
	    buttons_list[cur] = g_strdup(GNOME_STOCK_BUTTON_YES);
	else if (g_strcasecmp((char *)p, "No") == 0)
	    buttons_list[cur] = g_strdup(GNOME_STOCK_BUTTON_NO);
	else if (g_strcasecmp((char *)p, "Close") == 0)
	    buttons_list[cur] = g_strdup(GNOME_STOCK_BUTTON_CLOSE);
	else if (g_strcasecmp((char *)p, "Help") == 0)
	    buttons_list[cur] = g_strdup(GNOME_STOCK_BUTTON_HELP);
	else if (g_strcasecmp((char *)p, "Apply") == 0)
	    buttons_list[cur] = g_strdup(GNOME_STOCK_BUTTON_APPLY);
#if 0
	/*
	 * these aren't really used that often anyway, but are listed here as
	 * placeholders in case we need them.
	 */
	else if (g_strcasecmp((char *)p, "Next") == 0)
	    buttons_list[cur] = g_strdup(GNOME_STOCK_BUTTON_NEXT);
	else if (g_strcasecmp((char *)p, "Prev") == 0)
	    buttons_list[cur] = g_strdup(GNOME_STOCK_BUTTON_PREV);
	else if (g_strcasecmp((char *)p, "Up") == 0)
	    buttons_list[cur] = g_strdup(GNOME_STOCK_BUTTON_UP);
	else if (g_strcasecmp((char *)p, "Down") == 0)
	    buttons_list[cur] = g_strdup(GNOME_STOCK_BUTTON_DOWN);
	else if (g_strcasecmp((char *)p, "Font") == 0)
	    buttons_list[cur] = g_strdup(GNOME_STOCK_BUTTON_FONT);
#endif
	else
	    buttons_list[cur] = g_strdup((char *)p);

	if (*next == NUL)
	    break;

	p = next;
    }
    vim_free(buttons_copy);

    dlg = gnome_message_box_newv((const char *)message,
				 (const char *)gdtype,
				 (const char **)buttons_list);
    for (cur = 0; cur < butcount; ++cur)
	g_free(buttons_list[cur]);
    g_free(buttons_list);

    dialog_textfield = textfield;
    if (textfield != NULL)
    {
	/* Add text entry field */
	dialog_textentry = gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(GNOME_DIALOG(dlg)->vbox), dialog_textentry,
			   TRUE, TRUE, 0);
	gtk_entry_set_text(GTK_ENTRY(dialog_textentry),
			   (const gchar *)textfield);
	gtk_entry_select_region(GTK_ENTRY(dialog_textentry), 0,
				STRLEN(textfield));
	gtk_entry_set_max_length(GTK_ENTRY(dialog_textentry), IOSIZE - 1);
	gtk_entry_set_position(GTK_ENTRY(dialog_textentry), STRLEN(textfield));
	gtk_widget_show(dialog_textentry);
	gtk_window_set_focus(GTK_WINDOW(dlg), dialog_textentry);
    }

    gtk_signal_connect_object(GTK_OBJECT(dlg), "destroy",
			      GTK_SIGNAL_FUNC(dlg_destroy), GTK_OBJECT(dlg));
    gnome_dialog_set_default(GNOME_DIALOG(dlg), dfltbutton + 1);
    gui_gtk_position_in_parent(GTK_WIDGET(gui.mainwin),
			       GTK_WIDGET(dlg), VW_POS_MOUSE);

    return (1 + gnome_dialog_run_and_close(GNOME_DIALOG(dlg)));
}

# endif /* FEAT_GUI_GNOME */

typedef struct _ButtonData
{
    int		*status;
    int		index;
    GtkWidget	*dialog;
} ButtonData;

typedef struct _CancelData
{
    int		*status;
    int		ignore_enter;
    GtkWidget	*dialog;
} CancelData;

/* ARGSUSED */
    static void
dlg_button_clicked(GtkWidget * widget, ButtonData *data)
{
    *(data->status) = data->index + 1;
    dlg_destroy(data->dialog);
}

/*
 * This makes the Escape key equivalent to the cancel button.
 */
/*ARGSUSED*/
    static int
dlg_key_press_event(GtkWidget * widget, GdkEventKey * event, CancelData *data)
{
    /* Ignore hitting Enter when there is no default button. */
    if (data->ignore_enter && event->keyval == GDK_Return)
	return TRUE;

    if (event->keyval != GDK_Escape && event->keyval != GDK_Return)
	return FALSE;

    /* The result value of 0 from a dialog is signaling cancelation.
     * 1 means OK. */
    *(data->status) = (event->keyval == GDK_Return);
    dlg_destroy(data->dialog);

    return TRUE;
}

/*
 * Callback function for when the dialog was destroyed by a window manager.
 */
    static void
dlg_destroy_cb(int *p)
{
    *p = TRUE;		/* set dialog_destroyed to break out of the loop */
    if (gtk_main_level() > 0)
	gtk_main_quit();
}

/* ARGSUSED */
    int
gui_mch_dialog(	int	type,		/* type of dialog */
		char_u	*title,		/* title of dialog */
		char_u	*message,	/* message text */
		char_u	*buttons,	/* names of buttons */
		int	def_but,	/* default button */
		char_u	*textfield)	/* text for textfield or NULL */
{
    char_u	*names;
    char_u	*p;
    int		i;
    int		butcount;
    int		dialog_status = -1;
    int		dialog_destroyed = FALSE;
    int		vertical;

    GtkWidget		*dialog;
    GtkWidget		*frame;
    GtkWidget		*vbox;
    GtkWidget		*table;
    GtkWidget		*dialogmessage;
    GtkWidget		*action_area;
    GtkWidget		*sub_area;
    GtkWidget		*separator;
    GtkAccelGroup	*accel_group;
    GtkWidget		*pixmap;
    GdkPixmap		*icon = NULL;
    GdkBitmap		*mask = NULL;
    char		**icon_data = NULL;

    GtkWidget		**button;
    ButtonData		*data;
    CancelData		cancel_data;

    /* if our pointer is currently hidden, then we should show it. */
    gui_mch_mousehide(FALSE);

# ifdef FEAT_GUI_GNOME
    /* If Gnome is available, use it for the dialog. */
    if (gtk_socket_id == 0)
	return gui_gnome_dialog(type, title, message, buttons, def_but,
								   textfield);
# endif

    if (title == NULL)
	title = (char_u *)_("Vim dialog...");

    if ((type < 0) || (type > VIM_LAST_TYPE))
	type = VIM_GENERIC;

    /* Check 'v' flag in 'guioptions': vertical button placement. */
    vertical = (vim_strchr(p_go, GO_VERTICAL) != NULL);

    dialog = gtk_window_new(GTK_WINDOW_DIALOG);
    gtk_window_set_title(GTK_WINDOW(dialog), (const gchar *)title);
    gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(gui.mainwin));
    gtk_widget_realize(dialog);
    gdk_window_set_decorations(dialog->window, GDK_DECOR_BORDER);
    gdk_window_set_functions(dialog->window, GDK_FUNC_MOVE);

    cancel_data.status = &dialog_status;
    cancel_data.dialog = dialog;
    gtk_signal_connect_after(GTK_OBJECT(dialog), "key_press_event",
		    GTK_SIGNAL_FUNC(dlg_key_press_event),
		    (gpointer) &cancel_data);
    /* Catch the destroy signal, otherwise we don't notice a window manager
     * destroying the dialog window. */
    gtk_signal_connect_object(GTK_OBJECT(dialog), "destroy",
		    GTK_SIGNAL_FUNC(dlg_destroy_cb),
		    (gpointer)&dialog_destroyed);

    gtk_grab_add(dialog);

    /* this makes it look beter on Motif style window managers */
    frame = gtk_frame_new(NULL);
    gtk_container_add(GTK_CONTAINER(dialog), frame);
    gtk_widget_show(frame);

    vbox = gtk_vbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(frame), vbox);
    gtk_widget_show(vbox);

    table = gtk_table_new(1, 3, FALSE);
    gtk_table_set_row_spacings(GTK_TABLE(table), 4);
    gtk_table_set_col_spacings(GTK_TABLE(table), 8);
    gtk_container_border_width(GTK_CONTAINER(table), 4);
    gtk_box_pack_start(GTK_BOX(vbox), table, 4, 4, 0);
    gtk_widget_show(table);

    /* Add pixmap */
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
    icon = gdk_pixmap_colormap_create_from_xpm_d(NULL,
				     gtk_widget_get_colormap(dialog),
				     &mask, NULL, icon_data);
    if (icon)
    {
	pixmap = gtk_pixmap_new(icon, mask);
	/* gtk_misc_set_alignment(GTK_MISC(pixmap), 0.5, 0.5); */
	gtk_table_attach_defaults(GTK_TABLE(table), pixmap, 0, 1, 0, 1);
	gtk_widget_show(pixmap);
    }

    /* Add label */
    dialogmessage = gtk_label_new((const gchar *)message);
    gtk_table_attach_defaults(GTK_TABLE(table), dialogmessage, 1, 2, 0, 1);
    gtk_widget_show(dialogmessage);

    dialog_textfield = textfield;
    if (textfield != NULL)
    {
	/* Add text entry field */
	dialog_textentry = gtk_entry_new();
	gtk_widget_set_usize(dialog_textentry, 400, -2);
	gtk_box_pack_start(GTK_BOX(vbox), dialog_textentry, TRUE, TRUE, 0);
	gtk_entry_set_text(GTK_ENTRY(dialog_textentry),
						    (const gchar *)textfield);
	gtk_entry_select_region(GTK_ENTRY(dialog_textentry), 0,
							   STRLEN(textfield));
	gtk_entry_set_max_length(GTK_ENTRY(dialog_textentry), IOSIZE - 1);
	gtk_entry_set_position(GTK_ENTRY(dialog_textentry), STRLEN(textfield));
	gtk_widget_show(dialog_textentry);
    }

    /* Add box for buttons */
    action_area = gtk_hbox_new(FALSE, 0);
    gtk_container_border_width(GTK_CONTAINER(action_area), 4);
    gtk_box_pack_end(GTK_BOX(vbox), action_area, FALSE, TRUE, 0);
    gtk_widget_show(action_area);

    /* Add a [vh]box in the hbox to center the buttons in the dialog. */
    if (vertical)
	sub_area = gtk_vbox_new(FALSE, 0);
    else
	sub_area = gtk_hbox_new(FALSE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(sub_area), 0);
    gtk_box_pack_start(GTK_BOX(action_area), sub_area, TRUE, FALSE, 0);
    gtk_widget_show(sub_area);

    /*
     * Create the buttons.
     */

    /*
     * Translate the Vim accelerator character into an underscore for GTK+.
     * Double underscores to keep them in the label.
     */
    /* count the number of underscores */
    i = 1;
    for (p = buttons; *p; ++p)
	if (*p == '_')
	    ++i;

    /* make a copy of "buttons" with the translated characters */
    names = alloc(STRLEN(buttons) + i);
    if (names == NULL)
	return -1;

    p = names;
    for (i = 0; buttons[i]; ++i)
    {
	if (buttons[i] == DLG_HOTKEY_CHAR)
	    *p++ = '_';
	else
	{
	    if (buttons[i] == '_')
		*p++ = '_';
	    *p++ = buttons[i];
	}
    }
    *p = NUL;

    /* Count the number of buttons and allocate button[] and data[]. */
    butcount = 1;
    for (p = names; *p; ++p)
	if (*p == DLG_BUTTON_SEP)
	    ++butcount;
    button = (GtkWidget **)alloc((unsigned)(butcount * sizeof(GtkWidget *)));
    data = (ButtonData *)alloc((unsigned)(butcount * sizeof(ButtonData)));
    if (button == NULL || data == NULL)
    {
	vim_free(names);
	vim_free(button);
	vim_free(data);
	return -1;
    }

    /* Attach the new accelerator group to the window. */
    accel_group = gtk_accel_group_new();
    gtk_accel_group_attach(accel_group, GTK_OBJECT(dialog));

    p = names;
    for (butcount = 0; *p; ++butcount)
    {
	char_u		*next;
	GtkWidget	*label;
# ifdef GTK_USE_ACCEL
	guint		accel_key;
# endif

	/* Chunk out this single button. */
	for (next = p; *next; ++next)
	{
	    if (*next == DLG_BUTTON_SEP)
	    {
		*next++ = NUL;
		break;
	    }
	}

	button[butcount] = gtk_button_new();
	GTK_WIDGET_SET_FLAGS(button[butcount], GTK_CAN_DEFAULT);

	label = gtk_accel_label_new("");
	gtk_accel_label_set_accel_widget(GTK_ACCEL_LABEL(label), dialog);

# ifdef GTK_USE_ACCEL
	accel_key = gtk_label_parse_uline(GTK_LABEL(label), (const gchar *)p);
	/* Don't add accelator if 'winaltkeys' is "no". */
	if (accel_key != GDK_VoidSymbol)
	{
	    gtk_widget_add_accelerator(button[butcount],
		    "clicked",
		    accel_group,
		    accel_key, 0,
		    (GtkAccelFlags)0);
	}
# else
	(void)gtk_label_parse_uline(GTK_LABEL(label), (const gchar *)p);
# endif

	gtk_container_add(GTK_CONTAINER(button[butcount]), label);
	gtk_widget_show_all(button[butcount]);

	data[butcount].status = &dialog_status;
	data[butcount].index = butcount;
	data[butcount].dialog = dialog;
	gtk_signal_connect(GTK_OBJECT(button[butcount]),
			   (const char *)"clicked",
			   GTK_SIGNAL_FUNC(dlg_button_clicked),
			   (gpointer) &data[butcount]);

	gtk_box_pack_start(GTK_BOX(sub_area), button[butcount],
			   TRUE, FALSE, 0);
	p = next;
    }

    vim_free(names);

    cancel_data.ignore_enter = FALSE;
    if (butcount > 0)
    {
	--def_but;		/* 1 is first button */
	if (def_but >= butcount)
	    def_but = -1;
	if (def_but >= 0)
	{
	    gtk_widget_grab_focus(button[def_but]);
	    gtk_widget_grab_default(button[def_but]);
	}
	else
	    /* No default, ignore hitting Enter. */
	    cancel_data.ignore_enter = TRUE;
    }

    if (textfield != NULL)
	gtk_window_set_focus(GTK_WINDOW(dialog), dialog_textentry);

    separator = gtk_hseparator_new();
    gtk_box_pack_end(GTK_BOX(vbox), separator, FALSE, TRUE, 0);
    gtk_widget_show(separator);

    dialog_status = -1;

    gui_gtk_position_in_parent(GTK_WIDGET(gui.mainwin),
					    GTK_WIDGET(dialog), VW_POS_MOUSE);

    gtk_widget_show(dialog);

    /* loop here until the dialog goes away */
    while (dialog_status == -1 && !dialog_destroyed
					       && GTK_WIDGET_DRAWABLE(dialog))
	gtk_main_iteration_do(TRUE);

    if (dialog_status < 0)
	dialog_status = 0;
    if (dialog_status != 1 && textfield != NULL)
	*textfield = NUL;	/* dialog was cancelled */

    /* let the garbage collector know that we don't need it any longer */
    gtk_accel_group_unref(accel_group);

    vim_free(button);
    vim_free(data);

    return dialog_status;
}

#endif /* FEAT_GUI_DIALOG && !HAVE_GTK2 */


#if (defined(FEAT_GUI_DIALOG) && defined(HAVE_GTK2)) || defined(PROTO)

    static GtkWidget *
create_message_dialog(int type, char_u *title, char_u *message)
{
    GtkWidget	    *dialog;
    GtkMessageType  message_type;

    switch (type)
    {
	case VIM_ERROR:	    message_type = GTK_MESSAGE_ERROR;	 break;
	case VIM_WARNING:   message_type = GTK_MESSAGE_WARNING;	 break;
	case VIM_QUESTION:  message_type = GTK_MESSAGE_QUESTION; break;
	default:	    message_type = GTK_MESSAGE_INFO;	 break;
    }

    message = CONVERT_TO_UTF8(message);
    dialog  = gtk_message_dialog_new(GTK_WINDOW(gui.mainwin),
				     GTK_DIALOG_DESTROY_WITH_PARENT,
				     message_type,
				     GTK_BUTTONS_NONE,
				     "%s", (const char *)message);
    CONVERT_TO_UTF8_FREE(message);

    if (title != NULL)
    {
	title = CONVERT_TO_UTF8(title);
	gtk_window_set_title(GTK_WINDOW(dialog), (const char *)title);
	CONVERT_TO_UTF8_FREE(title);
    }
    else if (type == VIM_GENERIC)
    {
	gtk_window_set_title(GTK_WINDOW(dialog), "VIM");
    }

    return dialog;
}

/*
 * Split up button_string into individual button labels by inserting
 * NUL bytes.  Also replace the Vim-style mnemonic accelerator prefix
 * '&' with '_'.  button_string must point to allocated memory!
 * Return an allocated array of pointers into button_string.
 */
    static char **
split_button_string(char_u *button_string, int *n_buttons)
{
    char	    **array;
    char_u	    *p;
    unsigned int    count = 1;

    for (p = button_string; *p != NUL; ++p)
	if (*p == DLG_BUTTON_SEP)
	    ++count;

    array = (char **)alloc((count + 1) * sizeof(char *));
    count = 0;

    if (array != NULL)
    {
	array[count++] = (char *)button_string;
	for (p = button_string; *p != NUL; )
	{
	    if (*p == DLG_BUTTON_SEP)
	    {
		*p++ = NUL;
		array[count++] = (char *)p;
	    }
	    else if (*p == DLG_HOTKEY_CHAR)
		*p++ = '_';
	    else
		mb_ptr_adv(p);
	}
	array[count] = NULL; /* currently not relied upon, but doesn't hurt */
    }

    *n_buttons = count;
    return array;
}

    static char **
split_button_translation(const char *message)
{
    char    **buttons = NULL;
    char_u  *str;
    int	    n_buttons = 0;
    int	    n_expected = 1;

    for (str = (char_u *)message; *str != NUL; ++str)
	if (*str == DLG_BUTTON_SEP)
	    ++n_expected;

    str = (char_u *)_(message);
    if (str != NULL)
    {
	if (output_conv.vc_type != CONV_NONE)
	    str = string_convert(&output_conv, str, NULL);
	else
	    str = vim_strsave(str);

	if (str != NULL)
	    buttons = split_button_string(str, &n_buttons);
    }
    /*
     * Uh-oh... this should never ever happen.	But we don't wanna crash
     * if the translation is broken, thus fall back to the untranslated
     * buttons string in case of emergency.
     */
    if (buttons == NULL || n_buttons != n_expected)
    {
	vim_free(buttons);
	vim_free(str);
	buttons = NULL;
	str = vim_strsave((char_u *)message);

	if (str != NULL)
	    buttons = split_button_string(str, &n_buttons);
	if (buttons == NULL)
	    vim_free(str);
    }

    return buttons;
}

    static int
button_equal(const char *a, const char *b)
{
    while (*a != '\0' && *b != '\0')
    {
	if (*a == '_' && *++a == '\0')
	    break;
	if (*b == '_' && *++b == '\0')
	    break;

	if (g_unichar_tolower(g_utf8_get_char(a))
		!= g_unichar_tolower(g_utf8_get_char(b)))
	    return FALSE;

	a = g_utf8_next_char(a);
	b = g_utf8_next_char(b);
    }

    return (*a == '\0' && *b == '\0');
}

    static void
dialog_add_buttons(GtkDialog *dialog, char_u *button_string)
{
    char    **ok;
    char    **ync;  /* "yes no cancel" */
    char    **buttons;
    int	    n_buttons = 0;
    int	    index;

    button_string = vim_strsave(button_string); /* must be writable */
    if (button_string == NULL)
	return;

    /* Check 'v' flag in 'guioptions': vertical button placement. */
    if (vim_strchr(p_go, GO_VERTICAL) != NULL)
    {
	GtkWidget	*vbutton_box;

	vbutton_box = gtk_vbutton_box_new();
	gtk_widget_show(vbutton_box);
	gtk_box_pack_end(GTK_BOX(GTK_DIALOG(dialog)->vbox),
						 vbutton_box, TRUE, FALSE, 0);
	/* Overrule the "action_area" value, hopefully this works... */
	GTK_DIALOG(dialog)->action_area = vbutton_box;
    }

    /*
     * Yes this is ugly, I don't particularly like it either.  But doing it
     * this way has the compelling advantage that translations need not to
     * be touched at all.  See below what 'ok' and 'ync' are used for.
     */
    ok	    = split_button_translation(N_("&Ok"));
    ync     = split_button_translation(N_("&Yes\n&No\n&Cancel"));
    buttons = split_button_string(button_string, &n_buttons);

    /*
     * Yes, the buttons are in reversed order to match the GNOME 2 desktop
     * environment.  Don't hit me -- it's all about consistency.
     * Well, apparently somebody changed his mind: with GTK 2.2.4 it works the
     * other way around...
     */
    for (index = 1; index <= n_buttons; ++index)
    {
	char	*label;
	char_u	*label8;

	label = buttons[index - 1];
	/*
	 * Perform some guesswork to find appropriate stock items for the
	 * buttons.  We have to compare with a sample of the translated
	 * button string to get things right.  Yes, this is hackish :/
	 *
	 * But even the common button labels aren't necessarily translated,
	 * since anyone can create their own dialogs using Vim functions.
	 * Thus we have to check for those too.
	 */
	if (ok != NULL && ync != NULL) /* almost impossible to fail */
	{
	    if	    (button_equal(label, ok[0]))    label = GTK_STOCK_OK;
	    else if (button_equal(label, ync[0]))   label = GTK_STOCK_YES;
	    else if (button_equal(label, ync[1]))   label = GTK_STOCK_NO;
	    else if (button_equal(label, ync[2]))   label = GTK_STOCK_CANCEL;
	    else if (button_equal(label, "Ok"))     label = GTK_STOCK_OK;
	    else if (button_equal(label, "Yes"))    label = GTK_STOCK_YES;
	    else if (button_equal(label, "No"))     label = GTK_STOCK_NO;
	    else if (button_equal(label, "Cancel")) label = GTK_STOCK_CANCEL;
	}
	label8 = CONVERT_TO_UTF8((char_u *)label);
	gtk_dialog_add_button(dialog, (const gchar *)label8, index);
	CONVERT_TO_UTF8_FREE(label8);
    }

    if (ok != NULL)
	vim_free(*ok);
    if (ync != NULL)
	vim_free(*ync);
    vim_free(ok);
    vim_free(ync);
    vim_free(buttons);
    vim_free(button_string);
}

/*
 * Allow mnemonic accelerators to be activated without pressing <Alt>.
 * I'm not sure if it's a wise idea to do this.  However, the old GTK+ 1.2
 * GUI used to work this way, and I consider the impact on UI consistency
 * low enough to justify implementing this as a special Vim feature.
 */
typedef struct _DialogInfo
{
    int		ignore_enter;	    /* no default button, ignore "Enter" */
    int		noalt;		    /* accept accelerators without Alt */
    GtkDialog	*dialog;	    /* Widget of the dialog */
} DialogInfo;

/*ARGSUSED2*/
    static gboolean
dialog_key_press_event_cb(GtkWidget *widget, GdkEventKey *event, gpointer data)
{
    DialogInfo *di = (DialogInfo *)data;

    /* Close the dialog when hitting "Esc". */
    if (event->keyval == GDK_Escape)
    {
	gtk_dialog_response(di->dialog, GTK_RESPONSE_REJECT);
	return TRUE;
    }

    if (di->noalt
	      && (event->state & gtk_accelerator_get_default_mod_mask()) == 0)
    {
	return gtk_window_mnemonic_activate(
		   GTK_WINDOW(widget), event->keyval,
		   gtk_window_get_mnemonic_modifier(GTK_WINDOW(widget)));
    }

    return FALSE; /* continue emission */
}

    int
gui_mch_dialog(int	type,	    /* type of dialog */
	       char_u	*title,	    /* title of dialog */
	       char_u	*message,   /* message text */
	       char_u	*buttons,   /* names of buttons */
	       int	def_but,    /* default button */
	       char_u	*textfield) /* text for textfield or NULL */
{
    GtkWidget	*dialog;
    GtkWidget	*entry = NULL;
    char_u	*text;
    int		response;
    DialogInfo  dialoginfo;

    dialog = create_message_dialog(type, title, message);
    dialoginfo.dialog = GTK_DIALOG(dialog);
    dialog_add_buttons(GTK_DIALOG(dialog), buttons);

    if (textfield != NULL)
    {
	GtkWidget *alignment;

	entry = gtk_entry_new();
	gtk_widget_show(entry);

	text = CONVERT_TO_UTF8(textfield);
	gtk_entry_set_text(GTK_ENTRY(entry), (const char *)text);
	CONVERT_TO_UTF8_FREE(text);

	alignment = gtk_alignment_new((float)0.5, (float)0.5,
						      (float)1.0, (float)1.0);
	gtk_container_add(GTK_CONTAINER(alignment), entry);
	gtk_container_set_border_width(GTK_CONTAINER(alignment), 5);
	gtk_widget_show(alignment);

	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox),
			   alignment, TRUE, FALSE, 0);
	dialoginfo.noalt = FALSE;
    }
    else
	dialoginfo.noalt = TRUE;

    /* Allow activation of mnemonic accelerators without pressing <Alt> when
     * there is no textfield.  Handle pressing Esc. */
    g_signal_connect(G_OBJECT(dialog), "key_press_event",
			 G_CALLBACK(&dialog_key_press_event_cb), &dialoginfo);

    if (def_but > 0)
    {
	gtk_dialog_set_default_response(GTK_DIALOG(dialog), def_but);
	dialoginfo.ignore_enter = FALSE;
    }
    else
	/* No default button, ignore pressing Enter. */
	dialoginfo.ignore_enter = TRUE;

    /* Show the mouse pointer if it's currently hidden. */
    gui_mch_mousehide(FALSE);

    response = gtk_dialog_run(GTK_DIALOG(dialog));

    /* GTK_RESPONSE_NONE means the dialog was programmatically destroyed. */
    if (response != GTK_RESPONSE_NONE)
    {
	if (response == GTK_RESPONSE_ACCEPT)	    /* Enter pressed */
	    response = def_but;
	if (textfield != NULL)
	{
	    text = (char_u *)gtk_entry_get_text(GTK_ENTRY(entry));
	    text = CONVERT_FROM_UTF8(text);

	    vim_strncpy(textfield, text, IOSIZE - 1);

	    CONVERT_FROM_UTF8_FREE(text);
	}
	gtk_widget_destroy(dialog);
    }

    /* Terrible hack: When the text area still has focus when we remove the
     * dialog, somehow gvim loses window focus.  This is with "point to type"
     * in the KDE 3.1 window manager.  Warp the mouse pointer to outside the
     * window and back to avoid that. */
    if (!gui.in_focus)
    {
	int x, y;

	gdk_window_get_pointer(gui.drawarea->window, &x, &y, NULL);
	gui_mch_setmouse(-100, -100);
	gui_mch_setmouse(x, y);
    }

    return response > 0 ? response : 0;
}

#endif /* FEAT_GUI_DIALOG && HAVE_GTK2 */


#if defined(FEAT_MENU) || defined(PROTO)

    void
gui_mch_show_popupmenu(vimmenu_T *menu)
{
# if defined(FEAT_XIM) && defined(HAVE_GTK2)
    /*
     * Append a submenu for selecting an input method.	This is
     * currently the only way to switch input methods at runtime.
     */
    if (xic != NULL && g_object_get_data(G_OBJECT(menu->submenu_id),
					 "vim-has-im-menu") == NULL)
    {
	GtkWidget   *menuitem;
	GtkWidget   *submenu;
	char_u	    *name;

	menuitem = gtk_separator_menu_item_new();
	gtk_widget_show(menuitem);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu->submenu_id), menuitem);

	name = (char_u *)_("Input _Methods");
	name = CONVERT_TO_UTF8(name);
	menuitem = gtk_menu_item_new_with_mnemonic((const char *)name);
	CONVERT_TO_UTF8_FREE(name);
	gtk_widget_show(menuitem);

	submenu = gtk_menu_new();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menuitem), submenu);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu->submenu_id), menuitem);

	gtk_im_multicontext_append_menuitems(GTK_IM_MULTICONTEXT(xic),
					     GTK_MENU_SHELL(submenu));
	g_object_set_data(G_OBJECT(menu->submenu_id),
			  "vim-has-im-menu", GINT_TO_POINTER(TRUE));
    }
# endif /* FEAT_XIM && HAVE_GTK2 */

    gtk_menu_popup(GTK_MENU(menu->submenu_id),
		   NULL, NULL,
		   (GtkMenuPositionFunc)NULL, NULL,
		   3U, (guint32)GDK_CURRENT_TIME);
}

/* Ugly global variable to pass "mouse_pos" flag from gui_make_popup() to
 * popup_menu_position_func(). */
static int popup_mouse_pos;

/*
 * Menu position callback; used by gui_make_popup() to place the menu
 * at the current text cursor position.
 *
 * Note: The push_in output argument seems to affect scrolling of huge
 * menus that don't fit on the screen.	Leave it at the default for now.
 */
/*ARGSUSED0*/
    static void
popup_menu_position_func(GtkMenu *menu,
			 gint *x, gint *y,
# ifdef HAVE_GTK2
			 gboolean *push_in,
# endif
			 gpointer user_data)
{
    gdk_window_get_origin(gui.drawarea->window, x, y);

    if (popup_mouse_pos)
    {
	int	mx, my;

	gui_mch_getmouse(&mx, &my);
	*x += mx;
	*y += my;
    }
    else if (curwin != NULL && gui.drawarea != NULL && gui.drawarea->window != NULL)
    {
	/* Find the cursor position in the current window */
	*x += FILL_X(W_WINCOL(curwin) + curwin->w_wcol + 1) + 1;
	*y += FILL_Y(W_WINROW(curwin) + curwin->w_wrow + 1) + 1;
    }
}

    void
gui_make_popup(char_u *path_name, int mouse_pos)
{
    vimmenu_T *menu;

    popup_mouse_pos = mouse_pos;

    menu = gui_find_menu(path_name);

    if (menu != NULL && menu->submenu_id != NULL)
    {
	gtk_menu_popup(GTK_MENU(menu->submenu_id),
		       NULL, NULL,
		       &popup_menu_position_func, NULL,
		       0U, (guint32)GDK_CURRENT_TIME);
    }
}

#endif /* FEAT_MENU */


/*
 * We don't create it twice.
 */

typedef struct _SharedFindReplace
{
    GtkWidget *dialog;	/* the main dialog widget */
    GtkWidget *wword;	/* 'Whole word only' check button */
    GtkWidget *mcase;	/* 'Match case' check button */
    GtkWidget *up;	/* search direction 'Up' radio button */
    GtkWidget *down;	/* search direction 'Down' radio button */
    GtkWidget *what;	/* 'Find what' entry text widget */
    GtkWidget *with;	/* 'Replace with' entry text widget */
    GtkWidget *find;	/* 'Find Next' action button */
    GtkWidget *replace;	/* 'Replace With' action button */
    GtkWidget *all;	/* 'Replace All' action button */
} SharedFindReplace;

static SharedFindReplace find_widgets = { NULL, };
static SharedFindReplace repl_widgets = { NULL, };

/* ARGSUSED */
    static int
find_key_press_event(
		GtkWidget	*widget,
		GdkEventKey	*event,
		SharedFindReplace *frdp)
{
    /* If the user is holding one of the key modifiers we will just bail out,
     * thus preserving the possibility of normal focus traversal.
     */
    if (event->state & (GDK_CONTROL_MASK | GDK_SHIFT_MASK))
	return FALSE;

    /* the Escape key synthesizes a cancellation action */
    if (event->keyval == GDK_Escape)
    {
	gtk_widget_hide(frdp->dialog);

	return TRUE;
    }
    /*
     * What the **** is this for?  Disabled for GTK+ 2 because due to
     * gtk_signal_connect_after() it doesn't have any effect anyway.
     * (Fortunately.)
     */
#ifndef HAVE_GTK2
    /* block traversal resulting from those keys */
    if (event->keyval == GDK_Left
	    || event->keyval == GDK_Right
	    || event->keyval == GDK_space)
	return TRUE;
#endif

    /* It would be delightfull if it where possible to do search history
     * operations on the K_UP and K_DOWN keys here.
     */

    return FALSE;
}

#ifdef HAVE_GTK2
    static GtkWidget *
create_image_button(const char *stock_id, const char *label)
{
    char_u	*text;
    GtkWidget	*box;
    GtkWidget	*alignment;
    GtkWidget	*button;

    text = CONVERT_TO_UTF8((char_u *)label);

    box = gtk_hbox_new(FALSE, 3);
    gtk_box_pack_start(GTK_BOX(box),
		       gtk_image_new_from_stock(stock_id, GTK_ICON_SIZE_BUTTON),
		       FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box),
		       gtk_label_new((const char *)text),
		       FALSE, FALSE, 0);

    CONVERT_TO_UTF8_FREE(text);

    alignment = gtk_alignment_new((float)0.5, (float)0.5,
						      (float)0.0, (float)0.0);
    gtk_container_add(GTK_CONTAINER(alignment), box);
    gtk_widget_show_all(alignment);

    button = gtk_button_new();
    gtk_container_add(GTK_CONTAINER(button), alignment);

    return button;
}

/*
 * This is currently only used by find_replace_dialog_create(), and
 * I'd really like to keep it at that.	In other words: don't spread
 * this nasty hack all over the code.  Think twice.
 */
    static const char *
convert_localized_message(char_u **buffer, const char *message)
{
    if (output_conv.vc_type == CONV_NONE)
	return message;

    vim_free(*buffer);
    *buffer = string_convert(&output_conv, (char_u *)message, NULL);

    return (const char *)*buffer;
}
#endif /* HAVE_GTK2 */

    static void
find_replace_dialog_create(char_u *arg, int do_replace)
{
#ifndef HAVE_GTK2
    GtkWidget	*frame;
#endif
    GtkWidget	*hbox;		/* main top down box */
    GtkWidget	*actionarea;
    GtkWidget	*table;
    GtkWidget	*tmp;
    GtkWidget	*vbox;
    gboolean	sensitive;
    SharedFindReplace *frdp;
    char_u	*entry_text;
    int		wword = FALSE;
    int		mcase = !p_ic;
#ifdef HAVE_GTK2
    char_u	*conv_buffer = NULL;
#   define CONV(message) convert_localized_message(&conv_buffer, (message))
#else
#   define CONV(message) (message)
#endif

    frdp = (do_replace) ? (&repl_widgets) : (&find_widgets);

    /* Get the search string to use. */
    entry_text = get_find_dialog_text(arg, &wword, &mcase);

#ifdef HAVE_GTK2
    if (entry_text != NULL && output_conv.vc_type != CONV_NONE)
    {
	char_u *old_text = entry_text;
	entry_text = string_convert(&output_conv, entry_text, NULL);
	vim_free(old_text);
    }
#endif

    /*
     * If the dialog already exists, just raise it.
     */
    if (frdp->dialog)
    {
#ifndef HAVE_GTK2
	/* always make the dialog appear where you want it even if the mainwin
	 * has moved -- dbv */
	gui_gtk_position_in_parent(GTK_WIDGET(gui.mainwin),
				      GTK_WIDGET(frdp->dialog), VW_POS_MOUSE);
	gui_gtk_synch_fonts();

	if (!GTK_WIDGET_VISIBLE(frdp->dialog))
	{
	    gtk_widget_grab_focus(frdp->what);
	    gtk_widget_show(frdp->dialog);
	}
#endif
	if (entry_text != NULL)
	{
	    gtk_entry_set_text(GTK_ENTRY(frdp->what), (char *)entry_text);
	    gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(frdp->wword),
							     (gboolean)wword);
	    gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(frdp->mcase),
							     (gboolean)mcase);
	}
#ifdef HAVE_GTK2
	gtk_window_present(GTK_WINDOW(frdp->dialog));
#else
	gdk_window_raise(frdp->dialog->window);
#endif
	vim_free(entry_text);
	return;
    }

#ifdef HAVE_GTK2
    frdp->dialog = gtk_dialog_new();
    gtk_dialog_set_has_separator(GTK_DIALOG(frdp->dialog), FALSE);
    gtk_window_set_transient_for(GTK_WINDOW(frdp->dialog), GTK_WINDOW(gui.mainwin));
    gtk_window_set_destroy_with_parent(GTK_WINDOW(frdp->dialog), TRUE);
#else
    frdp->dialog = gtk_window_new(GTK_WINDOW_DIALOG);
#endif

    if (do_replace)
    {
#ifndef HAVE_GTK2
	gtk_window_set_wmclass(GTK_WINDOW(frdp->dialog), "searchrepl", "gvim");
#endif
	gtk_window_set_title(GTK_WINDOW(frdp->dialog),
			     CONV(_("VIM - Search and Replace...")));
    }
    else
    {
#ifndef HAVE_GTK2
	gtk_window_set_wmclass(GTK_WINDOW(frdp->dialog), "search", "gvim");
#endif
	gtk_window_set_title(GTK_WINDOW(frdp->dialog),
			     CONV(_("VIM - Search...")));
    }

#ifndef HAVE_GTK2 /* Utter crack.  Shudder. */
    gtk_widget_realize(frdp->dialog);
    gdk_window_set_decorations(frdp->dialog->window,
	    GDK_DECOR_TITLE | GDK_DECOR_BORDER | GDK_DECOR_RESIZEH);
    gdk_window_set_functions(frdp->dialog->window,
	    GDK_FUNC_RESIZE | GDK_FUNC_MOVE);
#endif

#ifndef HAVE_GTK2
    /* this makes it look better on Motif style window managers */
    frame = gtk_frame_new(NULL);
    gtk_container_add(GTK_CONTAINER(frdp->dialog), frame);
#endif

    hbox = gtk_hbox_new(FALSE, 0);
#ifdef HAVE_GTK2
    gtk_container_set_border_width(GTK_CONTAINER(hbox), 10);
    gtk_container_add(GTK_CONTAINER(GTK_DIALOG(frdp->dialog)->vbox), hbox);
#else
    gtk_container_add(GTK_CONTAINER(frame), hbox);
#endif

    if (do_replace)
	table = gtk_table_new(1024, 4, FALSE);
    else
	table = gtk_table_new(1024, 3, FALSE);
    gtk_box_pack_start(GTK_BOX(hbox), table, TRUE, TRUE, 0);
    gtk_container_border_width(GTK_CONTAINER(table), 4);

    tmp = gtk_label_new(CONV(_("Find what:")));
    gtk_misc_set_alignment(GTK_MISC(tmp), (gfloat)0.0, (gfloat)0.5);
    gtk_table_attach(GTK_TABLE(table), tmp, 0, 1, 0, 1,
		     GTK_FILL, GTK_EXPAND, 2, 2);
    frdp->what = gtk_entry_new();
    sensitive = (entry_text != NULL && entry_text[0] != NUL);
    if (entry_text != NULL)
	gtk_entry_set_text(GTK_ENTRY(frdp->what), (char *)entry_text);
    gtk_signal_connect(GTK_OBJECT(frdp->what), "changed",
		       GTK_SIGNAL_FUNC(entry_changed_cb), frdp->dialog);
    gtk_signal_connect_after(GTK_OBJECT(frdp->what), "key_press_event",
				 GTK_SIGNAL_FUNC(find_key_press_event),
				 (gpointer) frdp);
    gtk_table_attach(GTK_TABLE(table), frdp->what, 1, 1024, 0, 1,
		     GTK_EXPAND | GTK_FILL, GTK_EXPAND, 2, 2);

    if (do_replace)
    {
	tmp = gtk_label_new(CONV(_("Replace with:")));
	gtk_misc_set_alignment(GTK_MISC(tmp), (gfloat)0.0, (gfloat)0.5);
	gtk_table_attach(GTK_TABLE(table), tmp, 0, 1, 1, 2,
			 GTK_FILL, GTK_EXPAND, 2, 2);
	frdp->with = gtk_entry_new();
	gtk_signal_connect(GTK_OBJECT(frdp->with), "activate",
			   GTK_SIGNAL_FUNC(find_replace_cb),
			   GINT_TO_POINTER(FRD_R_FINDNEXT));
	gtk_signal_connect_after(GTK_OBJECT(frdp->with), "key_press_event",
				 GTK_SIGNAL_FUNC(find_key_press_event),
				 (gpointer) frdp);
	gtk_table_attach(GTK_TABLE(table), frdp->with, 1, 1024, 1, 2,
			 GTK_EXPAND | GTK_FILL, GTK_EXPAND, 2, 2);

	/*
	 * Make the entry activation only change the input focus onto the
	 * with item.
	 */
	gtk_signal_connect(GTK_OBJECT(frdp->what), "activate",
			   GTK_SIGNAL_FUNC(entry_activate_cb), frdp->with);
    }
    else
    {
	/*
	 * Make the entry activation do the search.
	 */
	gtk_signal_connect(GTK_OBJECT(frdp->what), "activate",
			   GTK_SIGNAL_FUNC(find_replace_cb),
			   GINT_TO_POINTER(FRD_FINDNEXT));
    }

    /* whole word only button */
    frdp->wword = gtk_check_button_new_with_label(CONV(_("Match whole word only")));
    gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(frdp->wword),
							(gboolean)wword);
    if (do_replace)
	gtk_table_attach(GTK_TABLE(table), frdp->wword, 0, 1023, 2, 3,
			 GTK_FILL, GTK_EXPAND, 2, 2);
    else
	gtk_table_attach(GTK_TABLE(table), frdp->wword, 0, 1023, 1, 2,
			 GTK_FILL, GTK_EXPAND, 2, 2);

    /* match case button */
    frdp->mcase = gtk_check_button_new_with_label(CONV(_("Match case")));
    gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(frdp->mcase),
							     (gboolean)mcase);
    if (do_replace)
	gtk_table_attach(GTK_TABLE(table), frdp->mcase, 0, 1023, 3, 4,
			 GTK_FILL, GTK_EXPAND, 2, 2);
    else
	gtk_table_attach(GTK_TABLE(table), frdp->mcase, 0, 1023, 2, 3,
			 GTK_FILL, GTK_EXPAND, 2, 2);

    tmp = gtk_frame_new(CONV(_("Direction")));
    if (do_replace)
	gtk_table_attach(GTK_TABLE(table), tmp, 1023, 1024, 2, 4,
			 GTK_FILL, GTK_FILL, 2, 2);
    else
	gtk_table_attach(GTK_TABLE(table), tmp, 1023, 1024, 1, 3,
			 GTK_FILL, GTK_FILL, 2, 2);
    vbox = gtk_vbox_new(FALSE, 0);
    gtk_container_border_width(GTK_CONTAINER(vbox), 0);
    gtk_container_add(GTK_CONTAINER(tmp), vbox);

    /* 'Up' and 'Down' buttons */
    frdp->up = gtk_radio_button_new_with_label(NULL, CONV(_("Up")));
    gtk_box_pack_start(GTK_BOX(vbox), frdp->up, TRUE, TRUE, 0);
    frdp->down = gtk_radio_button_new_with_label(
			gtk_radio_button_group(GTK_RADIO_BUTTON(frdp->up)),
			CONV(_("Down")));
    gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(frdp->down), TRUE);
#ifdef HAVE_GTK2
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 2);
#endif
    gtk_box_pack_start(GTK_BOX(vbox), frdp->down, TRUE, TRUE, 0);

    /* vbox to hold the action buttons */
    actionarea = gtk_vbutton_box_new();
    gtk_container_border_width(GTK_CONTAINER(actionarea), 2);
#ifndef HAVE_GTK2
    if (do_replace)
    {
	gtk_button_box_set_layout(GTK_BUTTON_BOX(actionarea),
				  GTK_BUTTONBOX_END);
	gtk_button_box_set_spacing(GTK_BUTTON_BOX(actionarea), 0);
    }
#endif
    gtk_box_pack_end(GTK_BOX(hbox), actionarea, FALSE, FALSE, 0);

    /* 'Find Next' button */
#ifdef HAVE_GTK2
    frdp->find = create_image_button(GTK_STOCK_FIND, _("Find Next"));
#else
    frdp->find = gtk_button_new_with_label(_("Find Next"));
#endif
    gtk_widget_set_sensitive(frdp->find, sensitive);

    gtk_signal_connect(GTK_OBJECT(frdp->find), "clicked",
		       GTK_SIGNAL_FUNC(find_replace_cb),
		       (do_replace) ? GINT_TO_POINTER(FRD_R_FINDNEXT)
				    : GINT_TO_POINTER(FRD_FINDNEXT));

    GTK_WIDGET_SET_FLAGS(frdp->find, GTK_CAN_DEFAULT);
    gtk_box_pack_start(GTK_BOX(actionarea), frdp->find, FALSE, FALSE, 0);
    gtk_widget_grab_default(frdp->find);

    if (do_replace)
    {
	/* 'Replace' button */
#ifdef HAVE_GTK2
	frdp->replace = create_image_button(GTK_STOCK_CONVERT, _("Replace"));
#else
	frdp->replace = gtk_button_new_with_label(_("Replace"));
#endif
	gtk_widget_set_sensitive(frdp->replace, sensitive);
	GTK_WIDGET_SET_FLAGS(frdp->replace, GTK_CAN_DEFAULT);
	gtk_box_pack_start(GTK_BOX(actionarea), frdp->replace, FALSE, FALSE, 0);
	gtk_signal_connect(GTK_OBJECT(frdp->replace), "clicked",
			   GTK_SIGNAL_FUNC(find_replace_cb),
			   GINT_TO_POINTER(FRD_REPLACE));

	/* 'Replace All' button */
#ifdef HAVE_GTK2
	frdp->all = create_image_button(GTK_STOCK_CONVERT, _("Replace All"));
#else
	frdp->all = gtk_button_new_with_label(_("Replace All"));
#endif
	gtk_widget_set_sensitive(frdp->all, sensitive);
	GTK_WIDGET_SET_FLAGS(frdp->all, GTK_CAN_DEFAULT);
	gtk_box_pack_start(GTK_BOX(actionarea), frdp->all, FALSE, FALSE, 0);
	gtk_signal_connect(GTK_OBJECT(frdp->all), "clicked",
			   GTK_SIGNAL_FUNC(find_replace_cb),
			   GINT_TO_POINTER(FRD_REPLACEALL));
    }

    /* 'Cancel' button */
#ifdef HAVE_GTK2
    tmp = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
#else
    tmp = gtk_button_new_with_label(_("Cancel"));
#endif
    GTK_WIDGET_SET_FLAGS(tmp, GTK_CAN_DEFAULT);
    gtk_box_pack_end(GTK_BOX(actionarea), tmp, FALSE, FALSE, 0);
    gtk_signal_connect_object(GTK_OBJECT(tmp),
			      "clicked", GTK_SIGNAL_FUNC(gtk_widget_hide),
			      GTK_OBJECT(frdp->dialog));
    gtk_signal_connect_object(GTK_OBJECT(frdp->dialog),
			      "delete_event", GTK_SIGNAL_FUNC(gtk_widget_hide_on_delete),
			      GTK_OBJECT(frdp->dialog));

    tmp = gtk_vseparator_new();
#ifdef HAVE_GTK2
    gtk_box_pack_end(GTK_BOX(hbox), tmp, FALSE, FALSE, 10);
#else
    gtk_box_pack_end(GTK_BOX(hbox), tmp, FALSE, TRUE, 0);
#endif

#ifndef HAVE_GTK2
    gtk_widget_grab_focus(frdp->what);

    /* show the frame and realize the frdp->dialog this gives us a window size
     * request that we'll use to position the window within the boundary of
     * the mainwin --dbv */
    gtk_widget_show_all(frame);
    gui_gtk_position_in_parent(GTK_WIDGET(gui.mainwin),
				      GTK_WIDGET(frdp->dialog), VW_POS_MOUSE);
    gui_gtk_synch_fonts();
    gtk_widget_show_all(frdp->dialog);
#endif

#ifdef HAVE_GTK2
    /* Suppress automatic show of the unused action area */
    gtk_widget_hide(GTK_DIALOG(frdp->dialog)->action_area);
    gtk_widget_show_all(hbox);
    gtk_widget_show(frdp->dialog);
#endif

    vim_free(entry_text);
#ifdef HAVE_GTK2
    vim_free(conv_buffer);
#endif
#undef CONV
}

    void
gui_mch_find_dialog(exarg_T *eap)
{
    if (gui.in_use)
	find_replace_dialog_create(eap->arg, FALSE);
}

    void
gui_mch_replace_dialog(exarg_T *eap)
{
    if (gui.in_use)
	find_replace_dialog_create(eap->arg, TRUE);
}


#if !defined(HAVE_GTK2) || defined(PROTO)
/*
 * Synchronize all gui elements, which are dependant upon the
 * main text font used. Those are in esp. the find/replace dialogs.
 * If you don't understand why this should be needed, please try to
 * search for "pięść" in iso8859-2.
 *
 * (<danielk> I converted the comment above to UTF-8 to put
 *  a stopper to the encoding mess.  Forgive me :)
 *
 * Obsolete with GTK2.
 */
    void
gui_gtk_synch_fonts(void)
{
    SharedFindReplace *frdp;
    int do_replace;

    /* OK this loop is a bit tricky... */
    for (do_replace = 0; do_replace <= 1; ++do_replace)
    {
	frdp = (do_replace) ? (&repl_widgets) : (&find_widgets);
	if (frdp->dialog)
	{
	    GtkStyle *style;

	    /* synch the font with whats used by the text itself */
	    style = gtk_style_copy(gtk_widget_get_style(frdp->what));
	    gdk_font_unref(style->font);
# ifdef FEAT_XFONTSET
	    if (gui.fontset != NOFONTSET)
		style->font = gui.fontset;
	    else
# endif
		style->font = gui.norm_font;
	    gdk_font_ref(style->font);
	    gtk_widget_set_style(frdp->what, style);
	    gtk_style_unref(style);
	    if (do_replace)
	    {
		style = gtk_style_copy(gtk_widget_get_style(frdp->with));
		gdk_font_unref(style->font);
# ifdef FEAT_XFONTSET
		if (gui.fontset != NOFONTSET)
		    style->font = gui.fontset;
		else
# endif
		    style->font = gui.norm_font;
		gdk_font_ref(style->font);
		gtk_widget_set_style(frdp->with, style);
		gtk_style_unref(style);
	    }
	}
    }
}
#endif /* !HAVE_GTK2 */


/*
 * Callback for actions of the find and replace dialogs
 */
/*ARGSUSED*/
    static void
find_replace_cb(GtkWidget *widget, gpointer data)
{
    int			flags;
    char_u		*find_text;
    char_u		*repl_text;
    gboolean		direction_down;
    SharedFindReplace	*sfr;
    int			rc;

    flags = (int)(long)data;	    /* avoid a lint warning here */

    /* Get the search/replace strings from the dialog */
    if (flags == FRD_FINDNEXT)
    {
	repl_text = NULL;
	sfr = &find_widgets;
    }
    else
    {
	repl_text = (char_u *)gtk_entry_get_text(GTK_ENTRY(repl_widgets.with));
	sfr = &repl_widgets;
    }

    find_text = (char_u *)gtk_entry_get_text(GTK_ENTRY(sfr->what));
    direction_down = GTK_TOGGLE_BUTTON(sfr->down)->active;

    if (GTK_TOGGLE_BUTTON(sfr->wword)->active)
	flags |= FRD_WHOLE_WORD;
    if (GTK_TOGGLE_BUTTON(sfr->mcase)->active)
	flags |= FRD_MATCH_CASE;

#ifdef HAVE_GTK2
    repl_text = CONVERT_FROM_UTF8(repl_text);
    find_text = CONVERT_FROM_UTF8(find_text);
#endif
    rc = gui_do_findrepl(flags, find_text, repl_text, direction_down);
#ifdef HAVE_GTK2
    CONVERT_FROM_UTF8_FREE(repl_text);
    CONVERT_FROM_UTF8_FREE(find_text);
#endif

    if (rc && gtk_main_level() > 0)
	gtk_main_quit(); /* make sure cmd will be handled immediately */
}

/* our usual callback function */
/*ARGSUSED*/
    static void
entry_activate_cb(GtkWidget *widget, gpointer data)
{
    gtk_widget_grab_focus(GTK_WIDGET(data));
}

/*
 * Syncing the find/replace dialogs on the fly is utterly useless crack,
 * and causes nothing but problems.  Please tell me a use case for which
 * you'd need both a find dialog and a find/replace one at the same time,
 * without being able to actually use them separately since they're syncing
 * all the time.  I don't think it's worthwhile to fix this nonsense,
 * particularly evil incarnation of braindeadness, whatever; I'd much rather
 * see it extinguished from this planet.  Thanks for listening.  Sorry.
 */
    static void
entry_changed_cb(GtkWidget * entry, GtkWidget * dialog)
{
    const gchar	*entry_text;
    gboolean	nonempty;

    entry_text = gtk_entry_get_text(GTK_ENTRY(entry));

    if (!entry_text)
	return;			/* shouldn't happen */

    nonempty = (entry_text[0] != '\0');

    if (dialog == find_widgets.dialog)
    {
	gtk_widget_set_sensitive(find_widgets.find, nonempty);
    }

    if (dialog == repl_widgets.dialog)
    {
	gtk_widget_set_sensitive(repl_widgets.find, nonempty);
	gtk_widget_set_sensitive(repl_widgets.replace, nonempty);
	gtk_widget_set_sensitive(repl_widgets.all, nonempty);
    }
}

/*
 * ":helpfind"
 */
/*ARGSUSED*/
    void
ex_helpfind(eap)
    exarg_T	*eap;
{
    /* This will fail when menus are not loaded.  Well, it's only for
     * backwards compatibility anyway. */
    do_cmdline_cmd((char_u *)"emenu ToolBar.FindHelp");
}

#if !defined(HAVE_GTK2) || defined(PROTO) /* Crack crack crack.  Brrrr. */

/*  gui_gtk_position_in_parent
 *
 *  this function causes a child window to be placed within the boundary of
 *  the parent (mainwin) window.
 *
 *  you can specify where the window will be positioned by the third argument
 *  (defined in gui.h):
 *	VW_POS_CENTER		at center of parent window
 *	VW_POS_MOUSE		center of child at mouse position
 *	VW_POS_TOP_CENTER	top of child at top of parent centered
 *				horizontally about the mouse.
 *
 *  NOTE: for this function to act as desired the child window must have a
 *	  window size requested.  this can be accomplished by packing/placing
 *	  child widgets onto a gtk_frame widget rather than the gtk_window
 *	  widget...
 *
 *  brent -- dbv
 */
    static void
gui_gtk_position_in_parent(
	GtkWidget	*parent,
	GtkWidget	*child,
	gui_win_pos_T	where)
{
    GtkRequisition	c_size;
    gint		xPm, yPm;
    gint		xP, yP, wP, hP, pos_x, pos_y;

    /* make sure the child widget is set up then get its size. */
    gtk_widget_size_request(child, &c_size);

    /* get origin and size of parent window */
    gdk_window_get_origin((GdkWindow *)(parent->window), &xP, &yP);
    gdk_window_get_size((GdkWindow *)(parent->window), &wP, &hP);

    if (c_size.width > wP || c_size.height > hP)
    {
	/* doh! maybe the user should consider giving gVim a little more
	 * screen real estate */
	gtk_widget_set_uposition(child , xP + 2 , yP + 2);
	return;
    }

    if (where == VW_POS_MOUSE)
    {
	/* position window at mouse pointer */
	gtk_widget_get_pointer(parent, &xPm, &yPm);
	pos_x = xP + xPm - (c_size.width) / 2;
	pos_y = yP + yPm - (c_size.height) / 2;
    }
    else
    {
	/* set child x origin so it is in center of Vim window */
	pos_x =  xP + (wP - c_size.width) / 2;

	if (where == VW_POS_TOP_CENTER)
	    pos_y = yP + 2;
	else
	    /* where == VW_POS_CENTER */
	    pos_y = yP + (hP - c_size.height) / 2;
    }

    /* now, make sure the window will be inside the Vim window... */
    if (pos_x < xP)
	pos_x = xP + 2;
    if (pos_y < yP)
	pos_y = yP + 2;
    if ((pos_x + c_size.width) > (wP + xP))
	pos_x = xP + wP - c_size.width - 2;
    /* Assume 'guiheadroom' indicates the title bar height... */
    if ((pos_y + c_size.height + p_ghr / 2) > (hP + yP))

    gtk_widget_set_uposition(child, pos_x, pos_y);
}

#endif /* !HAVE_GTK2 */
