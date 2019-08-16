/* sound.c */
int has_any_sound_callback(void);
int has_sound_callback_in_queue(void);
void invoke_sound_callback(void);
void f_sound_playevent(typval_T *argvars, typval_T *rettv);
void f_sound_playfile(typval_T *argvars, typval_T *rettv);
void f_sound_stop(typval_T *argvars, typval_T *rettv);
void f_sound_clear(typval_T *argvars, typval_T *rettv);
void sound_free(void);
/* vim: set ft=c : */
