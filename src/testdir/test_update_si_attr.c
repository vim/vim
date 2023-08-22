#include <vim.h>

void test_update_si_attr(void)
{
  struct syntax_item *si;
  int attr;

  si = vim_alloc(sizeof(struct syntax_item));
  attr = syn_get_match_attr("foobar", 'matchgroup');

  update_si_attr(si);

  assert(si->si_attr == attr);

  /* Test that the si_attr is not changed if the end pattern is NULL */
  si->sp_end_pattern = NULL;
  update_si_attr(si);
  assert(si->si_attr == attr);

  /* Test that the si_attr is not changed if the end pattern has a different matchgroup attribute */
  si->sp_end_pattern = "baz";
  attr = syn_get_match_attr("baz", 'matchgroup');
  update_si_attr(si);
  assert(si->si_attr == attr);
}
