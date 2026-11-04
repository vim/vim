/* vi:set ts=8 sts=4 sw=4 et:
 *
 * VIM - Vi IMproved by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */

/*
 * gui_gtk4_adw.c: The buttons of the header bar of the GTK 4 GUI with
 * libadwaita and what they show: the recently used files, the main menu and
 * the properties of the current buffer.  Also the dialogs.
 */

#include "vim.h"

#if defined(FEAT_GUI_GTK) && defined(USE_LIBADWAITA)

#include <gtk/gtk.h>
#include <adwaita.h>
#ifdef FEAT_MENU
# include "gui_gtk4_menu.h"
#endif

/*
 * Return TRUE if a widget of the header bar may change the current buffer or
 * edit another one.  Same restrictions as for dropping a file, see
 * gui_handle_drop() and handle_drop().
 */
    static int
buffer_change_allowed(void)
{
    return !(State & MODE_CMDLINE) && !text_locked() && !curbuf_locked()
                                                            && !updating_screen;
}

// Maximum width of a value in the properties popover, in characters.
#define PROPERTY_MAX_CHARS 45

/*
 * Add a row to the list of properties of the current buffer.  "value" is in
 * 'encoding'.
 */
    static void
add_property_row(GtkWidget *list_box, const char *title, char_u *value)
{
    GtkWidget   *row = gtk_list_box_row_new();
    GtkWidget   *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
    GtkWidget   *title_label = gtk_label_new(title);
    GtkWidget   *value_label;
    char_u      *text = CONVERT_TO_UTF8(value);

    gtk_widget_set_halign(title_label, GTK_ALIGN_START);
    gtk_widget_add_css_class(title_label, "caption");
    gtk_widget_add_css_class(title_label, "dim-label");
    gtk_widget_add_css_class(title_label, "dimmed");
    gtk_box_append(GTK_BOX(box), title_label);

    // An AdwActionRow cannot ellipsize a long path: it keeps the popover from
    // becoming wider than the window.  The tooltip shows all of it.
    value_label = gtk_label_new(text != NULL ? (const char *)text : "");
    gtk_label_set_xalign(GTK_LABEL(value_label), 0.0);
    gtk_label_set_ellipsize(GTK_LABEL(value_label), PANGO_ELLIPSIZE_MIDDLE);
    gtk_label_set_max_width_chars(GTK_LABEL(value_label), PROPERTY_MAX_CHARS);
    gtk_label_set_selectable(GTK_LABEL(value_label), TRUE);
    if (text != NULL)
        gtk_widget_set_tooltip_text(value_label, (const char *)text);
    CONVERT_TO_UTF8_FREE(text);
    gtk_box_append(GTK_BOX(box), value_label);

    gtk_widget_set_margin_start(box, 12);
    gtk_widget_set_margin_end(box, 12);
    gtk_widget_set_margin_top(box, 8);
    gtk_widget_set_margin_bottom(box, 8);
    gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), box);
    gtk_list_box_row_set_activatable(GTK_LIST_BOX_ROW(row), FALSE);
    gtk_list_box_append(GTK_LIST_BOX(list_box), row);
}

// The widgets of the properties popover and the buffer they are for.
typedef struct
{
    bufref_T    buf;
    GtkWidget   *popover;
    GtkWidget   *filetype;
    GtkWidget   *encoding;
    GtkWidget   *fileformat;
    GtkWidget   *expandtab;
    GtkWidget   *tabstop;
    GtkWidget   *shiftwidth;
    int         updating;       // TRUE while setting the widgets
} PropertiesState;

static char *fileformat_names[] = {"unix", "dos", "mac"};

/*
 * Select the item "value" of combo row "row", append it when it is not there.
 */
    static void
combo_row_select(GtkWidget *row, const char *value)
{
    GtkStringList   *list = GTK_STRING_LIST(
                            adw_combo_row_get_model(ADW_COMBO_ROW(row)));
    guint           n = g_list_model_get_n_items(G_LIST_MODEL(list));
    guint           i;

    for (i = 0; i < n; i++)
        if (STRCMP(gtk_string_list_get_string(list, i), value) == 0)
            break;
    if (i == n)
        gtk_string_list_append(list, value);
    adw_combo_row_set_selected(ADW_COMBO_ROW(row), i);
}

/*
 * Set the widgets of the properties popover from the options of the buffer.
 */
    static void
properties_update(PropertiesState *state)
{
    buf_T *buf = state->buf.br_buf;

    state->updating = TRUE;
    // The first item is "None", for an empty 'filetype'.
    if (*buf->b_p_ft == NUL)
        adw_combo_row_set_selected(ADW_COMBO_ROW(state->filetype), 0);
    else
        combo_row_select(state->filetype, (const char *)buf->b_p_ft);
    combo_row_select(state->encoding,
            (const char *)(*buf->b_p_fenc != NUL ? buf->b_p_fenc : p_enc));
    adw_combo_row_set_selected(ADW_COMBO_ROW(state->fileformat),
            get_fileformat(buf) == EOL_DOS ? 1
                                    : get_fileformat(buf) == EOL_MAC ? 2 : 0);
    adw_switch_row_set_active(ADW_SWITCH_ROW(state->expandtab), buf->b_p_et);
    adw_spin_row_set_value(ADW_SPIN_ROW(state->tabstop), (double)buf->b_p_ts);
    adw_spin_row_set_value(ADW_SPIN_ROW(state->shiftwidth),
                                                (double)get_sw_value(buf));
    state->updating = FALSE;
}

// An option to set from the properties popover.
typedef struct
{
    PropertiesState *state;
    char            *name;
    long            number;
    char_u          *string;
} PropertiesOption;

/*
 * Set the option "data" for the buffer of the properties popover and update
 * the screen.  Called again later while Vim is busy.
 */
    static gboolean
