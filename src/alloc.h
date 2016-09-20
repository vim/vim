/* vi:set ts=8 sts=4 sw=4 noet:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 */

/*
 * alloc.h: enumeration of alloc IDs.
 * Each entry must be on exactly one line, GetAllocId() depends on that.
 */
typedef enum {
	aid_none = 0,
	aid_qf_dirname_start,
	aid_qf_dirname_now,
	aid_qf_namebuf,
	aid_qf_errmsg,
	aid_qf_pattern,
	aid_last
} alloc_id_T;
