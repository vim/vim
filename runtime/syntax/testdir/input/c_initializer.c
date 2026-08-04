// Compound literals and initializers inside expressions
void test(void)
{
    int a[(struct foo){ 42 }.bar];
    struct foo b = ((struct foo){ });
    _Static_assert(sizeof (int []){ 1, 2, 3 } == 3 * sizeof (int), "");
    for (struct foo i = { };;) break;
    int c[(struct foo) <% 42 %>.bar];
    struct foo d = ((struct foo) <% %>);
    f((int [2][2]){{ 1, 2 }, { 3, 4 }});
    syscall(SYS_clock_settime, CLOCK_REALTIME,
        tv ? &((struct timespec){ .tv_sec = tv->tv_sec,
            .tv_nsec = tv->tv_usec * 1000 }) : 0);
    f((struct timespec)
      <% .tv_sec = 0 %>);
    int e[){ 42 }.bar];
    f(a { 1 }); f(a, { 1 });
}
