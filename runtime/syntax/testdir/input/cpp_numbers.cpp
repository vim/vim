void foo()
{
    {
        // See https://en.cppreference.com/w/cpp/language/floating_literal
        double a = 1.0E2;
        double b = 1.0e2;
        double c = 1E2;
        double d = 1e2;

        double e = 0X1ffp10;
        double f = 0x1ffp10;
    }

    // See: https://en.cppreference.com/w/cpp/language/integer_literal
    int a = 0X123;
    int b = 0x123;

    int c = 0B101;
    int d = 0b101;
}
