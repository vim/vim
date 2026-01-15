#include <stdio.h>
#include <stdlib.h>
#include <stdnoreturn.h>

// causes undefined behavior if i <= 0
// exits if i > 0
noreturn void exit_now(int i) // or _Noreturn void exit_now(int i)
{
    if (i > 0)
        exit(i);
}

int main(void)
{
    puts("Preparing to exit...");
    exit_now(2);
    puts("This code is never executed.");
}
