typedef struct {
  long length;
  int cols;
  int ebcdic;
  int hextype;
  int octspergrp;
  int autoskip;
  int seekoff;
  int displayoff;
  int relseek;
  int negseek;
  int grplen;
  int revert;
  int nonzero;
  int exit_code;
  FILE *fp;
  FILE *fpo;
  char *input_filename;
  char *error;
} xxd_ctx;

typedef enum {
  XXD_OK,
  XXD_ERROR,
  XXD_INPUT_ERROR,
  XXD_OUTPUT_ERROR,
  XXD_SEEK_ERROR,
  XXD_HUNTYPE_ERROR,
  XXD_USAGE_ERROR,
  XXD_ERROR_UNKNOWN
} xxd_rc;

void xxd_init(xxd_ctx *ctx);
xxd_rc xxd_parse_cmd_line(xxd_ctx *ctx, int argc, char **argv);
xxd_rc xxd_validate(xxd_ctx *ctx);
xxd_rc xxd(xxd_ctx *ctx);
char *xxd_rc_str(xxd_rc rc);

/* vi:set ts=8 sw=4 sts=2 cino+={2 cino+=n-2 : */
