#
# Common Makefile, defines the list of tests to run.
#

# Individual tests, including the ones part of test_alot
# alot tests have to come at the end.
NEW_TESTS_ALOT = $(sort $(basename $(notdir $(wildcard testdir/test_alot_*.vim)))) test_alot
# test_eval_func is special: used as include in old-style test (test_eval.in).
NEW_TESTS = $(sort $(filter-out $(NEW_TESTS_ALOT) test_eval_func,$(basename $(notdir $(wildcard testdir/test_*.vim))))) $(NEW_TESTS_ALOT)
