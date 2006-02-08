/* vi:set ts=8 sts=4 sw=4:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */

/*
 * popupmenu.c: Popup menu (PUM)
 */
#include "vim.h"

#if defined(FEAT_INS_EXPAND) || defined(PROTO)

static pumitem_T *pum_array = NULL;	/* items of displayed pum */
static int pum_size;			/* nr of items in "pum_array" */
static int pum_selected;		/* index of selected item or -1 */
static int pum_first = 0;		/* index of top item */

static int pum_height;			/* nr of displayed pum items */
static int pum_width;			/* width of displayed pum items */
static int pum_base_width;		/* width of pum items base */
static int pum_scrollbar;		/* TRUE when scrollbar present */

static int pum_row;			/* top row of pum */
static int pum_col;			/* left column of pum */

#define PUM_DEF_HEIGHT 10
#define PUM_DEF_WIDTH  15

/*
 * Show the popup menu with items "array[size]".
 * "array" must remain valid until pum_undisplay() is called!
 * When possible the leftmost character is aligned with screen column "col".
 * The menu appears above the screen line "row" or at "row" + "height" - 1.
 */
    void
pum_display(array, size, selected, row, height, col)
    pumitem_T	*array;
    int		size;
    int		selected;	/* index of initially selected item */
    int		row;
    int		height;
    int		col;
{
    int		w;
    int		def_width = PUM_DEF_WIDTH;
    int		max_width = 0;
    int		extra_width = 0;
    int		i;

    /*
     * Figure out the size and position of the pum.
     */
    if (size < PUM_DEF_HEIGHT)
	pum_height = size;
    else
	pum_height = PUM_DEF_HEIGHT;

    /* Put the pum below "row" if possible.  If there are few lines decide on
     * where there is more room. */
    if (row >= cmdline_row - pum_height && row > (cmdline_row - height) / 2)
    {
	/* pum above "row" */
	if (row >= size)
	{
	    pum_row = row - size;
	    pum_height = size;
	}
	else
	{
	    pum_row = 0;
	    pum_height = row;
	}
    }
    else
    {
	/* pum below "row" */
	pum_row = row + height;
	if (size > cmdline_row - pum_row)
	    pum_height = cmdline_row - pum_row;
	else
	    pum_height = size;
    }

    /* don't display when we only have room for one line */
    if (pum_height < 1 || (pum_height == 1 && size > 1))
	return;

    /* Compute the width of the widest match and the widest extra. */
    for (i = 0; i < size; ++i)
    {
	w = vim_strsize(array[i].pum_text);
	if (max_width < w)
	    max_width = w;
	if (array[i].pum_extra != NULL)
	{
	    w = vim_strsize(array[i].pum_extra);
	    if (extra_width < w)
		extra_width = w;
	}
    }
    pum_base_width = max_width;

    /* if there are more items than room we need a scrollbar */
    if (pum_height < size)
    {
	pum_scrollbar = 1;
	++max_width;
    }
    else
	pum_scrollbar = 0;

    if (def_width < max_width)
	def_width = max_width;

    if (col < Columns - PUM_DEF_WIDTH || col < Columns - max_width)
    {
	/* align pum column with "col" */
	pum_col = col;
	pum_width = Columns - pum_col - pum_scrollbar;
	if (pum_width > max_width + extra_width + 1
						 && pum_width > PUM_DEF_WIDTH)
	{
	    pum_width = max_width + extra_width + 1;
	    if (pum_width < PUM_DEF_WIDTH)
		pum_width = PUM_DEF_WIDTH;
	}
    }
    else if (Columns < def_width)
    {
	/* not enough room, will use what we have */
	pum_col = 0;
	pum_width = Columns - 1;
    }
    else
    {
	if (max_width > PUM_DEF_WIDTH)
	    max_width = PUM_DEF_WIDTH;	/* truncate */
	pum_col = Columns - max_width;
	pum_width = max_width - pum_scrollbar;
    }

    pum_array = array;
    pum_size = size;

    /* Set selected item and redraw. */
    pum_set_selected(selected);
}

/*
 * Redraw the popup menu, using "pum_first" and "pum_selected".
 */
    void
