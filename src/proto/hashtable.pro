/* hashtable.c */
void hash_init __ARGS((hashtable *ht));
void hash_clear __ARGS((hashtable *ht));
hashitem *hash_find __ARGS((hashtable *ht, char_u *key));
int hash_add __ARGS((hashtable *ht, char_u *key));
void hash_remove __ARGS((hashtable *ht, hashitem *hi));
void hash_lock __ARGS((hashtable *ht));
void hash_unlock __ARGS((hashtable *ht));
/* vim: set ft=c : */
