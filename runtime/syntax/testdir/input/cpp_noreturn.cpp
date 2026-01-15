#include <stdio.h>
#include <stdlib.h>

[[noreturn]] void error_exit(const char* reason)
{
    printf("Error: %s\n", reason);
    exit(1);
}

int main(void)
{
    puts("Preparing to exit...");
    error_exit("Assume something is wrong");
    puts("This code is never executed.");
}