properties_set_option_cb(gpointer data)
{
    static int          busy = FALSE;
    PropertiesOption    *opt = data;
    PropertiesState     *state = opt->state;

    // Vim handles GTK events while redrawing or executing scripts, e.g. for
    // setting another option.  Wait until nothing is pending, like for the
    // SafeState autocommand.
    if (busy || !get_was_safe_state() || updating_screen || text_locked()
                                                    || (State & MODE_CMDLINE))
        return G_SOURCE_CONTINUE;

    if (!bufref_valid(&state->buf) || state->buf.br_buf != curbuf
                                                        || curbuf_locked())
        vim_beep(BO_ERROR);
    else
    {
        busy = TRUE;
        set_option_value_give_err((char_u *)opt->name, opt->number,
                                                    opt->string, OPT_LOCAL);
        // Setting 'filetype' may change the other options.
        properties_update(state);

        redraw_after_callback(TRUE, FALSE);
        busy = FALSE;
    }

    g_object_unref(state->popover);
    vim_free(opt->string);
    g_free(opt);
    return G_SOURCE_REMOVE;
}

/*
 * Set local option "name" of the buffer of the properties popover to "number"
 * or "string".  This is done after GTK handled the event: setting 'filetype'
 * executes scripts, which may handle GTK events, e.g. to check for CTRL-C, and
 * then a list of a combo row would not be closed.  Use a timeout, an idle
 * callback would make Vim busy loop while it handles GTK events.
 */
    static void
properties_set_option(
        PropertiesState *state,
        char            *name,
        long            number,
        char_u          *string)
{
    PropertiesOption *opt;

    if (state->updating)
        return;

    opt = g_new0(PropertiesOption, 1);
    opt->state = state;
    opt->name = name;
    opt->number = number;
    opt->string = string != NULL ? vim_strsave(string) : NULL;
    // The state is freed with the popover.
    g_object_ref(state->popover);
    g_timeout_add(10, properties_set_option_cb, opt);
}

/*
 * Return the selected string of combo row "row".
 */
    static char_u *
combo_row_get_string(GtkWidget *row)
{
    GObject *item = adw_combo_row_get_selected_item(ADW_COMBO_ROW(row));

    if (item == NULL)
        return (char_u *)"";
    return (char_u *)gtk_string_object_get_string(GTK_STRING_OBJECT(item));
}

    static void
properties_filetype_cb(GObject *row, GParamSpec *pspec UNUSED,
                                                    PropertiesState *state)
{
    properties_set_option(state, "filetype", 0L,
            adw_combo_row_get_selected(ADW_COMBO_ROW(row)) == 0
                        ? (char_u *)"" : combo_row_get_string(GTK_WIDGET(row)));
}

    static void
properties_encoding_cb(GObject *row, GParamSpec *pspec UNUSED,
                                                    PropertiesState *state)
{
    properties_set_option(state, "fileencoding", 0L,
                                        combo_row_get_string(GTK_WIDGET(row)));
}

    static void
properties_fileformat_cb(GObject *row, GParamSpec *pspec UNUSED,
                                                    PropertiesState *state)
{
    guint i = adw_combo_row_get_selected(ADW_COMBO_ROW(row));

    if (i < ARRAY_LENGTH(fileformat_names))
        properties_set_option(state, "fileformat", 0L,
                                            (char_u *)fileformat_names[i]);
}

    static void
properties_expandtab_cb(GObject *row, GParamSpec *pspec UNUSED,
                                                    PropertiesState *state)
{
    properties_set_option(state, "expandtab",
                (long)adw_switch_row_get_active(ADW_SWITCH_ROW(row)), NULL);
}

    static void
properties_tabstop_cb(GObject *row, GParamSpec *pspec UNUSED,
                                                    PropertiesState *state)
{
    properties_set_option(state, "tabstop",
                (long)adw_spin_row_get_value(ADW_SPIN_ROW(row)), NULL);
}

    static void
properties_shiftwidth_cb(GObject *row, GParamSpec *pspec UNUSED,
                                                    PropertiesState *state)
{
    properties_set_option(state, "shiftwidth",
                (long)adw_spin_row_get_value(ADW_SPIN_ROW(row)), NULL);
}

    static void
properties_pressed_cb(
        GtkGesture  *gesture,
        int         n_press UNUSED,
        double      x UNUSED,
        double      y UNUSED,
        GtkWidget   *popover)
{
    if (gui_gtk_claim_window_press(gesture, popover))
        gtk_popover_popdown(GTK_POPOVER(popover));
}

/*
 * Create a combo row with title "title" and the strings "items", with a
 * search entry when "search" is TRUE.
 */
    static GtkWidget *
new_combo_row(const char *title, const char * const *items, int search)
{
    GtkWidget           *row = adw_combo_row_new();
    GtkStringList       *list = gtk_string_list_new(items);

    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), title);
    adw_combo_row_set_model(ADW_COMBO_ROW(row), G_LIST_MODEL(list));
    g_object_unref(list);
    if (search)
    {
        // Searching needs to know the string of the items.
        GtkExpression *expression = gtk_property_expression_new(
                                    GTK_TYPE_STRING_OBJECT, NULL, "string");

        adw_combo_row_set_expression(ADW_COMBO_ROW(row), expression);
        gtk_expression_unref(expression);
        adw_combo_row_set_enable_search(ADW_COMBO_ROW(row), TRUE);
    }
    return row;
}

/*
 * Create the popover of the properties button of the header bar, each time it
 * is opened, with the properties of the current buffer, which can be changed,
 * like GNOME applications.
 */
    static void
properties_create_popup_cb(GtkMenuButton *button, gpointer data UNUSED)
{
    static const char * const encodings[] = {"utf-8", "latin1", "cp1252",
        "iso-8859-15", "utf-16", "utf-16le", "ucs-bom", "cp1251", "koi8-r",
        "sjis", "euc-jp", "gb18030", "big5", "euc-kr", NULL};
    const char * const fileformats[] = {_("Unix/Linux (LF)"),
        _("Windows (CR+LF)"), _("Classic Mac (CR)"), NULL};
    const char * const no_filetype[] = {_("None"), NULL};
    PropertiesState *state;
    GtkWidget   *popover;
    GtkWidget   *list_box;
    char_u      *fname = curbuf->b_ffname;
    char_u      *value;
    char_u      buf[NUMBUFLEN];

    state = g_new0(PropertiesState, 1);
    set_bufref(&state->buf, curbuf);

    list_box = gtk_list_box_new();
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(list_box),
            GTK_SELECTION_NONE);
    gtk_widget_add_css_class(list_box, "boxed-list");
    gtk_widget_set_size_request(list_box, 320, -1);

    if (fname == NULL)
        add_property_row(list_box, _("Name"), buf_spname(curbuf));
    else
    {
        add_property_row(list_box, _("Name"), gettail(fname));
        value = vim_strsave(fname);
        if (value != NULL)
        {
            *gettail_sep(value) = NUL;
            home_replace(NULL, value, NameBuff, MAXPATHL, TRUE);
            vim_free(value);
            add_property_row(list_box, _("Location"), NameBuff);
        }
    }

    // All the file types Vim knows about, like ":setfiletype <Tab>".
    state->filetype = new_combo_row(_("File Type"), no_filetype, TRUE);
    gtk_widget_set_tooltip_text(state->filetype,
            _("Selects the syntax highlighting and indentation settings "
                "for this buffer."));
