/* vi:set ts=8 sts=4 sw=4:
 *
 * VIM - Vi IMproved		by Bram Moolenaar
 *				Amiga GUI support by Michael Nielsen
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 *
 * Amiga GUI header file.
 */

#if !defined(__GUI_AMIGA__H)
#define __GUI_AMIGA__H

#define SetAttrib(_ptr,_attrib,_value) ((_ptr)->_attrib=(_value))

#if defined(FEAT_GUI_AMIGA)

#include <intuition/intuition.h>

enum event {
    ev_NullEvent,
    ev_MenuVerify,
    ev_MenuPick,
    ev_CloseWindow,
    ev_NewSize,
    ev_RefreshWindow,
    ev_MouseButtons,
    ev_MouseMove,
    ev_GadgetDown,
    ev_GadgetUp,
    ev_KeyStroke,
    ev_IntuiTicks,
    ev_MenuHelp,
    ev_GadgetHelp,

    ev_Ignore
};

struct MyMenuItem {
    struct MenuItem menuItem;
    vimmenu_T	*guiMenu;
};

union myMenuItemUnion {
    struct MenuItem menuItem;
    struct MyMenuItem myMenuItem;
};

#endif /* FEAT_GUI_AMIGA*/
#endif /* __GUI_AMIGA__H */

