/* gc.c */
int get_copyID(void);
int garbage_collect(int testing);
int set_ref_in_ht(hashtab_T *ht, int copyID, list_stack_T **list_stack, tuple_stack_T **tuple_stack);
int set_ref_in_dict(dict_T *d, int copyID);
int set_ref_in_list(list_T *ll, int copyID);
int set_ref_in_list_items(list_T *l, int copyID, ht_stack_T **ht_stack, tuple_stack_T **tuple_stack);
int set_ref_in_tuple_items(tuple_T *tuple, int copyID, ht_stack_T **ht_stack, list_stack_T **list_stack);
int set_ref_in_callback(callback_T *cb, int copyID);
int set_ref_in_item_class(class_T *cl, int copyID, ht_stack_T **ht_stack, list_stack_T **list_stack, tuple_stack_T **tuple_stack);
int set_ref_in_item(typval_T *tv, int copyID, ht_stack_T **ht_stack, list_stack_T **list_stack, tuple_stack_T **tuple_stack);
/* vim: set ft=c : */
