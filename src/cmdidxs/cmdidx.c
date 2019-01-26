/* vi:set ts=8 sts=4 sw=4 noet: */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define BUFSIZE  (1024)
#define IDXSIZE  (255)
#define CH_START ('a')
#define CH_LAST  ('z')
#define CH_LEN   (CH_LAST - CH_START + 1)

/*
 * note: free return value if is not NULL.
 */
    static char*
parse_excmd(char* line)
{
    const char* s = "EX(CMD_";
    if (!strncmp(s, line, strlen(s)))
    {
	char* first = strchr(line, '"');
	char* second = strchr(first + 1, '"');
	if ((NULL != first) && (first < second))
	{
	    int size = sizeof(char) * (second - first);
	    char* p = malloc(size);
	    if (NULL != p)
	    {
		memset(p, 0x00, size);
		strncpy(p, first + 1, (second - first - 1));
		return p;
	    }
	}
    }
    return NULL;
}

    int
main(int argc, char *argv[])
{
    FILE* fp = NULL;
    char line[BUFSIZE];
    char* cmds[BUFSIZE];
    int cmds_cnt = 0;
    int cmdidxs1[IDXSIZE];
    int cmdidxs2[IDXSIZE][IDXSIZE];
    int i, j, k;
    char c1, c2;

    memset(line, 0x00, sizeof(line));
    memset(cmds, 0x00, sizeof(cmds));

    for (j = 0; j < IDXSIZE; j++)
    {
	cmdidxs1[j] = -1;
	for (k = 0; k < IDXSIZE; k++)
	    cmdidxs2[j][k] = -1;
    }

    fp = fopen("ex_cmds.h", "r");
    if (NULL != fp)
    {
        while (NULL != fgets(line, sizeof(line), fp))
        {
            char* cmd = parse_excmd(line);
            if (NULL != cmd)
            {
        	cmds[cmds_cnt] = cmd;
        	cmds_cnt++;
            }
        }
        fclose(fp);
    }

    for (i = cmds_cnt - 1; 0 <= i; i--)
    {
        char* cmd = cmds[i];
        int c1 = (int) cmd[0];
        int c2 = (int) cmd[1];
        cmdidxs1[c1] = i;
        if ((c2 >= CH_START) && (c2 <= CH_LAST))
        {
            cmdidxs2[c1][c2] = i;
        }
    }

    fp = fopen("ex_cmdidxs.h", "w");
    if (NULL != fp)
    {
        fprintf(fp, "/* Automatically generated code by cmdidxs\n");
        fprintf(fp, " *\n");
        fprintf(fp, " * Table giving the index of the first command in cmdnames[] to lookup\n");
        fprintf(fp, " * based on the first letter of a command.\n");
        fprintf(fp, " */\n");
        fprintf(fp, "static const unsigned short cmdidxs1[26] =\n");
        fprintf(fp, "{\n");
        for (c1 = CH_START; c1 <= CH_LAST; c1++)
        {
            fprintf(fp, "  /* %c */ %d%s\n", c1, cmdidxs1[(int) c1], (c1 == CH_LAST ? "" : ","));
        }
        fprintf(fp, "};\n");
        fprintf(fp, "\n");
        fprintf(fp, "/*\n");
        fprintf(fp, " * Table giving the index of the first command in cmdnames[] to lookup\n");
        fprintf(fp, " * based on the first 2 letters of a command.\n");
        fprintf(fp, " * Values in cmdidxs2[c1][c2] are relative to cmdidxs1[c1] so that they\n");
        fprintf(fp, " * fit in a byte.\n");
        fprintf(fp, " */\n");
        fprintf(fp, "static const unsigned char cmdidxs2[26][26] =\n");
        fprintf(fp, "{ /*      ");
        for (c1 = CH_START; c1 <= CH_LAST; c1++)
        {
            fprintf(fp, "%4c", c1);
        }
        fprintf(fp, " */\n");
        for (c1 = CH_START; c1 <= CH_LAST; c1++)
        {
            fprintf(fp, "  /* %c */ {", c1);
            for (c2 = CH_START; c2 <= CH_LAST; c2++)
            {
        	if (-1 != cmdidxs2[(int) c1][(int) c2])
        	{
        	    fprintf(fp, "%3d", cmdidxs2[(int) c1][(int) c2] - cmdidxs1[(int) c1]);
        	}
        	else
        	{
        	    fprintf(fp, "  0");
        	}
        	if (c2 != CH_LAST)
        	    fprintf(fp, ",");
            }
            fprintf(fp, " }%s\n", (c1 == CH_LAST ? "" : ","));
        }
        fprintf(fp, "};\n");
        fprintf(fp, "\n");
        fprintf(fp, "static const int command_count = %d;\n", cmds_cnt);

        fclose(fp);
    }

    for (i = 0; i < cmds_cnt; i++)
    {
        if (NULL != cmds[i])
        {
            free(cmds[i]);
        }
    }
}