# ifdef FEAT_EVAL
    {
        // eval_expr() changes the expression while evaluating it.
        char_u      *expr = vim_strsave(
                                (char_u *)"getcompletion('', 'filetype')");
        typval_T    *tv = expr != NULL ? eval_expr(expr, NULL) : NULL;

        vim_free(expr);
        if (tv != NULL && tv->v_type == VAR_LIST && tv->vval.v_list != NULL)
        {
            GtkStringList   *list = GTK_STRING_LIST(adw_combo_row_get_model(
                                            ADW_COMBO_ROW(state->filetype)));
            listitem_T      *li;

            FOR_ALL_LIST_ITEMS(tv->vval.v_list, li)
                if (li->li_tv.v_type == VAR_STRING
                                            && li->li_tv.vval.v_string != NULL)
                    gtk_string_list_append(list,
                                    (const char *)li->li_tv.vval.v_string);
        }
        free_tv(tv);
    }
# endif
    gtk_list_box_append(GTK_LIST_BOX(list_box), state->filetype);

    state->encoding = new_combo_row(_("Encoding"), encodings, FALSE);
    gtk_widget_set_tooltip_text(state->encoding,
            _("Character encoding used when writing the file. "
                "The text is not read again."));
    gtk_list_box_append(GTK_LIST_BOX(list_box), state->encoding);

    state->fileformat = new_combo_row(_("Line Endings"), fileformats, FALSE);
    gtk_widget_set_tooltip_text(state->fileformat,
            _("Line endings used when writing the file. "
                "The text is not read again."));
    gtk_list_box_append(GTK_LIST_BOX(list_box), state->fileformat);

    state->expandtab = adw_switch_row_new();
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(state->expandtab),
                                                        _("Insert Spaces"));
    gtk_widget_set_tooltip_text(state->expandtab,
            _("Insert spaces instead of Tab characters in this buffer."));
    gtk_list_box_append(GTK_LIST_BOX(list_box), state->expandtab);

    state->tabstop = adw_spin_row_new_with_range(1, 32, 1);
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(state->tabstop),
                                                            _("Tab Width"));
    gtk_widget_set_tooltip_text(state->tabstop,
            _("Display width of Tab characters in this buffer."));
    gtk_list_box_append(GTK_LIST_BOX(list_box), state->tabstop);

    state->shiftwidth = adw_spin_row_new_with_range(1, 32, 1);
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(state->shiftwidth),
                                                            _("Indent Width"));
    gtk_widget_set_tooltip_text(state->shiftwidth,
            _("Indentation width used by >> and automatic indentation "
                "in this buffer."));
    gtk_list_box_append(GTK_LIST_BOX(list_box), state->shiftwidth);

    vim_snprintf((char *)buf, sizeof(buf), "%ld",
                                    (long)curbuf->b_ml.ml_line_count);
    add_property_row(list_box, _("Lines"), buf);

    properties_update(state);
    g_signal_connect(state->filetype, "notify::selected",
            G_CALLBACK(properties_filetype_cb), state);
    g_signal_connect(state->encoding, "notify::selected",
            G_CALLBACK(properties_encoding_cb), state);
    g_signal_connect(state->fileformat, "notify::selected",
            G_CALLBACK(properties_fileformat_cb), state);
    g_signal_connect(state->expandtab, "notify::active",
            G_CALLBACK(properties_expandtab_cb), state);
    g_signal_connect(state->tabstop, "notify::value",
            G_CALLBACK(properties_tabstop_cb), state);
    g_signal_connect(state->shiftwidth, "notify::value",
            G_CALLBACK(properties_shiftwidth_cb), state);

    popover = gtk_popover_new();
    state->popover = popover;
    gtk_popover_set_child(GTK_POPOVER(popover), list_box);
    gui_gtk_popover_add_press(popover, G_CALLBACK(properties_pressed_cb));
    // Free the state together with the popover.
    g_object_set_data_full(G_OBJECT(popover), "state", state, g_free);
    gtk_menu_button_set_popover(button, popover);
}

#ifdef FEAT_MENU
/*
 * Create the popover of the main menu button of the header bar from the
 * "]HeaderBar" menu, followed by the whole menubar, each time it is opened, so
 * that it is up to date.
 */
    static void
main_menu_create_popup_cb(GtkMenuButton *button, gpointer data UNUSED)
{
    vimmenu_T   *menu;
    GtkWidget   *popover;

    for (menu = root_menu; menu != NULL; menu = menu->next)
        if (STRCMP(menu->name, "]HeaderBar") == 0)
            break;

    if (menu != NULL && menu->submenu_id != NULL && menu->children != NULL)
    {
        popover = vim_menu_copy(VIM_MENU(menu->submenu_id));
        if (gui.menubar != NULL)
            vim_menu_bar_append_to_menu(VIM_MENU_BAR(gui.menubar),
                                                        VIM_MENU(popover));
    }
    else if (gui.menubar != NULL)
        popover = vim_menu_bar_to_menu(VIM_MENU_BAR(gui.menubar));
    else
        return;

    gtk_menu_button_set_popover(button, popover);
}
#endif // FEAT_MENU

#ifdef FEAT_GUI_TABLINE
/*
 * A tab page button of the header bar was clicked, "udata" is the tab pages
 * line menu event to send.
 */
    static void
header_tab_button_clicked_cb(GtkWidget *button UNUSED, gpointer udata)
{
    send_tabline_menu_event(0, GPOINTER_TO_INT(udata));
}

