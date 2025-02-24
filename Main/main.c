//TODO: Setup init
//TODO: auto gcc build for original (also check for gcc) + make install.sh/deinstall with link etc
//TODO: flags later -> auto -> TODO: Doorverwijzing

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char **argv){
	printf("ForgeVim \n");
	printf("no Pre is being used...\nPress enter to continue normal (froge)mode... \n");
	getchar();

	// vim binary&args
	char *vim_path = "../src/vim";
    char *vim_args[] = {vim_path, "-u", "../Main/configs/vimrc", NULL};
    execvp(vim_args[0], vim_args);	//start norm

	perror("Fout bij starten van Vim");
	return 1;
}