/* vi:set ts=8 sts=4 sw=4 noet:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */

/*
 * sound.c: functions related making noise
 */

#include "vim.h"

#if (defined(FEAT_SOUND) && defined(HAVE_CANBERRA)) || defined(PROTO)

#include <canberra.h>

static long	    sound_id = 0;
static ca_context   *context = NULL;

typedef struct soundcb_S soundcb_T;

struct soundcb_S {
    callback_T	snd_callback;
    soundcb_T	*snd_next;
};

static soundcb_T    *first_callback = NULL;

    static soundcb_T *
get_sound_callback(typval_T *arg)
{
    callback_T	callback;
    soundcb_T	*soundcb;

    if (arg->v_type == VAR_UNKNOWN)
	return NULL;
    callback = get_callback(arg);
    if (callback.cb_name == NULL)
	return NULL;

    soundcb = ALLOC_ONE(soundcb_T);
    if (soundcb == NULL)
	free_callback(&callback);
    else
    {
	soundcb->snd_next = first_callback;
	first_callback = soundcb;
	set_callback(&soundcb->snd_callback, &callback);
    }
    return soundcb;
}

/*
 * Delete "soundcb" from the list of pending callbacks.
 */
    static void
delete_sound_callback(soundcb_T *soundcb)
{
    soundcb_T	*p;
    soundcb_T	*prev = NULL;

    for (p = first_callback; p != NULL; prev = p, p = p->snd_next)
	if (p == soundcb)
	{
	    if (prev == NULL)
		first_callback = p->snd_next;
	    else
		prev->snd_next = p->snd_next;
	    free_callback(&p->snd_callback);
	    vim_free(p);
	    break;
	}
}

    static void
sound_callback(
	ca_context  *c UNUSED,
	uint32_t    id,
	int	    error_code,
	void	    *userdata)
{
    soundcb_T	*soundcb = (soundcb_T *)userdata;
    typval_T	argv[3];
    typval_T	rettv;
    int		dummy;

    argv[0].v_type = VAR_NUMBER;
    argv[0].vval.v_number = id;
    argv[1].v_type = VAR_NUMBER;
    argv[1].vval.v_number = error_code == CA_SUCCESS ? 0
			  : error_code == CA_ERROR_CANCELED
					    || error_code == CA_ERROR_DESTROYED
			  ? 1 : 2;
    argv[2].v_type = VAR_UNKNOWN;

    call_callback(&soundcb->snd_callback, -1,
			    &rettv, 2, argv, NULL, 0L, 0L, &dummy, TRUE, NULL);
    clear_tv(&rettv);

    delete_sound_callback(soundcb);
    redraw_after_callback(TRUE);
}

    static void
sound_play_common(typval_T *argvars, typval_T *rettv, int playfile)
{
    if (context == NULL)
	ca_context_create(&context);
    if (context != NULL)
    {
	soundcb_T	*soundcb = get_sound_callback(&argvars[1]);
	int		res = CA_ERROR_INVALID;

	++sound_id;
	if (soundcb == NULL)
	{
	    res = ca_context_play(context, sound_id,
		    playfile ? CA_PROP_MEDIA_FILENAME : CA_PROP_EVENT_ID,
						    tv_get_string(&argvars[0]),
		    CA_PROP_CANBERRA_CACHE_CONTROL, "volatile",
		    NULL);
	}
	else
	{
	    static ca_proplist *proplist = NULL;

	    ca_proplist_create(&proplist);
	    if (proplist != NULL)
	    {
		if (playfile)
		    ca_proplist_sets(proplist, CA_PROP_MEDIA_FILENAME,
					   (char *)tv_get_string(&argvars[0]));
		else
		    ca_proplist_sets(proplist, CA_PROP_EVENT_ID,
					   (char *)tv_get_string(&argvars[0]));
		ca_proplist_sets(proplist, CA_PROP_CANBERRA_CACHE_CONTROL,
			"volatile");
		res = ca_context_play_full(context, sound_id, proplist,
						      sound_callback, soundcb);
		if (res != CA_SUCCESS)
		    delete_sound_callback(soundcb);

		ca_proplist_destroy(proplist);
	    }
	}
	rettv->vval.v_number = res == CA_SUCCESS ? sound_id : 0;
    }
}

    void
f_sound_playevent(typval_T *argvars, typval_T *rettv)
{
    sound_play_common(argvars, rettv, FALSE);
}

/*
 * implementation of sound_playfile({path} [, {callback}])
 */
    void
f_sound_playfile(typval_T *argvars, typval_T *rettv)
{
    sound_play_common(argvars, rettv, TRUE);
}

/*
 * implementation of sound_stop({id})
 */
    void
f_sound_stop(typval_T *argvars, typval_T *rettv UNUSED)
{
    if (context != NULL)
	ca_context_cancel(context, tv_get_number(&argvars[0]));
}

/*
 * implementation of sound_clear()
 */
    void
f_sound_clear(typval_T *argvars UNUSED, typval_T *rettv UNUSED)
{
    if (context != NULL)
    {
	ca_context_destroy(context);
	context = NULL;
    }
}

#if defined(EXITFREE) || defined(PROTO)
    void
sound_free(void)
{
    if (context != NULL)
	ca_context_destroy(context);
    while (first_callback != NULL)
	delete_sound_callback(first_callback);
}
#endif

#endif  // FEAT_SOUND && HAVE_CANBERRA