# define RECENT_FILES_MAX 20

/*
 * Add a row for file "fname" to the list of recently used files: the name of
 * the file and, dimmed below it, its directory.  Nothing is done when the file
 * does not exist, is already in the list or the list is full.
 */
    static void
add_recent_file_row(GtkWidget *list_box, char_u *fname)
{
    GtkListBoxRow   *row;
    GtkWidget       *new_row;
    GtkWidget       *box;
    GtkWidget       *label;
    char_u          *dir;
    char_u          *text;
    int             i;

    if (mch_getperm(fname) < 0 || mch_isdir(fname))
        return;
    for (i = 0; (row = gtk_list_box_get_row_at_index(
                                    GTK_LIST_BOX(list_box), i)) != NULL; ++i)
    {
        char_u *path = g_object_get_data(G_OBJECT(row), "path");

        if (path != NULL && fnamecmp(path, fname) == 0)
            return;
    }
    if (i >= RECENT_FILES_MAX)
        return;

    box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);

    // Without a maximum width an ellipsized label only asks for the width of
    // the ellipsis.
    text = CONVERT_TO_UTF8(gettail(fname));
    label = gtk_label_new((const char *)text);
    CONVERT_TO_UTF8_FREE(text);
    gtk_label_set_xalign(GTK_LABEL(label), 0);
    gtk_label_set_ellipsize(GTK_LABEL(label), PANGO_ELLIPSIZE_MIDDLE);
    gtk_label_set_max_width_chars(GTK_LABEL(label), 50);
    gtk_box_append(GTK_BOX(box), label);

    dir = vim_strsave(fname);
    if (dir != NULL)
    {
        *gettail_sep(dir) = NUL;
        home_replace(NULL, dir, NameBuff, MAXPATHL, TRUE);
        vim_free(dir);
        text = CONVERT_TO_UTF8(NameBuff);
        label = gtk_label_new((const char *)text);
        CONVERT_TO_UTF8_FREE(text);
        gtk_label_set_xalign(GTK_LABEL(label), 0);
        gtk_label_set_ellipsize(GTK_LABEL(label), PANGO_ELLIPSIZE_START);
        gtk_label_set_max_width_chars(GTK_LABEL(label), 50);
        gtk_widget_add_css_class(label, "caption");
        gtk_widget_add_css_class(label, "dim-label");
        gtk_widget_add_css_class(label, "dimmed");
        gtk_box_append(GTK_BOX(box), label);
    }

    new_row = gtk_list_box_row_new();
    gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(new_row), box);
    g_object_set_data_full(G_OBJECT(new_row), "path",
            vim_strsave(fname), vim_free);
    text = CONVERT_TO_UTF8(fname);
    g_object_set_data_full(G_OBJECT(new_row), "search",
            g_utf8_casefold((const char *)text, -1), g_free);
    CONVERT_TO_UTF8_FREE(text);
    gtk_list_box_append(GTK_LIST_BOX(list_box), new_row);
}

# ifdef FEAT_VIMINFO
/*
 * Compare two buffers for qsort(), the most recently used first.
 */
    static int
recent_buf_compare(const void *s1, const void *s2)
{
    buf_T       *buf1 = *(buf_T **)s1;
    buf_T       *buf2 = *(buf_T **)s2;

    if (buf1->b_last_used == buf2->b_last_used)
        return 0;
    return buf1->b_last_used > buf2->b_last_used ? -1 : 1;
}
# endif

/*
 * The popover with the recently used files is shown: fill the list with the
 * files that were closed in this session, the most recently used first, and
 * then the files of "v:oldfiles", like ":oldfiles".  Like in GNOME
 * applications, a file is added to the list when it is closed, not when it is
 * opened.
 */
    static void
recent_popover_show_cb(GtkWidget *popover UNUSED, GtkWidget *list_box)
{
    GtkListBoxRow   *row;

    gtk_editable_set_text(
            GTK_EDITABLE(g_object_get_data(G_OBJECT(list_box), "search")), "");

    while ((row = gtk_list_box_get_row_at_index(
                                    GTK_LIST_BOX(list_box), 0)) != NULL)
        gtk_list_box_remove(GTK_LIST_BOX(list_box), GTK_WIDGET(row));

# ifdef FEAT_VIMINFO
    {
        buf_T   *buf;
        buf_T   **bufs;
        int     count = 0;
        int     i;

        FOR_ALL_BUFFERS(buf)
            ++count;
        bufs = ALLOC_MULT(buf_T *, count);
        if (bufs != NULL)
        {
            // Buffers that were displayed in a window but no longer are,
            // including deleted buffers.
            count = 0;
            FOR_ALL_BUFFERS(buf)
                if (buf->b_ffname != NULL && buf->b_nwindows == 0
                        && buf->b_last_used != 0 && !buf->b_help
                        && bt_normal(buf))
                    bufs[count++] = buf;
            qsort(bufs, (size_t)count, sizeof(buf_T *), recent_buf_compare);
            for (i = 0; i < count; ++i)
                add_recent_file_row(list_box, bufs[i]->b_ffname);
            vim_free(bufs);
        }
    }
# endif

# ifdef FEAT_EVAL
    {
        list_T      *l = get_vim_var_list(VV_OLDFILES);
        listitem_T  *li;
        char_u      *fname;

        if (l != NULL)
            FOR_ALL_LIST_ITEMS(l, li)
            {
                if (li->li_tv.v_type != VAR_STRING
                                        || li->li_tv.vval.v_string == NULL)
                    continue;
                fname = expand_env_save(li->li_tv.vval.v_string);
                if (fname != NULL)
                {
                    add_recent_file_row(list_box, fname);
                    vim_free(fname);
                }
            }
    }
# endif
}

/*
 * Only show the rows of the files whose name contains the text of the search
 * entry "data", ignoring case.
 */
    static gboolean
recent_filter_func(GtkListBoxRow *row, gpointer data)
{
    const char  *text = gtk_editable_get_text(GTK_EDITABLE(data));
    const char  *path_fold = g_object_get_data(G_OBJECT(row), "search");
    char        *text_fold;
    gboolean    found;

    if (text == NULL || *text == NUL)
        return TRUE;
    if (path_fold == NULL)
        return FALSE;

    text_fold = g_utf8_casefold(text, -1);
    found = strstr(path_fold, text_fold) != NULL;
    g_free(text_fold);
    return found;
}

    static void
