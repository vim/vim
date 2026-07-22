// C preprocessor - binary resource inclusion

// Source: https://en.cppreference.com/w/c/preprocessor/embed

#include <stdint.h>
#include <stdio.h>

const uint8_t image_data[] = {
#embed "image.png"
};

const char message[] = {
#embed "message.txt" if_empty('M', 'i', 's', 's', 'i', 'n', 'g', '\n')
,'\0' // null terminator
};

void dump(const uint8_t arr[], size_t size)
{
    for (size_t i = 0; i != size; ++i)
        printf("%02X%c", arr[i], (i + 1) % 16 ? ' ' : '\n');
    puts("");
}

int main()
{
    puts("image_data[]:");
    dump(image_data, sizeof image_data);
    puts("message[]:");
    dump((const uint8_t*)message, sizeof message);
}

