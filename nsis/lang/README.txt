This directory contains a file with text strings for gVim installer.
It also contains files with translations of the text strings for gVim installer
into different languages.

For translators.

If you want to prepare a translation for the gVim installer, use the file
"english.nsi" as a master file.  See the other translation files in this
directory.
Note that in the definition of the MUI_LANGUAGE macro, the name of the language
to be translated must be the English name of the language.
The name of the file with the translation must match the name of the target
language.
Also, when translating strings, pay attention to some restrictions on the
allowable length of strings.  For example:
 component description field - 117 characters;
 description above the drop-down lists on the .vimrc page - 53 characters;
 drop-down lists on the .vimrc page - 55 characters.
Characters in this case mean characters of the English alphabet.

If you do not yet have a translated "LICENSE" file and/or a main "README.txt"
file, set the following values:

for the license file
LicenseLangString page_lic_file 0 "..\lang\LICENSE.nsis.txt"

for the readme.txt file
LangString vim_readme_file 0 "README.txt"

Once you have the translations of these files, then set the values for these
variables similarly to what is done in the other translation files.
Translation files should be located in the "lang" subdirectory of the root
directory. The name of the files is as follows: "README.xx.txt", where xx is the
language code according to ISO639.
