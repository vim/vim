// C++26 lexical constructs

// P3394 annotations: [[ ... ]] may carry a value expression.
[[=rename{"full_name"}]] int braced;
[[=key(8)]]              int parened;
[[nodiscard]]            int plain();

// P2900 contracts: contract_assert is a keyword.
void check(int x) { contract_assert(x != 0); }

// P2996 reflection (^^) and splicing ([: :]).
constexpr auto refl = ^^int;
[:refl:] spliced{};
