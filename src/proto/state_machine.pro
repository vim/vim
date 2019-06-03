/* state_machine.c */

void sm_push(void *context, state_execute executeFn, state_cleanup cleanupFn);

void sm_execute_normal(char_u *keys);

void sm_execute(char_u* key);

char_u* sm_get_current_name(void);

sm_T* sm_get_current(void);

/* vim: set ft=c : */