recent_search_changed_cb(GtkSearchEntry *search UNUSED, GtkWidget *list_box)
{
    gtk_list_box_invalidate_filter(GTK_LIST_BOX(list_box));
}

/*
 * The button to browse for a file in the popover was clicked.
 */
    static void
recent_open_clicked_cb(GtkButton *button UNUSED, GtkWidget *popover)
{
    gtk_popover_popdown(GTK_POPOVER(popover));
    send_tabline_menu_event(0, TABLINE_MENU_OPEN);
}

    static void
recent_popover_closed_cb(GtkPopover *popover UNUSED, gpointer data UNUSED)
{
    if (gui.drawarea != NULL)
        gtk_widget_grab_focus(gui.drawarea);
}

/*
 * A recently used file was selected: edit it in a new tab page, or go to the
 * window where it is already edited, like ":tab drop".
 */
    static void
recent_row_activated_cb(
        GtkListBox      *list_box UNUSED,
        GtkListBoxRow   *row,
        GtkWidget       *popover)
{
    char_u      *path = g_object_get_data(G_OBJECT(row), "path");
    char_u      *fname;
    char_u      *cmd;

    if (path == NULL)
        return;

    if (!buffer_change_allowed())
    {
        vim_beep(BO_ERROR);
        return;
    }

    fname = vim_strsave_fnameescape(path, VSE_NONE);
    if (fname == NULL)
        return;
    cmd = concat_str((char_u *)"tab drop ", fname);
    vim_free(fname);
    if (cmd == NULL)
        return;

    gtk_popover_popdown(GTK_POPOVER(popover));
    do_cmdline_cmd(cmd);
    vim_free(cmd);

    // Update the screen display, like after dropping a file.
    update_screen(UPD_NOT_VALID);
# ifdef FEAT_MENU
    gui_update_menus(0);
# endif
    maketitle();
    setcursor();
    out_flush_cursor(FALSE, FALSE);
}
#endif // FEAT_GUI_TABLINE

#ifdef FEAT_GUI_TABLINE
/*
 * Create the list of the recently used files, which is empty for now.
 */
    static GtkWidget *
create_recent_list_box(void)
{
    GtkWidget *list_box;
    GtkWidget *placeholder;
    GtkWidget *image;
    GtkWidget *label;

    list_box = gtk_list_box_new();
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(list_box),
            GTK_SELECTION_NONE);
    gtk_widget_add_css_class(list_box, "navigation-sidebar");

    placeholder = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_widget_set_margin_top(placeholder, 18);
    gtk_widget_set_margin_bottom(placeholder, 18);
    image = gtk_image_new_from_icon_name(
            "document-open-recent-symbolic");
    gtk_image_set_pixel_size(GTK_IMAGE(image), 32);
    gtk_widget_add_css_class(image, "dim-label");
    gtk_widget_add_css_class(image, "dimmed");
    gtk_box_append(GTK_BOX(placeholder), image);
    label = gtk_label_new(_("No Recent Files"));
    gtk_widget_add_css_class(label, "dim-label");
    gtk_widget_add_css_class(label, "dimmed");
    gtk_box_append(GTK_BOX(placeholder), label);
    gtk_list_box_set_placeholder(GTK_LIST_BOX(list_box), placeholder);

    return list_box;
}

/*
 * Create the top of the popover of the "Open" button: a search entry to filter
 * the files of "list_box" and a button to browse for a file.
 */
    static GtkWidget *
create_recent_search_box(GtkWidget *list_box, GtkWidget *popover)
{
    GtkWidget *search;
    GtkWidget *button;
    GtkWidget *top;

    search = gtk_search_entry_new();
    gtk_widget_set_hexpand(search, TRUE);
    gtk_search_entry_set_placeholder_text(GTK_SEARCH_ENTRY(search),
            _("Search Files"));
    gtk_list_box_set_filter_func(GTK_LIST_BOX(list_box),
            recent_filter_func, search, NULL);
    g_signal_connect(search, "search-changed",
            G_CALLBACK(recent_search_changed_cb), list_box);
    g_object_set_data(G_OBJECT(list_box), "search", search);

    button = gtk_button_new_from_icon_name("document-open-symbolic");
    gtk_widget_set_tooltip_text(button, _("Open Tab..."));
    g_signal_connect(button, "clicked",
            G_CALLBACK(recent_open_clicked_cb), popover);

    top = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_widget_set_margin_start(top, 6);
    gtk_widget_set_margin_end(top, 6);
    gtk_widget_set_margin_top(top, 6);
    gtk_widget_set_margin_bottom(top, 6);
    gtk_box_append(GTK_BOX(top), search);
    gtk_box_append(GTK_BOX(top), button);

    return top;
}

/*
 * Create the popover of the "Open" button, with the recently used files.
 */
    static GtkWidget *
create_recent_popover(void)
{
    GtkWidget *popover;
    GtkWidget *list_box;
    GtkWidget *scrolled;
    GtkWidget *box;

    list_box = create_recent_list_box();

    scrolled = gtk_scrolled_window_new();
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
            GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_propagate_natural_height(
            GTK_SCROLLED_WINDOW(scrolled), TRUE);
    gtk_scrolled_window_set_propagate_natural_width(
            GTK_SCROLLED_WINDOW(scrolled), TRUE);
    gtk_scrolled_window_set_max_content_height(
            GTK_SCROLLED_WINDOW(scrolled), 400);
    gtk_scrolled_window_set_min_content_width(
            GTK_SCROLLED_WINDOW(scrolled), 300);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled),
            list_box);

    popover = gtk_popover_new();

    box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_append(GTK_BOX(box), create_recent_search_box(list_box, popover));
    gtk_box_append(GTK_BOX(box),
            gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));
    gtk_box_append(GTK_BOX(box), scrolled);

    gtk_popover_set_child(GTK_POPOVER(popover), box);
    g_signal_connect(popover, "show",
            G_CALLBACK(recent_popover_show_cb), list_box);
    g_signal_connect(popover, "closed",
            G_CALLBACK(recent_popover_closed_cb), NULL);
    g_signal_connect(list_box, "row-activated",
            G_CALLBACK(recent_row_activated_cb), popover);

    return popover;
}

