// C compound literals

// Source: https://en.cppreference.com/c/language/compound_literal

#include <stdio.h>

    static void
print_int_array(int numbers[3])
{
    printf("%d\n%d\n%d\n", numbers[0], numbers[1], numbers[2]);
}

    int
main(void)
{
    print_int_array((int[]){1,2,3});

    return 0;
}
