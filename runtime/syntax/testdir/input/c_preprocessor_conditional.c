// C preprocessor - conditional inclusion

// Source: https://en.cppreference.com/w/c/preprocessor/conditional

#define ABCD 2
#include <stdio.h>

int main(void)
{

#ifdef ABCD
    printf("1: yes\n");
#else
    printf("1: no\n");
#endif

#ifndef ABCD
    printf("2: no1\n");
#elif ABCD == 2
    printf("2: yes\n");
#else
    printf("2: no2\n");
#endif

#if !defined(DCBA) && (ABCD < 2 * 4 - 3)
    printf("3: yes\n");
#endif

// C23 directives #elifdef/#elifndef
#ifdef CPU
    printf("4: no1\n");
#elifdef GPU
    printf("4: no2\n");
#elifndef RAM
    printf("4: yes\n"); // selected in C23 mode, may be selected in pre-C23 mode
#else
    printf("4: no3\n"); // may be selected in pre-C23 mode
#endif
}