/*
 * Like in GNOME applications, an "Open" button that shows the recently used
 * files.  It must not take the focus away from the text area.
 */
    static void
header_bar_add_open_button(GtkWidget *header_bar)
{
    GtkWidget *button = gtk_menu_button_new();

    gtk_menu_button_set_label(GTK_MENU_BUTTON(button), _("Open"));
    gtk_menu_button_set_popover(GTK_MENU_BUTTON(button),
                                                    create_recent_popover());
    gtk_widget_set_tooltip_text(button, _("Recent Files"));
    gtk_widget_set_focus_on_click(button, FALSE);
    adw_header_bar_pack_start(ADW_HEADER_BAR(header_bar), button);
}

/*
 * A button to edit a new file in a new tab page.  It must not take the focus
 * away from the text area.
 */
    static void
header_bar_add_new_tab_button(GtkWidget *header_bar)
{
    GtkWidget *button = gtk_button_new_from_icon_name("tab-new-symbolic");

    gtk_widget_set_tooltip_text(button, _("New Tab"));
    gtk_widget_set_focus_on_click(button, FALSE);
    g_signal_connect(button, "clicked",
            G_CALLBACK(header_tab_button_clicked_cb),
            GINT_TO_POINTER(TABLINE_MENU_NEW));
    adw_header_bar_pack_start(ADW_HEADER_BAR(header_bar), button);
}
#endif // FEAT_GUI_TABLINE

#ifdef FEAT_MENU
/*
 * The main menu button shows the "]HeaderBar" menu.  It is not made the
 * primary menu, GTK would then take <F10> away from Vim.
 */
    static void
header_bar_add_menu_button(GtkWidget *header_bar)
{
    GtkWidget *button = gtk_menu_button_new();

    gtk_menu_button_set_icon_name(GTK_MENU_BUTTON(button),
            "open-menu-symbolic");
    gtk_widget_set_tooltip_text(button, _("Main Menu"));
    gtk_widget_set_focus_on_click(button, FALSE);
    gtk_menu_button_set_create_popup_func(GTK_MENU_BUTTON(button),
            main_menu_create_popup_cb, NULL, NULL);
    adw_header_bar_pack_end(ADW_HEADER_BAR(header_bar), button);
}
#endif // FEAT_MENU

/*
 * A button that shows the properties of the current buffer.
 */
    static void
header_bar_add_info_button(GtkWidget *header_bar)
{
    GtkWidget *button = gtk_menu_button_new();

    gtk_menu_button_set_icon_name(GTK_MENU_BUTTON(button),
            "info-outline-symbolic");
    gtk_widget_set_tooltip_text(button, _("Document Properties"));
    gtk_widget_set_focus_on_click(button, FALSE);
    gtk_menu_button_set_create_popup_func(GTK_MENU_BUTTON(button),
            properties_create_popup_cb, NULL, NULL);
    adw_header_bar_pack_end(ADW_HEADER_BAR(header_bar), button);
}

/*
 * Add the buttons to the header bar "header_bar".
 */
    void
gui_adw_fill_header_bar(GtkWidget *header_bar)
{
#ifdef FEAT_GUI_TABLINE
    header_bar_add_open_button(header_bar);
    header_bar_add_new_tab_button(header_bar);
#endif
#ifdef FEAT_MENU
    header_bar_add_menu_button(header_bar);
#endif
    // Left of the main menu button.
    header_bar_add_info_button(header_bar);
}

typedef struct
{
    AdwDialog   *dialog;
    GtkEventController *key_controller; // NULL when not used
    gunichar    *mnemonics;     // lower case mnemonic of each button or 0
    int         n_buttons;
    int         response;       // index of the button, -1 if there is none
    gboolean    done;
} AlertDialogState;

    static void
alert_dialog_response_cb(
        AdwAlertDialog      *dialog UNUSED,
        const char          *response,
        AlertDialogState    *state)
{
    // The response IDs are "b0", "b1", etc., "close" when closed.
    state->response = response[0] == 'b' ? atoi(response + 1) : -1;
    state->done = TRUE;
}

    static void
alert_dialog_closed_cb(AdwDialog *dialog UNUSED, AlertDialogState *state)
{
    state->done = TRUE;
}

    static void
alert_dialog_entry_map_cb(GtkWidget *entry, gpointer data UNUSED)
{
    gtk_widget_grab_focus(entry);
}

/*
 * Like in the other dialogs, allow using the mnemonics of the buttons without
 * the <Alt> key.
 */
    static gboolean
alert_dialog_key_pressed_cb(
        GtkEventControllerKey   *controller UNUSED,
        guint                   keyval,
        guint                   keycode UNUSED,
        GdkModifierType         mod_state,
        AlertDialogState        *state)
{
    gunichar c;

    if (mod_state & gtk_accelerator_get_default_mod_mask())
        return FALSE;
    c = g_unichar_tolower(gdk_keyval_to_unicode(keyval));
    if (c == 0)
        return FALSE;
    for (int i = 0; i < state->n_buttons; i++)
        if (state->mnemonics[i] == c)
        {
            state->response = i;
            state->done = TRUE;
            adw_dialog_force_close(state->dialog);
            return TRUE;
        }
    return FALSE;
}

/*
 * Show the dialog of "state" and wait until a response is chosen or it is
 * closed.
 */
    static void
alert_dialog_wait(AlertDialogState *state)
{
    g_signal_connect(state->dialog, "response",
            G_CALLBACK(alert_dialog_response_cb), state);
    g_signal_connect(state->dialog, "closed",
            G_CALLBACK(alert_dialog_closed_cb), state);

    // When the main window is not shown yet, e.g. while sourcing the vimrc
    // file, the dialog would be invisible in it.  Without a parent the dialog
    // gets its own window.
    adw_dialog_present(state->dialog,
            gtk_widget_get_mapped(gui.mainwin) ? gui.mainwin : NULL);

    while (!state->done)
        g_main_context_iteration(NULL, TRUE);

    // The callbacks use "state", which is about to go away.
    g_signal_handlers_disconnect_by_data(state->dialog, state);
    if (state->key_controller != NULL)
        g_signal_handlers_disconnect_by_data(state->key_controller, state);
}

