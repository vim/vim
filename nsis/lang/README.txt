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

Once the message translation file is ready, it must be included in the
"gvim.nsi" file.
Find the line "# Include support for other languages:" in the file "gvim.nsi"
and specify the name of the file with your translation below the line
!if ${HAVE_MULTI_LANG}, similar to the entries already there. File names are
specified in alphabetical order.

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


There are two ways to test the installer in different languages:

1. Find and uncomment the "!define MUI_LANGDLL_ALWAYSSHOW" line in the
   "gvim.nsi" file and rebuild the installer.
   Now every time you run it, you will see a dialog box with the possibility to
   select the language of the installer.

2. If the Vim editor is already installed in your system, delete the
   "Installer Language" parameter in the Windows registry under
   "HKEY_CURRENT_USER\Software\Vim".
   Or you can create a file "NoLangInstallerVim.reg" with the following content:

	Windows Registry Editor Version 5.00

	[HKEY_CURRENT_USER\Software\Vim]
	"Installer Language"=-

   and apply it by double-clicking on it.
   After these steps, when you start the installer, a window with the installer
   language selection will also be displayed. 