pum_redraw()
{
    int		row = pum_row;
    int		col;
    int		attr_norm = highlight_attr[HLF_PNI];
    int		attr_select = highlight_attr[HLF_PSI];
    int		attr_scroll = highlight_attr[HLF_PSB];
    int		attr_thumb = highlight_attr[HLF_PST];
    int		attr;
    int		i;
    int		idx;
    char_u	*s;
    char_u	*p;
    int		totwidth, width, w;
    int		thumb_pos = 0;
    int		thumb_heigth = 1;
    int		round;

    if (pum_scrollbar)
    {
	thumb_heigth = pum_height * pum_height / pum_size;
	if (thumb_heigth == 0)
	    thumb_heigth = 1;
	thumb_pos = (pum_first * (pum_height - thumb_heigth)
			    + (pum_size - pum_height) / 2)
						    / (pum_size - pum_height);
    }

    for (i = 0; i < pum_height; ++i)
    {
	idx = i + pum_first;
	attr = (idx == pum_selected) ? attr_select : attr_norm;

	/* prepend a space if there is room */
	if (pum_col > 0)
	    screen_putchar(' ', row, pum_col - 1, attr);

	/* Display each entry, use two spaces for a Tab.
	 * Do this twice: For the main text and for the extra info */
	col = pum_col;
	totwidth = 0;
	for (round = 1; round <= 2; ++round)
	{
	    width = 0;
	    s = NULL;
	    for (p = round == 1 ? pum_array[idx].pum_text
				: pum_array[idx].pum_extra; ; mb_ptr_adv(p))
	    {
		if (s == NULL)
		    s = p;
		w = ptr2cells(p);
		if (*p == NUL || *p == TAB || totwidth + w > pum_width)
		{
		    /* Display the text that fits or comes before a Tab. */
		    screen_puts_len(s, p - s, row, col, attr);
		    col += width;

		    if (*p != TAB)
			break;

		    /* Display two spaces for a Tab. */
		    screen_puts_len((char_u *)"  ", 2, row, col, attr);
		    col += 2;
		    totwidth += 2;
		    s = NULL;	    /* start text at next char */
		    width = 0;
		}
		else
		    width += w;
	    }
	    if (round == 2 || pum_array[idx].pum_extra == NULL
					   || pum_base_width + 1 >= pum_width)
		break;
	    screen_fill(row, row + 1, col, pum_col + pum_base_width + 1,
							      ' ', ' ', attr);
	    col = pum_col + pum_base_width + 1;
	    totwidth = pum_base_width + 1;
	}

	screen_fill(row, row + 1, col, pum_col + pum_width, ' ', ' ', attr);
	if (pum_scrollbar > 0)
	    screen_putchar(' ', row, pum_col + pum_width,
		    i >= thumb_pos && i < thumb_pos + thumb_heigth
						  ? attr_thumb : attr_scroll);

	++row;
    }
}

#if 0 /* not used yet */
/*
 * Return the index of the currently selected item.
 */
    int
pum_get_selected()
{
    return pum_selected;
}
#endif

/*
 * Set the index of the currently selected item.  The menu will scroll when
 * necessary.
 */
    void
pum_set_selected(n)
    int	    n;
{
    pum_selected = n;

    if (pum_selected >= 0)
    {
	if (pum_first > pum_selected - 4)
	{
	    /* scroll down; when we did a jump it's probably a PageUp then
	     * scroll a whole page */
	    if (pum_first > pum_selected - 2)
	    {
		pum_first -= pum_height - 2;
		if (pum_first < 0)
		    pum_first = 0;
		else if (pum_first > pum_selected)
		    pum_first = pum_selected;
	    }
	    else
		pum_first = pum_selected;
	}
	else if (pum_first < pum_selected - pum_height + 5)
	{
	    /* scroll up; when we did a jump it's probably a PageDown then
	     * scroll a whole page */
	    if (pum_first < pum_selected - pum_height + 1 + 2)
	    {
		pum_first += pum_height - 2;
		if (pum_first < pum_selected - pum_height + 1)
		    pum_first = pum_selected - pum_height + 1;
	    }
	    else
		pum_first = pum_selected - pum_height + 1;
	}

	if (pum_height > 6)
	{
	    /* Give three lines of context when possible. */
	    if (pum_first > pum_selected - 3)
	    {
		/* scroll down */
		pum_first = pum_selected - 3;
		if (pum_first < 0)
		    pum_first = 0;
	    }
	    else if (pum_first < pum_selected + 3 - pum_height + 1)
	    {
		/* scroll up */
		pum_first = pum_selected + 3 - pum_height + 1;
	    }
	}
    }

    /* Never display more than we have */
    if (pum_first > pum_size - pum_height)
	pum_first = pum_size - pum_height;

    pum_redraw();
}

/*
 * Undisplay the popup menu (later).
 */
    void
pum_undisplay()
{
    pum_array = NULL;
    redraw_all_later(NOT_VALID);
}

/*
 * Clear the popup menu.  Currently only resets the offset to the first
 * displayed item.
 */
    void
pum_clear()
{
    pum_first = 0;
}

/*
 * Return TRUE if the popup menu is displayed.
 */
    int
pum_visible()
{
    return pum_array != NULL;
}

/*
 * Return the height of the popup menu, the number of entries visible.
 * Only valid when pum_visible() returns TRUE!
 */
    int
pum_get_height()
{
    return pum_height;
}

#endif
