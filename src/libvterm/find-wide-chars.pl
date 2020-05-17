#!/usr/bin/perl

use strict;
use warnings;

use Unicode::UCD qw( charprop );

STDOUT->autoflush(1);

sub iswide
{
   my ( $cp ) = @_;

   my $width = charprop( $cp, "East_Asian_Width" ) or return;
   return $width eq "Wide" || $width eq "Fullwidth";
}

my ( $start, $end );
foreach my $cp ( 0 .. 0x1FFFF ) {
   iswide($cp) or next;

   if( defined $end and $end == $cp-1 ) {
      # extend the range
      $end = $cp;
      next;
   }

   # start a new range
   printf "  { %#04x, %#04x },\n", $start, $end if defined $start;

   $start = $end = $cp;
}

printf "  { %#04x, %#04x },\n", $start, $end if defined $start;