/*
 * Show a dialog like GNOME applications, with an AdwAlertDialog.  The
 * arguments and the return value are like for gui_mch_dialog().
 */
    static int
alert_dialog_run(
        char_u  *title,
        char_u  *message,
        char_u  *buttons,
        int     def_but,
        char_u  *textfield)
{
    AlertDialogState    state;
    AdwAlertDialog      *alert;
    GtkWidget           *entry = NULL;
    char_u              *utf8_title;
    char_u              *utf8_message;
    char                *tofree = NULL;

    CLEAR_FIELD(state);
    state.response = -1;

    // A short message, like "Save changes to "file"?", is the heading.
    // Otherwise the title is the heading and the message is the body.
    utf8_title = CONVERT_TO_UTF8(title);
    utf8_message = CONVERT_TO_UTF8(message);
    if (utf8_message != NULL && vim_strchr(utf8_message, '\n') == NULL
                                            && STRLEN(utf8_message) <= 80)
        state.dialog = adw_alert_dialog_new((const char *)utf8_message, NULL);
    else
        state.dialog = adw_alert_dialog_new((const char *)utf8_title,
                                                (const char *)utf8_message);
    CONVERT_TO_UTF8_FREE(utf8_title);
    CONVERT_TO_UTF8_FREE(utf8_message);
    g_object_ref_sink(state.dialog);
    alert = ADW_ALERT_DIALOG(state.dialog);

    if (buttons != NULL)
    {
        char    **buttons_arr; // Note that array is allocated, not strings

        buttons_arr = split_button_string(buttons, &state.n_buttons, &tofree);
        state.mnemonics = g_new0(gunichar, state.n_buttons);

        for (int i = 0; i < state.n_buttons; i++)
        {
            char_u  *but_label;
            char    *p;
            char    id[NUMBUFLEN];

            but_label = CONVERT_TO_UTF8((char_u *)buttons_arr[i]);
            if (but_label == NULL)
                continue;

            vim_snprintf(id, sizeof(id), "b%d", i);
            adw_alert_dialog_add_response(alert, id, (const char *)but_label);
            if (i == def_but - 1)
            {
                adw_alert_dialog_set_default_response(alert, id);
                adw_alert_dialog_set_response_appearance(alert, id,
                        ADW_RESPONSE_SUGGESTED);
            }

            p = (char *)vim_strchr(but_label, '_');
            if (p != NULL && p[1] != NUL)
                state.mnemonics[i] = g_unichar_tolower(g_utf8_get_char(p + 1));
            CONVERT_TO_UTF8_FREE(but_label);
        }
        g_free(buttons_arr);
    }
    // <Esc> closes the dialog without choosing a button.
    adw_alert_dialog_set_close_response(alert, "close");

# if ADW_CHECK_VERSION(1, 6, 0)
    // Keep the buttons side by side, unless 'guioptions' has the 'v' flag.
    adw_alert_dialog_set_prefer_wide_layout(alert,
                                    vim_strchr(p_go, GO_VERTICAL) == NULL);
# endif

    if (textfield != NULL)
    {
        // Add text entry so user can enter text
        char_u *utf8_text = CONVERT_TO_UTF8(textfield);

        entry = gtk_entry_new();
        gtk_editable_set_text(GTK_EDITABLE(entry),
                utf8_text != NULL ? (const char *)utf8_text : "");
        CONVERT_TO_UTF8_FREE(utf8_text);
        gtk_entry_set_activates_default(GTK_ENTRY(entry), TRUE);
        // Give the focus to the entry when the dialog is shown.
        g_signal_connect(entry, "map",
                G_CALLBACK(alert_dialog_entry_map_cb), NULL);
        adw_alert_dialog_set_extra_child(alert, entry);
    }
    else
    {
        state.key_controller = gtk_event_controller_key_new();
        gtk_event_controller_set_propagation_phase(state.key_controller,
                GTK_PHASE_CAPTURE);
        g_signal_connect(state.key_controller, "key-pressed",
                G_CALLBACK(alert_dialog_key_pressed_cb), &state);
        gtk_widget_add_controller(GTK_WIDGET(state.dialog),
                                                        state.key_controller);
    }

    alert_dialog_wait(&state);

    if (state.response >= 0 && entry != NULL)
    {
        // Get the text the user entered
        char_u *text;

        text = (char_u *)gtk_editable_get_text(GTK_EDITABLE(entry));
        text = CONVERT_FROM_UTF8(text);
        vim_strncpy(textfield, text, IOSIZE - 1);
        CONVERT_FROM_UTF8_FREE(text);
    }

    g_object_unref(state.dialog);
    g_free(state.mnemonics);
    g_free(tofree);

    // Vim buttons are indexed starting from one.
    return state.response + 1;
}

/*
 * Ask which of the "count" changed buffers "bufs" to write before abandoning
 * them, in one dialog with a check box for each buffer, like GNOME
 * applications.  "save" has the initial state of the check boxes and is set
 * to their state.
 * Returns VIM_YES to write the checked buffers and discard the changes of the
 * other ones, VIM_DISCARDALL to discard all the changes or VIM_CANCEL.
 */
    static int
