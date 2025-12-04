# vi:set ts=8 sts=4 sw=4 et fdm=marker:
#
# swedish.nsi: Svenska strängar för gvim NSIS-installationsprogram.
#
# Lokalisering-ID    : 1053
# Lokalt namn  : sv
# filkodning : UTF-8
# Översättare       : Daniel Nylander

!insertmacro MUI_LANGUAGE "Swedish"


# Skriv över standardöversättningen.
# Dessa strängar ska alltid vara på engelska.  Annars misslyckas dosinst.c.
LangString ^SetupCaption     ${LANG_SWEDISH} \
        "$(^Name) Setup"
LangString ^UninstallCaption ${LANG_SWEDISH} \
        "$(^Name) Uninstall"

##############################################################################
# Licensfil för licenssidan                                       {{{1
##############################################################################

LicenseLangString page_lic_file ${LANG_SWEDISH} "..\lang\LICENSE.sv.nsis.txt"

##############################################################################
# README.txt-fil, som öppnas efter installationen                     {{{1
##############################################################################

LangString vim_readme_file ${LANG_SWEDISH} "README.sv.txt"

##############################################################################
# MUI-konfigurationssträngar                                               {{{1
##############################################################################

#LangString str_dest_folder          ${LANG_SWEDISH} \
#    "Målmapp (måste sluta med $\"vim$\")"

LangString str_show_readme          ${LANG_SWEDISH} \
    "Visa README efter avslutad installation"

# Installationstyper:
LangString str_type_typical         ${LANG_SWEDISH} \
    "Typisk"

LangString str_type_minimal         ${LANG_SWEDISH} \
    "Minimal"

LangString str_type_full            ${LANG_SWEDISH} \
    "Fullständig"


##############################################################################
# Avsnittstitlar och beskrivning                                            {{{1
##############################################################################

LangString str_section_old_ver      ${LANG_SWEDISH} \
    "Avinstallera befintliga versioner"
LangString str_desc_old_ver         ${LANG_SWEDISH} \
    "Avinstallera befintliga Vim-versioner från ditt system."

LangString str_section_exe          ${LANG_SWEDISH} \
    "Vim GUI och runtime-filer"
LangString str_desc_exe             ${LANG_SWEDISH} \
    "Vim GUI-körbara filer och runtime-filer.  Denna komponent krävs."

LangString str_section_console      ${LANG_SWEDISH} \
    "Vim-konsolprogram"
LangString str_desc_console         ${LANG_SWEDISH} \
    "Konsolversion av Vim (vim.exe)."

LangString str_section_batch        ${LANG_SWEDISH} \
    "Skapa .bat-filer"
LangString str_desc_batch           ${LANG_SWEDISH} \
    "Skapa .bat-filer för Vim-varianter i Windows-katalogen för \
     kommandoradsanvändning."

LangString str_group_icons          ${LANG_SWEDISH} \
    "Skapa ikoner för Vim"
LangString str_desc_icons           ${LANG_SWEDISH} \
    "Skapa ikoner för Vim på olika platser för att underlätta åtkomsten."

LangString str_section_desktop      ${LANG_SWEDISH} \
    "På skrivbordet"
LangString str_desc_desktop         ${LANG_SWEDISH} \
    "Skapa ikoner för gVim-körbara filer på skrivbordet."

LangString str_section_start_menu   ${LANG_SWEDISH} \
    "I startmenyns programmapp"
LangString str_desc_start_menu      ${LANG_SWEDISH} \
    "Lägg till Vim i programmappen i startmenyn."

#LangString str_section_quick_launch ${LANG_SWEDISH} \
#    "I snabbstartfältet"
#LangString str_desc_quick_launch    ${LANG_SWEDISH} \
#    "Lägg till Vim-genväg i snabbstartfältet."

LangString str_section_edit_with    ${LANG_SWEDISH} \
    "Lägg till Vim-kontextmeny"
LangString str_desc_edit_with       ${LANG_SWEDISH} \
    "Lägg till Vim i listan över snabbmenyn $\"Öppna med...$\"."

#LangString str_section_edit_with32  ${LANG_SWEDISH} \
#    "32-bitarsversion"
#LangString str_desc_edit_with32     ${LANG_SWEDISH} \
#    "Lägg till Vim i listan över snabbmenyn $\"Öppna med...$\" \
#     för 32-bitarsprogram."

