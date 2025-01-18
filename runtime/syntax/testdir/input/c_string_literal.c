// C string literals

// Source: https://en.cppreference.com/w/c/language/string_literal

#include <inttypes.h>
#include <locale.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <uchar.h>

int main(void)
{
    char s1[] = "a猫🍌"; // or "a\u732B\U0001F34C"
#if __STDC_VERSION__ >= 202311L
    char8_t
#else
    char
#endif
    s2[] = u8"a猫🍌";
    char16_t s3[] = u"a猫🍌";
    char32_t s4[] = U"a猫🍌";
    wchar_t s5[] = L"a猫🍌";

    setlocale(LC_ALL, "en_US.utf8");
    printf("  \"%s\" is a char[%zu] holding     { ", s1, sizeof s1 / sizeof *s1);
    for(size_t n = 0; n < sizeof s1 / sizeof *s1; ++n)
        printf("0x%02X ", +(unsigned char)s1[n]);
    puts("}");
    printf(
#if __STDC_VERSION__ >= 202311L
    "u8\"%s\" is a char8_t[%zu] holding  { "
#else
    "u8\"%s\" is a char[%zu] holding     { "
#endif
, s2, sizeof s2 / sizeof *s2);
    for(size_t n = 0; n < sizeof s2 / sizeof *s2; ++n)
#if __STDC_VERSION__ >= 202311L
       printf("0x%02X ", s2[n]);
#else
       printf("0x%02X ", +(unsigned char)s2[n]);
#endif
    puts("}");
    printf(" u\"a猫🍌\" is a char16_t[%zu] holding { ", sizeof s3 / sizeof *s3);
    for(size_t n = 0; n < sizeof s3 / sizeof *s3; ++n)
       printf("0x%04" PRIXLEAST16" ", s3[n]);
    puts("}");
    printf(" U\"a猫🍌\" is a char32_t[%zu] holding { ", sizeof s4 / sizeof *s4);
    for(size_t n = 0; n < sizeof s4 / sizeof *s4; ++n)
       printf("0x%08" PRIXLEAST32" ", s4[n]);
    puts("}");
    printf(" L\"%ls\" is a wchar_t[%zu] holding  { ", s5, sizeof s5 / sizeof *s5);
    for(size_t n = 0; n < sizeof s5 / sizeof *s5; ++n)
       printf("0x%08X ", (unsigned)s5[n]);
    puts("}");
}

