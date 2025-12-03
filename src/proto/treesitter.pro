/* treesitter.c */
int tsvim_init(void);
void tsvim_load_language(char_u *name, char_u *path, char_u *symbol_name);
opaque_T *tsparser_new(void);
void tsparser_set_language(opaque_T *parser, char_u *language);
opaque_T *tsparser_parse_buf( opaque_T *parser, opaque_T *last_tree, buf_T *buf, long timeout);
void tstree_edit(opaque_T *tree, uint32_t start_byte, uint32_t old_end_byte, uint32_t new_end_byte, uint32_t start_point[2], uint32_t old_end_point[2], uint32_t new_end_point[2]);
opaque_T *tstree_root_node(opaque_T *tree);
char_u *tsnode_type(opaque_T *node_obj);
int tsnode_symbol(opaque_T *node_obj);
dict_T *tsnode_position(opaque_T *node_obj);
opaque_T *tsquery_new(char_u *language, char_u *query_str);
/* vim: set ft=c : */