#LangString str_section_edit_with64  ${LANG_SWEDISH} \
#    "64-bitarsversion"
#LangString str_desc_edit_with64     ${LANG_SWEDISH} \
#    "Lägg till Vim i listan över kontextmenyn $\"Öppna med...$\" \
#     för 64-bitarsprogram."

LangString str_section_vim_rc       ${LANG_SWEDISH} \
    "Skapa standardkonfiguration"
LangString str_desc_vim_rc          ${LANG_SWEDISH} \
    "Skapa en standardkonfigurationsfil (_vimrc) om det inte redan finns en."

LangString str_group_plugin         ${LANG_SWEDISH} \
    "Skapa plugin-kataloger"
LangString str_desc_plugin          ${LANG_SWEDISH} \
    "Skapa plugin-kataloger.  Plugin-kataloger gör det möjligt att utöka Vim \
     genom att släppa en fil i en katalog."

LangString str_section_plugin_home  ${LANG_SWEDISH} \
    "Privat"
LangString str_desc_plugin_home     ${LANG_SWEDISH} \
    "Skapa plugin-kataloger i HOME-katalogen."

LangString str_section_plugin_vim   ${LANG_SWEDISH} \
    "Delad"
LangString str_desc_plugin_vim      ${LANG_SWEDISH} \
    "Skapa plugin-kataloger i Vim-installationskatalogen, den används för \
     alla på systemet."

LangString str_section_nls          ${LANG_SWEDISH} \
    "Stöd för modersmål"
LangString str_desc_nls             ${LANG_SWEDISH} \
    "Installera filer för stöd för modersmål."

LangString str_unsection_register   ${LANG_SWEDISH} \
    "Avregistrera Vim"
LangString str_desc_unregister      ${LANG_SWEDISH} \
    "Avregistrera Vim från systemet."

LangString str_unsection_exe        ${LANG_SWEDISH} \
    "Ta bort Vim-körbara filer/körningsfiler"
LangString str_desc_rm_exe          ${LANG_SWEDISH} \
    "Ta bort alla Vim-körbara filer och runtime-filer."

LangString str_ungroup_plugin       ${LANG_SWEDISH} \
    "Ta bort plugin-kataloger"
LangString str_desc_rm_plugin       ${LANG_SWEDISH} \
    "Ta bort plugin-katalogerna om de är tomma."

LangString str_unsection_plugin_home ${LANG_SWEDISH} \
    "Privat"
LangString str_desc_rm_plugin_home  ${LANG_SWEDISH} \
    "Ta bort plugin-katalogerna från HOME-katalogen."

LangString str_unsection_plugin_vim ${LANG_SWEDISH} \
    "Delad"
LangString str_desc_rm_plugin_vim   ${LANG_SWEDISH} \
    "Ta bort plugin-katalogerna från Vim-installationskatalogen."

LangString str_unsection_rootdir    ${LANG_SWEDISH} \
    "Ta bort Vim-rotkatalogen"
LangString str_desc_rm_rootdir      ${LANG_SWEDISH} \
    "Ta bort Vim-rotkatalogen. Den innehåller dina Vim-konfigurationsfiler!"


##############################################################################
# Meddelanden                                                                {{{1
##############################################################################

#LangString str_msg_too_many_ver  ${LANG_SWEDISH} \
#    "Hittade $vim_old_ver_count Vim-versioner på ditt system.$\r$\n\
#     Denna installationsprogram kan endast hantera ${VIM_MAX_OLD_VER} versioner \
#    .$\r$\n\
#     Ta bort några versioner och börja om."

#LangString str_msg_invalid_root  ${LANG_SWEDISH} \
#    "Ogiltig installationsväg: $vim_install_root!$\r$\n\
#     Den ska sluta med $\"vim$\"."

#LangString str_msg_bin_mismatch  ${LANG_SWEDISH} \
#    "Binär sökväg stämmer inte!$\r$\n$\r$\n\
#     Förväntar mig att binärvägen är $\"$vim_bin_path$\",$\r$\n\
#     men systemet anger att binärvägen är $\"$INSTDIR$\"."

#LangString str_msg_vim_running   ${LANG_SWEDISH} \
#    "Vim körs fortfarande på ditt system.$\r$\n\
#     Stäng alla instanser av Vim innan du fortsätter."

#LangString str_msg_register_ole  ${LANG_SWEDISH} \
#    "Försöker registrera Vim med OLE. \
#     Det finns inget meddelande som anger om detta fungerar eller inte."

