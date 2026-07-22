# GDB multiline commands
# VIM_TEST_SETUP setl fdc=2 fdl=999 fdm=syntax

define hello
  echo Hello, world!\n
end

document Foo
  Print a greeting.
end

commands
  echo Hello, world!\n
end

compile code
  printf("Hello, world!\n");
end

expression code
  printf("Hello, world!\n");
end

python
  print("Hello, world!\n")
end

guile
  (display "Hello, world!\n")
end

while 0
  echo Not this time\n
end

if 1
  echo Yes\n
else
  echo No\n
end

