/* vi:set ts=8 sts=4 sw=4:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 */

#define FEAT_BROWSE

#define TASK 0x4b534154

/* Nested wimp flags: */
#define CHILD_FIX_TO_WORKAREA	0
#define CHILD_FIX_TO_LEFT	1
#define CHILD_FIX_TO_BOTTOM	1
#define CHILD_FIX_TO_RIGHT	2
#define CHILD_FIX_TO_TOP	2

#define CHILD_SELF_SCROLL	0
#define CHILD_PARENT_SCROLL	1

#define CHILD_LEFT		16
#define CHILD_BOTTOM		18
#define CHILD_RIGHT		20
#define CHILD_TOP		22
#define CHILD_SCROLL_X		24
#define CHILD_SCROLL_Y		26

int wimp_poll(int mask, int *block);
int wimp_poll_idle(int mask, int *block, int end_time);
void ro_open_main(int *block);