#LangString str_msg_unreg_ole     ${LANG_SWEDISH} \
#    "Försöker avregistrera Vim från OLE. \
#     Det finns inget meddelande som anger om detta fungerar eller inte."

#LangString str_msg_rm_start      ${LANG_SWEDISH} \
#    "Avinstallera följande version:"

#LangString str_msg_rm_fail       ${LANG_SWEDISH} \
#    "Det gick inte att avinstallera följande version:"

#LangString str_msg_no_rm_key     ${LANG_SWEDISH} \
#    "Kan inte hitta avinstallationsnyckeln i registret."

#LangString str_msg_no_rm_reg     ${LANG_SWEDISH} \
#    "Kan inte hitta avinstallationsprogrammet i registret."

#LangString str_msg_no_rm_exe     ${LANG_SWEDISH} \
#    "Kan inte komma åt avinstallationsprogrammet."

#LangString str_msg_rm_copy_fail  ${LANG_SWEDISH} \
#    "Det gick inte att kopiera avinstallationsprogrammet till den tillfälliga katalogen."

#LangString str_msg_rm_run_fail   ${LANG_SWEDISH} \
#    "Det gick inte att köra avinstallationsprogrammet."

#LangString str_msg_abort_install ${LANG_SWEDISH} \
#    "Installationsprogrammet avbryts."

LangString str_msg_install_fail  ${LANG_SWEDISH} \
    "Installationen misslyckades. Bättre lycka nästa gång."

LangString str_msg_rm_exe_fail   ${LANG_SWEDISH} \
    "Vissa filer i $0 har inte raderats!$\r$\n\
     Du måste göra det manuellt."

#LangString str_msg_rm_root_fail  ${LANG_SWEDISH} \
#    "VARNING: Kan inte ta bort $\"$vim_install_root$\", den är inte tom!"

LangString str_msg_uninstalling  ${LANG_SWEDISH} \
    "Avinstallerar den gamla versionen..."

LangString str_msg_registering   ${LANG_SWEDISH} \
    "Registrerar..."

LangString str_msg_unregistering ${LANG_SWEDISH} \
    "Avregistrerar..."


##############################################################################
# Dialogruta                                                              {{{1
##############################################################################

LangString str_vimrc_page_title    ${LANG_SWEDISH} \
    "Välj _vimrc-inställningar"
LangString str_vimrc_page_subtitle ${LANG_SWEDISH} \
    "Välj inställningar för förbättringar, tangentbord och mus."

LangString str_msg_compat_title    ${LANG_SWEDISH} \
    " Vi / Vim-beteende "
LangString str_msg_compat_desc     ${LANG_SWEDISH} \
    "&Kompatibilitet och förbättringar"
LangString str_msg_compat_vi       ${LANG_SWEDISH} \
    "Vi-kompatibel"
LangString str_msg_compat_vim      ${LANG_SWEDISH} \
    "Vim original"
LangString str_msg_compat_defaults ${LANG_SWEDISH} \
    "Vim med vissa förbättringar (ladda defaults.vim)"
LangString str_msg_compat_all      ${LANG_SWEDISH} \
    "Vim med alla förbättringar (ladda vimrc_example.vim) (Standard)"

LangString str_msg_keymap_title   ${LANG_SWEDISH} \
    " Mappningar "
LangString str_msg_keymap_desc    ${LANG_SWEDISH} \
    "&Omkonfigurera några tangenter för Windows (Ctrl-V, Ctrl-C, Ctrl-A, Ctrl-S, Ctrl-F, etc)"
LangString str_msg_keymap_default ${LANG_SWEDISH} \
    "Omkonfigurera inte tangenter (standard)"
LangString str_msg_keymap_windows ${LANG_SWEDISH} \
    "Omkonfigurera några tangenter"

LangString str_msg_mouse_title   ${LANG_SWEDISH} \
    " Mus "
LangString str_msg_mouse_desc    ${LANG_SWEDISH} \
    "&Funktion för höger- och vänsterknapp"
LangString str_msg_mouse_default ${LANG_SWEDISH} \
    "Höger: popup-meny, Vänster: visuellt läge (Standard)"
LangString str_msg_mouse_windows ${LANG_SWEDISH} \
    "Höger: popup-meny, Vänster: välj läge (Windows)"
LangString str_msg_mouse_unix    ${LANG_SWEDISH} \
    "Höger: utökar valet, Vänster: visuellt läge (Unix)"
