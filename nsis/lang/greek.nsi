# vi:set ts=8 sts=4 sw=4 et fdm=marker:
#
# greek.nsi: Greek language strings for gvim NSIS installer.
#
# Locale ID    : 1032
# Locale Name  : el
# fileencoding : UTF-8
# Author       : Christos Longros

!insertmacro MUI_LANGUAGE "Greek"


# Overwrite the default translation.
# These strings should be always English.  Otherwise dosinst.c fails.
LangString ^SetupCaption     ${LANG_GREEK} \
        "$(^Name) Setup"
LangString ^UninstallCaption ${LANG_GREEK} \
        "$(^Name) Uninstall"

##############################################################################
# Translated license file for the license page                            {{{1
##############################################################################

LicenseLangString page_lic_file 0 "..\lang\LICENSE.nsis.txt"
#LicenseLangString page_lic_file ${LANG_GREEK} "..\lang\LICENSE.el.nsis.txt"

##############################################################################
# Translated README.txt file, which is opened after installation          {{{1
##############################################################################

LangString vim_readme_file 0 "README.txt"
#LangString vim_readme_file ${LANG_GREEK} "README.el.txt"

##############################################################################
# MUI Configuration Strings                                               {{{1
##############################################################################

#LangString str_dest_folder          ${LANG_GREEK} \
#    "Φάκελος προορισμός (Πρέπει να τελειώνει σε $\"vim$\")"

LangString str_show_readme          ${LANG_GREEK} \
    "Εμφάνιση README μετά την ολοκλήρωση της εγκατάστασης"

# Install types:
LangString str_type_typical         ${LANG_GREEK} \
    "Typical"

LangString str_type_minimal         ${LANG_GREEK} \
    "Minimal"

LangString str_type_full            ${LANG_GREEK} \
    "Full"


##############################################################################
# Section Titles & Description                                            {{{1
##############################################################################

LangString str_section_old_ver      ${LANG_GREEK} \
    "Απεγκατάσταση υπάρχουσων εκδόσεων"
LangString str_desc_old_ver         ${LANG_GREEK} \
    "Απεγκατάσταση υπάρχουσων εκδόσεων Vim από το σύστημά σας."

LangString str_section_exe          ${LANG_GREEK} \
    "Vim GUI and runtime files"
LangString str_desc_exe             ${LANG_GREEK} \
    "Vim GUI executables and runtime files.  This component is required."

LangString str_section_console      ${LANG_GREEK} \
    "Vim console program"
LangString str_desc_console         ${LANG_GREEK} \
    "Console version of Vim (vim.exe)."

LangString str_section_batch        ${LANG_GREEK} \
    "Δημιουργία αρχείων .bat"
LangString str_desc_batch           ${LANG_GREEK} \
    "Δημιουργία αρχείων .bat από παράγωγα Vim στον κατάλογο των Windows για \
     χρήση γραμμής εντολών."

LangString str_group_icons          ${LANG_GREEK} \
    "Δημιουργία  εικονιδίων Vim"
LangString str_desc_icons           ${LANG_GREEK} \
    "Δημιουργία εικονιδίων για τον Vim σε διάφορες τοποθεσίες για την διευκόλυνση της προσβασιμότητας."

LangString str_section_desktop      ${LANG_GREEK} \
    "Στην επιφάνεια εργασίας"
LangString str_desc_desktop         ${LANG_GREEK} \
    "Δημιουργία εικονιδίων για gVim εκτελέσιμα στην επιφάνεια εργασίας."

LangString str_section_start_menu   ${LANG_GREEK} \
    "In the Start Menu Programs Folder"
LangString str_desc_start_menu      ${LANG_GREEK} \
    "Προσθήκη του Vim στον φάκελο προγραμμάτων του μενού εκκίνησης."

#LangString str_section_quick_launch ${LANG_GREEK} \
#    "In the Quick Launch Bar"
#LangString str_desc_quick_launch    ${LANG_GREEK} \
#    "Add Vim shortcut in the quick launch bar."

LangString str_section_edit_with    ${LANG_GREEK} \
    "Add Vim Context Menu"
LangString str_desc_edit_with       ${LANG_GREEK} \
    "Add Vim to the $\"Open With...$\" context menu list."

