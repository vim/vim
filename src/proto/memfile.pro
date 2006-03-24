/* memfile.c */
extern memfile_T *mf_open __ARGS((char_u *fname, int flags));
extern int mf_open_file __ARGS((memfile_T *mfp, char_u *fname));
extern void mf_close __ARGS((memfile_T *mfp, int del_file));
extern void mf_close_file __ARGS((buf_T *buf, int getlines));
extern void mf_new_page_size __ARGS((memfile_T *mfp, unsigned new_size));
extern bhdr_T *mf_new __ARGS((memfile_T *mfp, int negative, int page_count));
extern bhdr_T *mf_get __ARGS((memfile_T *mfp, blocknr_T nr, int page_count));
extern void mf_put __ARGS((memfile_T *mfp, bhdr_T *hp, int dirty, int infile));
extern void mf_free __ARGS((memfile_T *mfp, bhdr_T *hp));
extern int mf_sync __ARGS((memfile_T *mfp, int flags));
extern void mf_set_dirty __ARGS((memfile_T *mfp));
extern int mf_release_all __ARGS((void));
extern blocknr_T mf_trans_del __ARGS((memfile_T *mfp, blocknr_T old_nr));
extern void mf_set_ffname __ARGS((memfile_T *mfp));
extern void mf_fullname __ARGS((memfile_T *mfp));
extern int mf_need_trans __ARGS((memfile_T *mfp));
/* vim: set ft=c : */
