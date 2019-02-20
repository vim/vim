/* Must include main.c because it contains much more than just main() */
#define NO_VIM_MAIN
#include "main.c"

    int
fuzzer_init(void)
{
    mch_early_init();

    mb_init();
#ifdef FEAT_EVAL
    eval_init();	/* init global variables */
#endif
    init_normal_cmds();
    IObuff = alloc(IOSIZE);
    assert(IObuff);
    NameBuff = alloc(MAXPATHL);
    assert(NameBuff);

#ifdef FEAT_CLIPBOARD
    clip_init(FALSE);		/* Initialise clipboard stuff */
#endif

    stdout_isatty = FALSE;;

    if (win_alloc_first() == FAIL)
	mch_exit(0);

    init_yank();		/* init yank buffers */
    alist_init(&global_alist);	/* Init the argument list to empty. */
    global_alist.id = 0;

    init_homedir();		/* find real value of $HOME */
    set_init_1(FALSE);

#ifdef FEAT_EVAL
    set_lang_var();		/* set v:lang and v:ctype */
#endif

#ifdef FEAT_SIGNS
    init_signs();
#endif

    ++RedrawingDisabled;
    setbuf(stdout, NULL);
    win_init_size();

    cmdline_row = (int)(Rows - p_ch);
    msg_row = cmdline_row;
    screenalloc(FALSE);  // allocate screen buffers
    set_init_2();

    msg_scroll = TRUE;
    no_wait_return = TRUE;

    init_highlight(TRUE, FALSE);  // Default highlight groups.

    // -u NONE
    p_lpl = FALSE;
    // -i NONE
    set_option_value((char_u *)"vif", 0L, (char_u *)"NONE", 0);
    // -Z
    restricted = TRUE;
    // -e
    exmode_active = EXMODE_NORMAL;
    // -s
    silent_mode = TRUE;
    // -n
    p_uc = 0;

    starting = NO_BUFFERS;

    curbuf = curwin->w_buffer;
    set_buflisted(TRUE);
    (void)open_buffer(FALSE, NULL, 0);

    curwin->w_cursor.lnum = curbuf->b_ml.ml_line_count;
    starting = 0;

    apply_autocmds(EVENT_BUFENTER, NULL, NULL, FALSE, curbuf);
    setpcmark();

    win_enter(curwin, FALSE);
    shorten_fnames(FALSE);

    // Options to avoid side effects between input cases.
    do_cmdline_cmd((char_u*)"set nomodeline");
    do_cmdline_cmd((char_u*)"set history=0");
    do_cmdline_cmd((char_u*)"set noswapfile");

    return 1;
}

    int
fuzzer_load_file(const uint8_t *data, size_t size)
{
    // different filename, so multiple fuzzers can run at the same time
    char filename[64];
    sprintf(filename, "/tmp/fuzz.vim.%d", getpid());
    FILE *fp = fopen(filename, "w");
    if (!fp)
	abort();
    if (size != 0 && fwrite(data, size, 1, fp) != 1)
	abort();
    if (fclose(fp) != 0)
	abort();

    char cmdline[64];
    sprintf(cmdline, "silent! e ++bad=keep %s", filename);
    do_cmdline_cmd((char_u*)cmdline);

    do_cmdline_cmd((char_u*)"set buftype=nofile");
    if (unlink(filename) != 0)
	abort();

    return 1;
}

