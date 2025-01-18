// C integer constants

// Source: https://en.cppreference.com/w/c/language/integer_constant

#include <inttypes.h>
#include <stdio.h>

int main(void)
{
    printf("123 = %d\n", 123);
    printf("0123 = %d\n", 0123);
    printf("0x123 = %d\n", 0x123);
    printf("12345678901234567890ull = %llu\n", 12345678901234567890ull);
    // the type is a 64-bit type (unsigned long long or possibly unsigned long)
    // even without a long suffix
    printf("12345678901234567890u = %"PRIu64"\n", 12345678901234567890u );

    // printf("%lld\n", -9223372036854775808); // Error:
        // the value 9223372036854775808 cannot fit in signed long long, which
        // is the biggest type allowed for unsuffixed decimal integer constant

    printf("%llu\n", -9223372036854775808ull );
    // unary minus applied to unsigned value subtracts it from 2^64,
    // this gives unsigned 9223372036854775808

    printf("%lld\n", -9223372036854775807ll - 1);
    // correct way to form signed value -9223372036854775808
}

// The following variables are initialized to the same value:

int d = 42;
int o = 052;
int x = 0x2a;
int X = 0X2A;
int b = 0b101010; // C23

// The following variables are also initialized to the same value: 

unsigned long long l1 = 18446744073709550592ull; // C99
unsigned long long l2 = 18'446'744'073'709'550'592llu; // C23
unsigned long long l3 = 1844'6744'0737'0955'0592uLL; // C23
unsigned long long l4 = 184467'440737'0'95505'92LLU; // C23

