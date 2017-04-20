#!/usr/bin/perl -w
#
# This script generates the tables cmdidxs1[] and cmdidxs2[][] which,
# given a Ex command, determine the first value to probe to find
# a matching command in cmdnames[] based on the first character
# and the first 2 characters of the command.
# This is used to speed up lookup in cmdnames[].
#
# Script should be run every time new Ex commands are added in Vim,
# from the src/vim directory, since it reads commands from "ex_cmds.h".

use strict;

# Find the list of Vim commands from cmdnames[] table in ex_cmds.h
my @cmds;
my $skipped_cmds;
open(IN, "< ex_cmds.h") or die "can't open ex_cmds.h: $!\n";
while (<IN>) {
  if (/^EX\(CMD_\S*,\s*"([a-z][^"]*)"/) {
    push @cmds, $1;
  } elsif (/^EX\(CMD_/) {
    ++$skipped_cmds;
  }
}

my %cmdidxs1;
my %cmdidxs2;

for (my $i = $#cmds; $i >= 0; --$i) {
  my $cmd = $cmds[$i];
  my $c1 = substr($cmd, 0, 1); # First character of command.

  $cmdidxs1{$c1} = $i;

  if (length($cmd) > 1) {
    my $c2 = substr($cmd, 1, 1); # Second character of command.
    $cmdidxs2{$c1}{$c2} = $i if (('a' lt $c2) and ($c2 lt 'z'));
  }
}

print "/* Beginning of automatically generated code by create_cmdidxs.pl\n",
      " *\n",
      " * Table giving the index of the first command in cmdnames[] to lookup\n",
      " * based on the first letter of a command.\n",
      " */\n",
      "static const unsigned short cmdidxs1[26] =\n{\n",
      join(",\n", map("  /* $_ */ $cmdidxs1{$_}", ('a' .. 'z'))),
      "\n};\n",
      "\n",
      "/*\n",
      " * Table giving the index of the first command in cmdnames[] to lookup\n",
      " * based on the first 2 letters of a command.\n",
      " * Values in cmdidxs2[c1][c2] are relative to cmdidxs1[c1] so that they\n",
      " * fit in a byte.\n",
      " */\n",
      "static const unsigned char cmdidxs2[26][26] =\n",
      "{ /*         a   b   c   d   e   f   g   h   i   j   k   l   m   n   o   p   q   r   s   t   u   v   w   x   y   z */\n";
for my $c1 ('a' .. 'z') {
  print "  /* $c1 */ {";
  for my $c2 ('a' .. 'z') {
    if (exists $cmdidxs2{$c1}{$c2}) {
      printf "%3d,", $cmdidxs2{$c1}{$c2} - $cmdidxs1{$c1};
    } else {
      printf "  0,";
    }
  }
  print " }";
  print "," unless ($c1 eq 'z');
  print "\n";
}
print "};\n",
      "\n",
      "static const int command_count = ", scalar(@cmds) + $skipped_cmds, ";\n",
      "\n",
      "/* End of automatically generated code by create_cmdidxs.pl */\n";
