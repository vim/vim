/* hashtable.c */
void hash_init __ARGS((hashtable *ht));
hashitem *hash_find __ARGS((hashtable *ht, char_u *key));
int hash_add __ARGS((hashtable *ht, char_u *key));
void hash_remove __ARGS((hashtable *ht, hashitem *hi));
/* vim: set ft=c : */
