void foo()
{
    {
        // See https://en.cppreference.com/w/cpp/language/floating_literal
        double a = 1.0E2;
        double b = 1.0e2;
        double c = 1E2;
        double d = 1e2;

        double e = 0X.1ffP10;
        float f = 0xFFp+2F;
        double g = 0x67.P-2;
        long double h = 0x1'2.f'fp-1'0L;
    }

    // See: https://en.cppreference.com/w/cpp/language/integer_literal
    int a = 0X123;
    int b = 0x123;

    int c = 0B101;
    int d = 0b101;

    // size_t suffix (since C++23)
    {
        auto dec = 123Z;
        auto bin = 0B10'10ZU;
        auto oct = 01'750Uz;
        auto hex = 0x12'EFz;
    }
}