changed_dialog_run(buf_T **bufs, int *save, int count)
{
    AlertDialogState    state;
    AdwAlertDialog      *alert;
    GtkWidget           *list_box;
    GtkWidget           *scrolled;
    GtkWidget           **checks;
    int                 i;

    CLEAR_FIELD(state);
    state.response = -1;
    state.dialog = adw_alert_dialog_new(_("Save Changes?"),
            _("Changes which are not saved will be permanently lost."));
    g_object_ref_sink(state.dialog);
    alert = ADW_ALERT_DIALOG(state.dialog);

    list_box = gtk_list_box_new();
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(list_box),
            GTK_SELECTION_NONE);
    gtk_widget_add_css_class(list_box, "boxed-list");

    checks = g_new0(GtkWidget *, count);
    for (i = 0; i < count; i++)
    {
        GtkWidget   *row = adw_action_row_new();
        char_u      *fname = bufs[i]->b_ffname;
        char_u      *text;

        adw_preferences_row_set_use_markup(ADW_PREFERENCES_ROW(row), FALSE);
        if (fname == NULL)
        {
            text = CONVERT_TO_UTF8(buf_spname(bufs[i]));
            adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row),
                                                        (const char *)text);
            CONVERT_TO_UTF8_FREE(text);
        }
        else
        {
            char_u *dir;

            text = CONVERT_TO_UTF8(gettail(fname));
            adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row),
                                                        (const char *)text);
            CONVERT_TO_UTF8_FREE(text);

            dir = vim_strsave(fname);
            if (dir != NULL)
            {
                *gettail_sep(dir) = NUL;
                home_replace(NULL, dir, NameBuff, MAXPATHL, TRUE);
                vim_free(dir);
                text = CONVERT_TO_UTF8(NameBuff);
                adw_action_row_set_subtitle(ADW_ACTION_ROW(row),
                                                        (const char *)text);
                CONVERT_TO_UTF8_FREE(text);
            }
        }

        checks[i] = gtk_check_button_new();
        gtk_check_button_set_active(GTK_CHECK_BUTTON(checks[i]), save[i]);
        gtk_widget_set_valign(checks[i], GTK_ALIGN_CENTER);
        adw_action_row_add_prefix(ADW_ACTION_ROW(row), checks[i]);
        adw_action_row_set_activatable_widget(ADW_ACTION_ROW(row), checks[i]);
        gtk_list_box_append(GTK_LIST_BOX(list_box), row);
    }

    scrolled = gtk_scrolled_window_new();
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
            GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_propagate_natural_height(
            GTK_SCROLLED_WINDOW(scrolled), TRUE);
    gtk_scrolled_window_set_max_content_height(
            GTK_SCROLLED_WINDOW(scrolled), 300);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled), list_box);
    adw_alert_dialog_set_extra_child(alert, scrolled);

    adw_alert_dialog_add_response(alert, "b0", _("_Cancel"));
    adw_alert_dialog_add_response(alert, "b1", _("_Discard"));
    adw_alert_dialog_set_response_appearance(alert, "b1",
            ADW_RESPONSE_DESTRUCTIVE);
    adw_alert_dialog_add_response(alert, "b2", _("_Save"));
    adw_alert_dialog_set_response_appearance(alert, "b2",
            ADW_RESPONSE_SUGGESTED);
    adw_alert_dialog_set_default_response(alert, "b2");
    adw_alert_dialog_set_close_response(alert, "b0");
# if ADW_CHECK_VERSION(1, 6, 0)
    adw_alert_dialog_set_prefer_wide_layout(alert, TRUE);
# endif

    alert_dialog_wait(&state);

    for (i = 0; i < count; i++)
        save[i] = gtk_check_button_get_active(GTK_CHECK_BUTTON(checks[i]));
    g_free(checks);
    g_object_unref(state.dialog);

    if (state.response == 2)
        return VIM_YES;
    if (state.response == 1)
        return VIM_DISCARDALL;
    return VIM_CANCEL;
}

/*
 * Return TRUE if "buf" is a changed buffer that can be written together with
 * the other ones, like with "Save All" in dialog_changed().
 */
    static int
changed_buf_can_write(buf_T *buf)
{
    return bufIsChanged(buf)
            && (buf->b_ffname != NULL
# ifdef FEAT_BROWSE
                || (cmdmod.cmod_flags & CMOD_BROWSE)
# endif
               )
            && !bt_dontwrite(buf)
            && !buf->b_p_ro;
}

/*
 * Used instead of dialog_changed() to ask what to do when abandoning changed
 * buffer "buf": ask in one dialog for "buf" and, when "checkall" is TRUE, for
 * all the other changed buffers which ones to write.  The changes of the
 * other ones are discarded.
 */
    void
gui_gtk_dialog_changed(buf_T *buf, int checkall)
{
    buf_T       *buf2;
    buf_T       **bufs = NULL;
    bufref_T    *bufrefs = NULL;
    int         *save = NULL;
    int         count = 1;
    int         ret;
    int         i;
    exarg_T     ea;
    char_u      buff[DIALOG_MSG_SIZE];

    if (checkall)
        FOR_ALL_BUFFERS(buf2)
            if (buf2 != buf && changed_buf_can_write(buf2))
                ++count;

    bufs = ALLOC_MULT(buf_T *, count);
    bufrefs = ALLOC_MULT(bufref_T, count);
    save = ALLOC_MULT(int, count);
    if (bufs == NULL || bufrefs == NULL || save == NULL)
        goto theend;

    // The buffer being abandoned comes first.
    bufs[0] = buf;
    count = 1;
    if (checkall)
        FOR_ALL_BUFFERS(buf2)
            if (buf2 != buf && changed_buf_can_write(buf2))
                bufs[count++] = buf2;
    for (i = 0; i < count; ++i)
    {
        set_bufref(&bufrefs[i], bufs[i]);
        save[i] = TRUE;
    }

    // --gui-dialog-file: write the text to a file and use the default button,
    // like do_dialog().
    dialog_msg(buff, _("Save changes to \"%s\"?"), buf->b_fname);
    if (gui_dialog_log((char_u *)_("Question"), buff))
        ret = VIM_YES;
    else
        ret = changed_dialog_run(bufs, save, count);
    gui_dialog_done();
    if (ret == VIM_CANCEL)
        goto theend;

    // Init ea pseudo-structure, this is needed for the check_overwrite()
    // function.
    CLEAR_FIELD(ea);

    for (i = 0; i < count; ++i)
    {
        // an autocommand may have deleted the buffer
        if (!bufref_valid(&bufrefs[i]) || !bufIsChanged(bufs[i]))
            continue;
        if (ret == VIM_YES && save[i])
            dialog_write_buf(bufs[i], &ea);
        else
            unchanged(bufs[i], TRUE, FALSE);
    }

theend:
    vim_free(bufs);
    vim_free(bufrefs);
    vim_free(save);
}

    int
gui_mch_dialog(
        int     type UNUSED,
        char_u  *title,
        char_u  *message,
        char_u  *buttons,
        int     def_but,
        char_u  *textfield,
        int     ex_cmd UNUSED)
{
    // Use a dialog like GNOME applications.
    return alert_dialog_run(title, message, buttons, def_but, textfield);
}

#endif // FEAT_GUI_GTK && USE_LIBADWAITA
