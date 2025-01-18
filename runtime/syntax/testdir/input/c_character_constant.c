// C character constants

// Source: https://en.cppreference.com/w/c/language/character_constant

#include <stddef.h>
#include <stdio.h>
#include <uchar.h>

int main (void)
{
    printf("constant value     \n");
    printf("-------- ----------\n");

    // integer character constants,
    int c1='a'; printf("'a':\t %#010x\n", c1);
    int c2='🍌'; printf("'🍌':\t %#010x\n\n", c2); // implementation-defined

    // multicharacter constant
    int c3='ab'; printf("'ab':\t %#010x\n\n", c3); // implementation-defined

    // 16-bit wide character constants
    char16_t uc1 = u'a'; printf("'a':\t %#010x\n", (int)uc1);
    char16_t uc2 = u'¢'; printf("'¢':\t %#010x\n", (int)uc2);
    char16_t uc3 = u'猫'; printf("'猫':\t %#010x\n", (int)uc3);
    // implementation-defined (🍌 maps to two 16-bit characters)
    char16_t uc4 = u'🍌'; printf("'🍌':\t %#010x\n\n", (int)uc4);

    // 32-bit wide character constants
    char32_t Uc1 = U'a'; printf("'a':\t %#010x\n", (int)Uc1);
    char32_t Uc2 = U'¢'; printf("'¢':\t %#010x\n", (int)Uc2);
    char32_t Uc3 = U'猫'; printf("'猫':\t %#010x\n", (int)Uc3);
    char32_t Uc4 = U'🍌'; printf("'🍌':\t %#010x\n\n", (int)Uc4);

    // wide character constants
    wchar_t wc1 = L'a'; printf("'a':\t %#010x\n", (int)wc1);
    wchar_t wc2 = L'¢'; printf("'¢':\t %#010x\n", (int)wc2);
    wchar_t wc3 = L'猫'; printf("'猫':\t %#010x\n", (int)wc3);
    wchar_t wc4 = L'🍌'; printf("'🍌':\t %#010x\n\n", (int)wc4);
}

