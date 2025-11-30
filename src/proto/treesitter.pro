/* treesitter.c */
int tsvim_init(void);
void tsvim_load_language(char_u *name, char_u *path, char_u *symbol_name);
tsobject_T *tsobject_ref(tsobject_T *obj);
void tsobject_free(tsobject_T *obj);
void tsobject_unref(tsobject_T *obj);
bool tsobject_equal(tsobject_T *a, tsobject_T *b);
char_u *tsobject_get_name(tsobject_T *obj);
int tsobject_get_refcount(tsobject_T *obj);
bool tsobject_is_parser(tsobject_T *obj);
bool tsobject_is_tree(tsobject_T *obj);
bool tsobject_is_node(tsobject_T *obj);
int check_tsobject_type_arg(typval_T *args, int idx, bool opt, bool (*func)(tsobject_T *obj));
tsobject_T *tsparser_new(void);
void tsparser_set_language(tsobject_T *parser, char_u *language);
tsobject_T *tsparser_parse_buf(tsobject_T *parser, tsobject_T *last_tree, buf_T *buf, long timeout);
void tstree_edit(tsobject_T *tree, uint32_t start_byte, uint32_t old_end_byte, uint32_t new_end_byte, uint32_t start_point[2], uint32_t old_end_point[2], uint32_t new_end_point[2]);
/* vim: set ft=c : */
