/* vi:set ts=8 sts=4 sw=4:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *			Visual Workshop integration by Gordon Prieur
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 */
/*
  THIS IS AN UNSTABLE INTERFACE! It is unsupported and will likely
  change in future releases, possibly breaking compatibility!
*/

#ifndef _INTEGRATION_H
#define _INTEGRATION_H

#include <X11/Intrinsic.h>
#include <Xm/Xm.h>

#ifdef  __cplusplus
extern "C" {
#endif

/* Enable NoHands test support functions. Define this only if you want to
   compile in support in the editor such that it can be run under
   the WorkShop test suite. */
#ifndef NOHANDS_SUPPORT_FUNCTIONS
#define NOHANDS_SUPPORT_FUNCTIONS
#endif


/* This header file has three parts.
 * 1. Functions you need to implement; these are called by the integration
 *    library
 * 2. Functions you need to call when certain events happen in the editor;
 *    these are implemented by the integration library
 * 3. Utility functions provided by the integration library; these make
 *	  task 1 a bit easier.
 */

/*
 * The following functions need to be implemented by the editor
 * integration code (and will be editor-specific). Please see the
 * sample workshop.c file for comments explaining what each functions
 * needs to do, what the arguments mean, etc.
 */

/*
 * This string is recognized by eserve and should be all lower case.
 * This is how the editor detects that it is talking to NEdit instead
 * of Vim, for example, when the connection is initiated from the editor.
 * Examples: "nedit", "gvim"
 */
char *workshop_get_editor_name();

/*
 * Version number of the editor.
 * This number is communicated along with the protocol
 * version to the application.
 * Examples: "5.0.2", "19.3"
 */
char *workshop_get_editor_version();


/* Goto a given line in a given file */
void workshop_goto_line(char *filename, int lineno);


/* Set mark in a given file */
void workshop_set_mark(char *filename, int lineno, int markId, int type);


/* Change mark type (for example from current-pc to pc-and-breakpoint) */
void workshop_change_mark_type(char *filename, int markId, int type);

/*
 * Goto the given mark in a file (e.g. show it).
 * If message is not null, display it in the footer.
 */

void workshop_goto_mark(char *filename, int markId, char *message);


/* Delete mark */
void workshop_delete_mark(char *filename, int markId);

/* Begin/end pair of messages indicating that a series of _set_mark and
 * _delete_mark messages will be sent. This can/should be used to suppress gui
 * redraws between the begin and end messages. For example, if you switch
 * to a headerfile that has a class breakpoint set, there may be hundreds
 * of marks that need to be added. You don't want to refresh the gui for each
 * added sign, you want to wait until the final end message.
 */
void workshop_mark_batch_begin();
void workshop_mark_batch_end();


/* Load a given file into the WorkShop buffer. "frameid" is a token string
 * that identifies which frame the file would like to be loaded into. This
 * will usually be null, in which case you should use the default frame.
 * However, if frameid is not null, you need to find a frame that has this
 * frameid, and replace the file in that frame. Finally, if the frameid is
 * one you haven't seen before, you should create a new frame for this file.
 * Note that "frameid" is a string value, not just an opaque pointer, so
 * you should use strcmp rather than == when testing for equality.
 */
void workshop_load_file(char *filename, int line, char *frameid);


/* Reload the WorkShop buffer */
void workshop_reload_file(char *filename, int line);


/* Show the given file */
void workshop_show_file(char *filename);


/* Front the given file */
void workshop_front_file(char *filename);


/* Save the given file  */
void workshop_save_file(char *filename);

/* Save all WorkShop edited files. You can ask user about modified files
 * and skip saving any files the user doesn't want to save.
 * This function is typically called when the user issues a build, a fix,
 * etc. (and also if you select "Save All" from the File menu :-)
 */
void workshop_save_files();

/* Show a message in all footers.
   Severity currently is not defined. */
void workshop_footer_message(char *message, int severity);

/* Minimize all windows */
void workshop_minimize();


/* Maximize all windows */
void workshop_maximize();


/*
 * Create a new mark type, assign it a given index, a given textbackground
 * color, and a given left-margin sign (where sign is a filename to an
 * .xpm file)
 */
void workshop_add_mark_type(int idx, char *colorspec, char *sign);


/* Get mark line number */
int workshop_get_mark_lineno(char *filename, int markId);


/* Exit editor; save confirmation dialogs are okay */
void workshop_quit();

/* Set an editor option.
 * For example, name="syntax",value="on" would enable syntax highlighting.
 * The currently defined options are:
 *    lineno		{on,off}	show line numbers
 *    syntax		{on,off}	highlight syntax
 *    parentheses	{on,off}	show matching parentheses
 * The following options are interpreted by the library for you (so you
 * will never see the message. However, the implementation requires you
 * to provide certain callbacks, like restore hotkeys or save all files.
 * These are documented separately).
 *    workshopkeys	{on,off}	set workshop hotkeys
 *    savefiles		{on,off}	save all files before issuing a build
 *    balloon		{on,off}	enable/disable balloon evaluate
 *
 * IGNORE an option if you do not recognize it.
 */
void workshop_set_option(char *name, char *value);

/*
 * (See workshop_add_frame first.) This function notifies the editor
 * that the frame for the given window (indicated by "frame", which
 * was supplied by the editor in workshop_add_frame) has been created.
 * This can happen much later than the workshop_add_frame message, since
 * often a window is created on editor startup, while the frame description
 * is passed over from eserve much later, when the connection is complete.
 * This gives the editor a chance to kick its GUI to show the frame
 * properly; typically you'll unmanage and remanage the parent widget to
 * force a geometry recalculation.
 */

void workshop_reconfigure_frame(void *frame);


/* Are there any moved marks? If so, call workshop_move_mark on
 * each of them now. This is how eserve can find out if for example
 * breakpoints have moved when a program has been recompiled and
 * reloaded into dbx.
 */
void workshop_moved_marks(char *filename);


/* A button in the toolbar has been pushed. "frame" is provided
 * which should let you determine which toolbar had a button pushed
 * (you supplied this clientData when you created a toolbar). From
 * this you should be able to figure out which file the operation
 * applies to, and for that window the cursor line and column,
 * selection begin line and column, selection end line and column,
 * selection text and selection text length. The column numbers are
 * currently unused but implement it anyway in case we decide to use
 * them in the future.
 * Note that frame can be NULL. In this case, you should pick
 * a default window to translate coordinates for (ideally, the
 * last window the user has operated on.) This will be the case when
 * the user clicks on a Custom Button programmed to take the current
 * line number as an argument. Here it's ambiguous which buffer
 * to use, so you need to pick one.
 * (Interface consideration: Perhaps we instead should add smarts
 * into the library such that we remember which frame pointer
 * we last noticed (e.g. last call to get_positions, or perhaps
 * last add_frame) and then pass that instead? For example, we could
 * have all workshop operations return the clientData when passed
 * the filename (or add a filename-to-clientData converter?) and then
 * remember the last filename/clientData used.
 */
int workshop_get_positions(void *frame,
			   char **filename,
			   int *curLine,
			   int *curCol,
			   int *selStartLine,
			   int *selStartCol,
			   int *selEndLine,
			   int *selEndCol,
			   int *selLength,
			   char **selection);

/* The following function should return the height of a character
 * in the text display. This is used to pick out a suitable size
 * for the signs to match the text (currently available in three
 * sizes). If you just return 0, WorkShop will use the default
 * sign size. (Use XmStringExtent on character "A" to get the height.)
 */

int workshop_get_font_height(void);

/* The following function requests that you register the given
 * hotkey as a keyboard accelerator for all frames. Whenever the
 * hotkey is pressed, you should invoke  workshop_hotkey_pressed
 * and pass the current frame pointer as an argument as well as
 * the clientData pointer passed in to this function.
 * The remove function unregisters the hotkey.
 */
void workshop_register_hotkey(Modifiers modifiers, KeySym keysym,
			      void *clientData);
void workshop_unregister_hotkey(Modifiers modifiers, KeySym keysym,
				void *clientData);




/*
 *
 * The following functions notify eserve of important editor events,
 * such as files being modified, files being saved, etc. You must
 * sprinkle your editor code with calls to these. For example, whenever
 * a file is modified (well, when its read-only status changes to modified),
 * call workshop_file_modified().
 *
 */



/* Connect with eserve. Add this call after you editor initialization
 * is done, right before entering the event loop or blocking on input.
 * This will set up a socket connection with eserve.
 */
void workshop_connect(XtAppContext context);

/* A file has been opened. */
void workshop_file_opened(char *filename, int readOnly);


/* A file has been saved. Despite its name, eserve also uses this
 * message to mean a file has been reverted or unmodified.
 */
void workshop_file_saved(char *filename);


/* A file has been closed */
void workshop_file_closed(char *filename);

/* Like workshop_file_closed, but also inform eserve what line the
   cursor was on when you left the file. That way eserve can put you
   back where you left off when you return to this file. */
void workshop_file_closed_lineno(char *filename, int line);

/* A file has been modified */
void workshop_file_modified(char *filename);


/*
 * A mark has been moved. Only call this as a response to
 * a workshop_moved_marks request call.
 */
void workshop_move_mark(char *filename, int markId, int newLineno);

/* Tell the integration library about a new frame being added.
 * Supply a form for the toolbar, a label for the footer, and an
 * XmPulldown menu for the WorkShop menu to attach to. Top and bottom
 * are the widgets above and below the toolbar form widget, if
 * any. Call this function when you create a new window. It returns a
 * void *, a handle which you should keep and return when you delete
 * the window with workshop_delete_toolbar.  The "footer" argument
 * points to a Label widget that is going to be used as a status
 * message area, and "menu" (if any) points to an Menu widget that
 * should contain a WorkShop menu.  Clientdata is a pointer which is
 * only used by the editor. It will typically be a pointer to the
 * window object that the toolbar is placed in. If you have multiple
 * windows, you need to use this pointer to figure out which window
 * (and thus corresponding buffer) the user has clicked on to respond
 * to the workshop_get_positions message.
 * Each frame's clientData ("frame") should be unique.
 */
void *workshop_add_frame(void *frame, Widget form,
			   Widget top, Widget bottom, Widget footer,
			   Widget menu);

/* Delete a window/frame. Call this when an editor window is being deleted. */
void workshop_delete_frame(void *handle);

/* Add a balloon evaluate text area. "frame" is used the same way
 * as in workshop_add_frame. This call is not part of workshop_add_frame because
 * a frame can have multiple tooltip areas (typically, an editor frame that
 * is split showing multiple buffers will have a separate tooltip area for
 * each text widget. Each such area is called a "window" (consistent with
 * XEmacs terminology). Separate these by the window argument if necessary.
 * You will need to implement workshop_get_balloon_text such that it uses
 * these two arguments to derive the file, line etc. for the tip.
 * Call the remove function if you delete this area such that the integration
 * library can update itself. You must call workshop_add_frame before you
 * call add_balloon_eval_area, and you must pass the same frame pointer.
 */
void workshop_add_balloon_eval_area(void *frame, void *window, Widget widget);
void workshop_remove_balloon_eval_area(void *frame, void *window, Widget widget);


/* For a given mouse position inside the balloon area (passed as x,y),
 * return the balloon text to be evaluated. There are two scenarios:
 * If the position is inside the selection, return the selection
 * string.  Else, return the full line (or possibly the full line up
 * to the last semicolon (that's TBD), along with an index pointing to
 * where which character the mouse is over.
 * If we have the selection-scenario, set mouseIndex to -1 to indicate
 * that no autoexpansion should occur but that the selection should
 * be evaluated as is.
 *
 * XXX Does dbx need more information here, like the filename and line
 * number in order to determine the correct language and scope to be
 * used during evaluation?? Or should it just work like the p= button
 * (where the current scope and language is used, even if you are
 * pointing at a different file with a different scope) ?
 */
int workshop_get_balloon_text(Position x, Position y,
			      void *frame,
			      void *window,
			      char **filename,
			      int *line,
			      char **text,
			      int *mouseIndex);


/* Window size and location
 * WorkShop will attempt to restore the size and location of a single
 * editor frame. For vi, this window is designated as the "reusable" one.
 * You can implement your own scheme for determining which window you
 * want to associate with WorkShop. Whenever the size and location of
 * this window is changed, call the following function to notify eserve.
 * Like workshop_invoked, this can be called before the workshop_connect()
 * call.
 */
void workshop_frame_moved(int new_x, int new_y, int new_w, int new_h);
Boolean workshop_get_width_height(int *, int *);
Boolean workshop_get_rows_cols(int *, int *);

/* This function should be invoked when you press a hotkey
 * set up by workshop_register_hotkey. Pass the clientData
 * to it that was given to you with workshop_register_hotkey.
*/
void workshop_hotkey_pressed(void *frame, void *clientData);





/*
 * Utility functions
 * These provide convenience functions to simplify implementing some
 * of the above functions.
 *
 */

/* Were we invoked by WorkShop? This function can be used early during startup
 * if you want to do things differently if the editor is started standalone
 * or in WorkShop mode. For example, in standalone mode you may not want to
 * add a footer/message area or a sign gutter.
 */
int workshop_invoked(void);

/*
 *Set the desktop icon of the current shell to the given xpm icon.
 * Standard WorkShop desktop icons should be 48x48.
 */

void workshop_set_icon(Display *display, Widget shell, char **xpmdata,
		       int width, int height);


/* Minimize (iconify) the given shell */
void workshop_minimize_shell(Widget shell);

/* Maximize (deiconify) the given shell */
void workshop_maximize_shell(Widget shell);

/* Called by frame.cc -- editor shouldn't call this directly.
 * Perhaps we need an integrationP.h file ? */
void workshop_perform_verb(char *verb, void *clientData);
void workshop_send_message(char *buf);


#ifdef NOHANDS_SUPPORT_FUNCTIONS
/* The following functions are needed to run the WorkShop testsuite
 * with this editor. You don't need to implement these unless you
 * intend for your editor to be run by Workshop's testsuite.
 * getcursorrow should return the number of lines from the top of
 * the window the cursor is; similarly for getcursorcol.
 */
char *workshop_test_getcurrentfile();
int workshop_test_getcursorrow();
int workshop_test_getcursorcol();
char *workshop_test_getcursorrowtext();
char *workshop_test_getselectedtext();
#endif

/*
 * Struct used to set/unset the sensitivity of verbs.
 */
typedef struct {
	char		*verb;
	Boolean		sense;
} VerbSense;

#ifdef  __cplusplus
}
#endif

#endif /* _INTEGRATION_H */
