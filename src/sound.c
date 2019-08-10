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

#if defined(FEAT_SOUND) || defined(PROTO)

static long	    sound_id = 0;

typedef struct soundcb_S soundcb_T;

struct soundcb_S {
    callback_T	snd_callback;
#ifdef MSWIN
    MCIDEVICEID	snd_device_id;
    long	snd_id;
#endif
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

#if defined(HAVE_CANBERRA) || defined(PROTO)

/*
 * Sound implementation for Linux/Unix/Mac using libcanberra.
 */
# include <canberra.h>

static ca_context   *context = NULL;

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

    argv[0].v_type = VAR_NUMBER;
    argv[0].vval.v_number = id;
    argv[1].v_type = VAR_NUMBER;
    argv[1].vval.v_number = error_code == CA_SUCCESS ? 0
			  : error_code == CA_ERROR_CANCELED
					    || error_code == CA_ERROR_DESTROYED
			  ? 1 : 2;
    argv[2].v_type = VAR_UNKNOWN;

    call_callback(&soundcb->snd_callback, -1, &rettv, 2, argv);
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

# if defined(EXITFREE) || defined(PROTO)
    void
sound_free(void)
{
    if (context != NULL)
	ca_context_destroy(context);
    while (first_callback != NULL)
	delete_sound_callback(first_callback);
}
# endif

#elif defined(MSWIN)

/*
 * Sound implementation for MS-Windows.
 */

static HWND g_hWndSound = NULL;

    static LRESULT CALLBACK
sound_wndproc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    soundcb_T	*p;

    switch (message)
    {
	case MM_MCINOTIFY:
	    for (p = first_callback; p != NULL; p = p->snd_next)
		if (p->snd_device_id == (MCIDEVICEID) lParam)
		{
		    typval_T	argv[3];
		    typval_T	rettv;
		    char	buf[32];

		    vim_snprintf(buf, sizeof(buf), "close sound%06ld",
								p->snd_id);
		    mciSendString(buf, NULL, 0, 0);

		    argv[0].v_type = VAR_NUMBER;
		    argv[0].vval.v_number = p->snd_id;
		    argv[1].v_type = VAR_NUMBER;
		    argv[1].vval.v_number =
					wParam == MCI_NOTIFY_SUCCESSFUL ? 0
				      : wParam == MCI_NOTIFY_ABORTED ? 1 : 2;
		    argv[2].v_type = VAR_UNKNOWN;

		    call_callback(&p->snd_callback, -1, &rettv, 2, argv);
		    clear_tv(&rettv);

		    delete_sound_callback(p);
		    redraw_after_callback(TRUE);

		}
	    break;
    }

    return DefWindowProc(hwnd, message, wParam, lParam);
}

    static HWND
sound_window()
{
    if (g_hWndSound == NULL)
    {
	LPCSTR clazz = "VimSound";
	WNDCLASS wndclass = {
	    0, sound_wndproc, 0, 0, g_hinst, NULL, 0, 0, NULL, clazz };
	RegisterClass(&wndclass);
	g_hWndSound = CreateWindow(clazz, NULL, 0, 0, 0, 0, 0,
		HWND_MESSAGE, NULL, g_hinst, NULL);
    }

    return g_hWndSound;
}

    void
f_sound_playevent(typval_T *argvars, typval_T *rettv)
{
    WCHAR	    *wp;

    rettv->v_type = VAR_NUMBER;
    rettv->vval.v_number = 0;

    wp = enc_to_utf16(tv_get_string(&argvars[0]), NULL);
    if (wp == NULL)
	return;

    PlaySoundW(wp, NULL, SND_ASYNC | SND_ALIAS);
    free(wp);

    rettv->vval.v_number = ++sound_id;
}

    void
f_sound_playfile(typval_T *argvars, typval_T *rettv)
{
    long	newid = sound_id + 1;
    size_t	len;
    char_u	*p, *esc;
    WCHAR	*wp;
    soundcb_T	*soundcb;
    char	buf[32];
    MCIERROR	err;

    rettv->v_type = VAR_NUMBER;
    rettv->vval.v_number = 0;

    esc = vim_strsave_shellescape(tv_get_string(&argvars[0]), FALSE, FALSE);

    len = STRLEN(esc) + 5 + 18 + 1;
    p = alloc(len);
    if (p == NULL)
    {
	free(esc);
	return;
    }
    vim_snprintf((char *)p, len, "open %s alias sound%06ld", esc, newid);
    free(esc);

    wp = enc_to_utf16((char_u *)p, NULL);
    free(p);
    if (wp == NULL)
	return;

    err = mciSendStringW(wp, NULL, 0, sound_window());
    free(wp);
    if (err != 0)
	return;

    vim_snprintf(buf, sizeof(buf), "play sound%06ld notify", newid);
    err = mciSendString(buf, NULL, 0, sound_window());
    if (err != 0)
	goto failure;

    sound_id = newid;
    rettv->vval.v_number = sound_id;

    soundcb = get_sound_callback(&argvars[1]);
    if (soundcb != NULL)
    {
	vim_snprintf(buf, sizeof(buf), "sound%06ld", newid);
	soundcb->snd_id = newid;
	soundcb->snd_device_id = mciGetDeviceID(buf);
    }
    return;

failure:
    vim_snprintf(buf, sizeof(buf), "close sound%06ld", newid);
    mciSendString(buf, NULL, 0, NULL);
}

    void
f_sound_stop(typval_T *argvars, typval_T *rettv UNUSED)
{
    long    id = tv_get_number(&argvars[0]);
    char    buf[32];

    vim_snprintf(buf, sizeof(buf), "stop sound%06ld", id);
    mciSendString(buf, NULL, 0, NULL);
}

    void
f_sound_clear(typval_T *argvars UNUSED, typval_T *rettv UNUSED)
{
    PlaySound(NULL, NULL, 0);
    mciSendString("close all", NULL, 0, NULL);
}

# if defined(EXITFREE)
    void
sound_free(void)
{
    CloseWindow(g_hWndSound);

    while (first_callback != NULL)
	delete_sound_callback(first_callback);
}
# endif

#endif // MSWIN

#endif  // FEAT_SOUND
