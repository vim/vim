#include <stdfloat>

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

        // extended floating-point types (since C++23)
        std::float16_t f16 = 1.0F16;
        std::float32_t f32 = .1'2e+3'4f32;
        std::float64_t f64 = -0.123'456F64;
        std::float128_t f128 = +0x1'23.A'BCp-1'234f128;
        std::bfloat16_t bf16 = 0X1P-7BF16;
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