#LangString str_section_edit_with32  ${LANG_GREEK} \
#    "32-bit Version"
#LangString str_desc_edit_with32     ${LANG_GREEK} \
#    "Add Vim to the $\"Open With...$\" context menu list \
#     for 32-bit applications."

#LangString str_section_edit_with64  ${LANG_GREEK} \
#    "64-bit Version"
#LangString str_desc_edit_with64     ${LANG_GREEK} \
#    "Add Vim to the $\"Open With...$\" context menu list \
#     for 64-bit applications."

LangString str_section_vim_rc       ${LANG_GREEK} \
    "Δημιουργία προεπιλεγμένων ρυθμίσεων"
LangString str_desc_vim_rc          ${LANG_GREEK} \
    "Δημιουργία προεπιλεγμένου αρχείου ρυθμίσεων (_vimrc) αν δεν υπάρχει ήδη."

LangString str_group_plugin         ${LANG_GREEK} \
    "Δημιουργία καταλόγων Plugin"
LangString str_desc_plugin          ${LANG_GREEK} \
    "Δημιουργία καταλόγων plugin.  Οι κατάλογοι Plugin επιτρέπουν την επέκταση του Vim \
     με την μεταφορά ενός αρχείου σε έναν κατάλογο."

LangString str_section_plugin_home  ${LANG_GREEK} \
    "Private"
LangString str_desc_plugin_home     ${LANG_GREEK} \
    "Δημιουργία καταλόγων plugin στον κατάλογο HOME."

LangString str_section_plugin_vim   ${LANG_GREEK} \
    "Shared"
LangString str_desc_plugin_vim      ${LANG_GREEK} \
    "Δημιουργία καταλόγων plugin στον κατάλογο εγκατάστασης του Vim, χρησιμοποιείται από \
     όλους στο σύστημα."

LangString str_section_nls          ${LANG_GREEK} \
    "Native Language Support"
LangString str_desc_nls             ${LANG_GREEK} \
    "Install files for native language support."

LangString str_unsection_register   ${LANG_GREEK} \
    "Unregister Vim"
LangString str_desc_unregister      ${LANG_GREEK} \
    "Unregister Vim from the system."

LangString str_unsection_exe        ${LANG_GREEK} \
    "Remove Vim Executables/Runtime Files"
LangString str_desc_rm_exe          ${LANG_GREEK} \
    "Remove all Vim executables and runtime files."

LangString str_ungroup_plugin       ${LANG_GREEK} \
    "Remove plugin directories"
LangString str_desc_rm_plugin       ${LANG_GREEK} \
    "Remove the plugin directories if they are empty."

LangString str_unsection_plugin_home ${LANG_GREEK} \
    "Private"
LangString str_desc_rm_plugin_home  ${LANG_GREEK} \
    "Remove the plugin directories from HOME directory."

LangString str_unsection_plugin_vim ${LANG_GREEK} \
    "Shared"
LangString str_desc_rm_plugin_vim   ${LANG_GREEK} \
    "Remove the plugin directories from Vim install directory."

LangString str_unsection_rootdir    ${LANG_GREEK} \
    "Remove the Vim root directory"
LangString str_desc_rm_rootdir      ${LANG_GREEK} \
    "Remove the Vim root directory. It contains your Vim configuration files!"


##############################################################################
# Messages                                                                {{{1
##############################################################################

#LangString str_msg_too_many_ver  ${LANG_GREEK} \
#    "Found $vim_old_ver_count Vim versions on your system.$\r$\n\
#     This installer can only handle ${VIM_MAX_OLD_VER} versions \
#     at most.$\r$\n\
#     Please remove some versions and start again."

#LangString str_msg_invalid_root  ${LANG_GREEK} \
#    "Invalid install path: $vim_install_root!$\r$\n\
#     It should end with $\"vim$\"."

#LangString str_msg_bin_mismatch  ${LANG_GREEK} \
#    "Binary path mismatch!$\r$\n$\r$\n\
#     Expect the binary path to be $\"$vim_bin_path$\",$\r$\n\
#     but system indicates the binary path is $\"$INSTDIR$\"."

#LangString str_msg_vim_running   ${LANG_GREEK} \
#    "Vim is still running on your system.$\r$\n\
#     Please close all instances of Vim before you continue."

