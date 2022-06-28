The location of source files for Serbian spelling dictionary were downloaded
from https://github.com/LibreOffice/dictionaries/tree/master/sr (Serbian
Spelling and Hyphenation for LibreOffice).

Here is the content of original README file from the repository:

	"LibreOffice Spelling and Hyphenation
	extension package for Serbian (Cyrillic and Latin)
	
	This extension package includes the Hunspell dictionary and Hyphen
	hyphenation patterns for the Serbian language adapted for usage in
	LibreOffice.
	
	Serbian spelling dictionary is developed by Milutin Smiljanic
	<msmiljanic.gm@gmail.com> and is released under GNU LGPL version 3 or
	later / MPL version 2 or later / GNU GPL version 3 or later, giving
	you the choice of one of the three sets of free software licensing
	terms.
	
	Serbian hyphenation patterns are derived from the official TeX
	patterns for Serbocroatian language (Cyrillic and Latin) created by
	Dejan Muhamedagić, version 2.02 from 22 June 2008 adopted for usage
	with Hyphen hyphenation library and released under GNU LGPL version
	2.1 or later."


This dictionary used to create Vim spl file is the result of merging the two
LibreOffice dictionaries for cyrillic and latin script.

The merge was accomplished by concatenating two .dic and .aff files (appending
the latin to cyrillic).

The first step was to fix both .aff files by adding a '.' at the end of every
SFX and PFX directive and removing directives that are not supported by Vim
(KEY, MIDWORD).

Next, update the flags in latin .dic and .aff files so that the flag sequence
continues monotonically after the last flag number in cyrillic .aff file. 

A couple of words in cyrillic dict used a latin codepoints for 'a' and 'e',
that was also corrected.


Ivan Pešić
28.06.2022.
