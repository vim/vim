/* sound.c */
int has_any_sound_callback(void);
void call_sound_callback(soundcb_T *soundcb, long snd_id, int result);
void delete_sound_callback(soundcb_T *soundcb);
int has_sound_callback_in_queue(void);
void invoke_sound_callback(void);
void f_sound_playevent(typval_T *argvars, typval_T *rettv);
void f_sound_playfile(typval_T *argvars, typval_T *rettv);
void f_sound_stop(typval_T *argvars, typval_T *rettv);
void f_sound_clear(typval_T *argvars, typval_T *rettv);
void sound_free(void);
/* vim: set ft=c : */