#LangString str_msg_register_ole  ${LANG_GREEK} \
#    "Attempting to register Vim with OLE. \
#     There is no message indicates whether this works or not."

#LangString str_msg_unreg_ole     ${LANG_GREEK} \
#    "Attempting to unregister Vim with OLE. \
#     There is no message indicates whether this works or not."

#LangString str_msg_rm_start      ${LANG_GREEK} \
#    "Uninstalling the following version:"

#LangString str_msg_rm_fail       ${LANG_GREEK} \
#    "Fail to uninstall the following version:"

#LangString str_msg_no_rm_key     ${LANG_GREEK} \
#    "Cannot find uninstaller registry key."

#LangString str_msg_no_rm_reg     ${LANG_GREEK} \
#    "Cannot find uninstaller from registry."

#LangString str_msg_no_rm_exe     ${LANG_GREEK} \
#    "Cannot access uninstaller."

#LangString str_msg_rm_copy_fail  ${LANG_GREEK} \
#    "Fail to copy uninstaller to temporary directory."

#LangString str_msg_rm_run_fail   ${LANG_GREEK} \
#    "Αποτυχία εκτέλεσης της λειτουργίας απεγκατάστασης."

#LangString str_msg_abort_install ${LANG_GREEK} \
#    "Installer will abort."

LangString str_msg_install_fail  ${LANG_GREEK} \
    "Η εγκατάσταση απέτυχε. Better luck next time."

LangString str_msg_rm_exe_fail   ${LANG_GREEK} \
    "Μερικά αρχεία στο $0 δεν έχουν διαγραφεί!$\r$\n\
     Πρέπει να το κάνετε χειροκίνητα."

#LangString str_msg_rm_root_fail  ${LANG_GREEK} \
#    "ΠΡΟΕΙΔΟΠΟΙΗΣΗ: Αδύνατη η αφαίρεση $\"$vim_install_root$\", δεν είναι κενό!"

LangString str_msg_uninstalling  ${LANG_GREEK} \
    "Απεγκατάσταση παλιάς έκδοσης the old version..."

LangString str_msg_registering   ${LANG_GREEK} \
    "Καταχώρηση..."

LangString str_msg_unregistering ${LANG_GREEK} \
    "Unregistering..."


##############################################################################
# Dialog Box                                                              {{{1
##############################################################################

LangString str_vimrc_page_title    ${LANG_GREEK} \
    "Επιλογή ρυθμίσεων _vimrc"
LangString str_vimrc_page_subtitle ${LANG_GREEK} \
    "Choose the settings for enhancement, keyboard and mouse."

LangString str_msg_compat_title    ${LANG_GREEK} \
    " Συμπεριφορά Vi / Vim  "
LangString str_msg_compat_desc     ${LANG_GREEK} \
    "&Compatibility and enhancements"
LangString str_msg_compat_vi       ${LANG_GREEK} \
    "Vi compatible"
LangString str_msg_compat_vim      ${LANG_GREEK} \
    "Vim original"
LangString str_msg_compat_defaults ${LANG_GREEK} \
    "Vim with some enhancements (load defaults.vim)"
LangString str_msg_compat_all      ${LANG_GREEK} \
    "Vim with all enhancements (load vimrc_example.vim) (Default)"

LangString str_msg_keymap_title   ${LANG_GREEK} \
    " Mappings "
LangString str_msg_keymap_desc    ${LANG_GREEK} \
    "&Remap a few keys for Windows (Ctrl-V, Ctrl-C, Ctrl-A, Ctrl-S, Ctrl-F, etc)"
LangString str_msg_keymap_default ${LANG_GREEK} \
    "Do not remap keys (Default)"
LangString str_msg_keymap_windows ${LANG_GREEK} \
    "Remap a few keys"

LangString str_msg_mouse_title   ${LANG_GREEK} \
    " Ποντίκι "
LangString str_msg_mouse_desc    ${LANG_GREEK} \
    "&Behavior of right and left buttons"
LangString str_msg_mouse_default ${LANG_GREEK} \
    "Right: popup menu, Left: visual mode (Default)"
LangString str_msg_mouse_windows ${LANG_GREEK} \
    "Right: popup menu, Left: select mode (Windows)"
LangString str_msg_mouse_unix    ${LANG_GREEK} \
    "Right: extends selection, Left: visual mode (Unix)"

