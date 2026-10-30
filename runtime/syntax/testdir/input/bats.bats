#!/usr/bin/env bats
# VIM_TEST_SETUP setlocal fen fdc=2 fdl=8 fdm=syntax
# VIM_TEST_SETUP let g:sh_fold_enabled = 1
# bats file_tags=slow
# bats is not a tag comment

bats_require_minimum_version 1.5.0

setup() {
  load test_helper
  bats_load_library bats-assert
}

# bats test_tags=fast, os:linux
@test "addition { works" {
  run -1 --separate-stderr false
  [ "$status" -eq 1 ]
  if true; then
    skip "not yet"
  fi
}

@test unquoted name { run true; }

@test 'single $quoted' {
  run ! bats_pipe echo test \| grep -q skip
}

@test "expanded $BATS_TEST_FILENAME" {
  bats_load_safe helper
}

echo @test run skip load
