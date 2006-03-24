/* hashtab.c */
extern void hash_init __ARGS((hashtab_T *ht));
extern void hash_clear __ARGS((hashtab_T *ht));
extern void hash_clear_all __ARGS((hashtab_T *ht, int off));
extern hashitem_T *hash_find __ARGS((hashtab_T *ht, char_u *key));
extern hashitem_T *hash_lookup __ARGS((hashtab_T *ht, char_u *key, hash_T hash));
extern void hash_debug_results __ARGS((void));
extern int hash_add __ARGS((hashtab_T *ht, char_u *key));
extern int hash_add_item __ARGS((hashtab_T *ht, hashitem_T *hi, char_u *key, hash_T hash));
extern void hash_remove __ARGS((hashtab_T *ht, hashitem_T *hi));
extern void hash_lock __ARGS((hashtab_T *ht));
extern void hash_unlock __ARGS((hashtab_T *ht));
extern hash_T hash_hash __ARGS((char_u *key));
/* vim: set ft=c : */
