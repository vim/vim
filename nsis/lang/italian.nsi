# vi:set ts=8 sts=4 sw=4 et fdm=marker:
#
# italian.nsi : Italian language strings for gvim NSIS installer.
#
# Locale ID    : 1040
# Locale Name  : it
# fileencoding : UTF-8
# Author       : Antonio Colombo

!insertmacro MUI_LANGUAGE "Italian"


# Overwrite the default translation.
# These strings should be always English.  Otherwise dosinst.c fails.
LangString ^SetupCaption     ${LANG_ITALIAN} \
        "$(^Name) Setup"
LangString ^UninstallCaption ${LANG_ITALIAN} \
        "$(^Name) Uninstall"

##############################################################################
# MUI Configuration Strings                                               {{{1
##############################################################################

#LangString str_dest_folder          ${LANG_ITALIAN} \
#    "Cartella d'installazione (il nome deve finire con $\"vim$\")"

LangString str_show_readme          ${LANG_ITALIAN} \
    "Visualizza README al termine dell'installazione"

# Install types:
LangString str_type_typical         ${LANG_ITALIAN} \
    "Tipica"

LangString str_type_minimal         ${LANG_ITALIAN} \
    "Minima"

LangString str_type_full            ${LANG_ITALIAN} \
    "Completa"


##############################################################################
# Section Titles & Description                                            {{{1
##############################################################################

LangString str_section_old_ver      ${LANG_ITALIAN} \
    "Disinstalla versione/i esistente/i"
LangString str_desc_old_ver         ${LANG_ITALIAN} \
    "Disinstalla versione/i esistente/i di Vim dal vostro sistema."

LangString str_section_exe          ${LANG_ITALIAN} \
    "Vim GUI e file di supporto"
LangString str_desc_exe             ${LANG_ITALIAN} \
    "Vim GUI programmi e file di supporto.  Questa componente è indispensabile."

LangString str_section_console      ${LANG_ITALIAN} \
    "Vim console (vim.exe per MS-DOS)"
LangString str_desc_console         ${LANG_ITALIAN} \
    "Versione console di Vim (vim.exe)."

LangString str_section_batch        ${LANG_ITALIAN} \
    "Crea file di invocazione (MS-DOS) .bat"
LangString str_desc_batch           ${LANG_ITALIAN} \
    "Crea file di invocazione .bat per varianti di Vim nella directory \
     di Windows, per utilizzo da linea di comando (MS-DOS)."

LangString str_group_icons          ${LANG_ITALIAN} \
    "Crea icone per Vim"
LangString str_desc_icons           ${LANG_ITALIAN} \
    "Crea icone per Vim in vari posti, per rendere facile l'accesso."

LangString str_section_desktop      ${LANG_ITALIAN} \
    "Sul Desktop"
LangString str_desc_desktop         ${LANG_ITALIAN} \
    "Crea icone per programma gVim sul desktop."

LangString str_section_start_menu   ${LANG_ITALIAN} \
    "Nella cartella del menù START"
LangString str_desc_start_menu      ${LANG_ITALIAN} \
    "Aggiungi Vim alle cartelle del menù START."

#LangString str_section_quick_launch ${LANG_ITALIAN} \
#    "Nella barra di Avvio Veloce"
#LangString str_desc_quick_launch    ${LANG_ITALIAN} \
#    "Aggiungi un puntatore a Vim nella barra di Avvio Veloce."

LangString str_section_edit_with    ${LANG_ITALIAN} \
    "Aggiungi Vim al Menù Contestuale"
LangString str_desc_edit_with       ${LANG_ITALIAN} \
    "Aggiungi Vim alla lista contestuale $\"Apri con...$\"."

#LangString str_section_edit_with32  ${LANG_ITALIAN} \
#    "Versione a 32-bit"
#LangString str_desc_edit_with32     ${LANG_ITALIAN} \
#    "Aggiungi Vim alla lista contestuale $\"Apri con...$\" \
#     per applicazioni a 32-bit."

#LangString str_section_edit_with64  ${LANG_ITALIAN} \
#    "Versione a 64-bit"
#LangString str_desc_edit_with64     ${LANG_ITALIAN} \
#    "Aggiungi Vim alla lista contestuale $\"Apri con...$\" \
#     per applicazioni a 64-bit."

LangString str_section_vim_rc       ${LANG_ITALIAN} \
    "Crea configurazione di default"
LangString str_desc_vim_rc          ${LANG_ITALIAN} \
    "Crea un file configurazione di default (_vimrc) se non \
     ne esiste già uno."

LangString str_group_plugin         ${LANG_ITALIAN} \
    "Crea directory per plugin"
LangString str_desc_plugin          ${LANG_ITALIAN} \
    "Crea directory per plugin.  Consentono di aggiungere funzionalità \
     a Vim mettendo file in una di queste directory."

LangString str_section_plugin_home  ${LANG_ITALIAN} \
    "Private"
LangString str_desc_plugin_home     ${LANG_ITALIAN} \
    "Crea directory per plugin nella directory HOME."

LangString str_section_plugin_vim   ${LANG_ITALIAN} \
    "Condivise"
LangString str_desc_plugin_vim      ${LANG_ITALIAN} \
    "Crea directory per plugin nella directory di installazione di Vim \
     per uso da parte di tutti gli utenti di questo sistema."

LangString str_section_vis_vim      ${LANG_ITALIAN} \
    "Estensione VisVim"
LangString str_desc_vis_vim         ${LANG_ITALIAN} \
    "Estensione VisVim per integrazione con Microsoft Visual Studio."

LangString str_section_nls          ${LANG_ITALIAN} \
    "Supporto Multilingue (NLS)"
LangString str_desc_nls             ${LANG_ITALIAN} \
    "Installa file per supportare messaggi in diverse lingue."

LangString str_unsection_register   ${LANG_ITALIAN} \
    "Togli Vim dal Registry"
LangString str_desc_unregister      ${LANG_ITALIAN} \
    "Togli Vim dal Registry di configurazione sistema."

LangString str_unsection_exe        ${LANG_ITALIAN} \
    "Cancella programmi/file di supporto Vim"
LangString str_desc_rm_exe          ${LANG_ITALIAN} \
    "Cancella tutti i programmi/file di supporto di Vim."

LangString str_ungroup_plugin       ${LANG_ITALIAN} \
    "Cancella le directory per plugin"
LangString str_desc_rm_plugin       ${LANG_ITALIAN} \
    "Cancella le directory per plugin se sono vuote."

LangString str_unsection_plugin_home ${LANG_ITALIAN} \
    "Private"
LangString str_desc_rm_plugin_home  ${LANG_ITALIAN} \
    "Cancella le directory per plugin dalla directory HOME."

LangString str_unsection_plugin_vim ${LANG_ITALIAN} \
    "Condivise"
LangString str_desc_rm_plugin_vim   ${LANG_ITALIAN} \
    "Cancella le directory per plugin dalla directory di installazione di Vim."

LangString str_unsection_rootdir    ${LANG_ITALIAN} \
    "Cancella la directory di installazione di Vim"
LangString str_desc_rm_rootdir      ${LANG_ITALIAN} \
    "Cancella la directory di installazione di Vim. Contiene i vostri file di configurazione!"


##############################################################################
# Messages                                                                {{{1
##############################################################################

#LangString str_msg_too_many_ver  ${LANG_ITALIAN} \
#    "Trovate $vim_old_ver_count versioni di Vim sul vostro sistema.$\r$\n\
#     Questo programma di installazione può gestire solo \
#     ${VIM_MAX_OLD_VER} versioni.$\r$\n\
#     Disinstallate qualche versione precedente e ricominciate."

#LangString str_msg_invalid_root  ${LANG_ITALIAN} \
#    "Nome di directory di installazione non valida: $vim_install_root!$\r$\n\
#     Dovrebbe terminare con $\"vim$\"."

#LangString str_msg_bin_mismatch  ${LANG_ITALIAN} \
#    "Conflitto nella directory di installazione!$\r$\n$\r$\n\
#     Cartella di installazione dev'essere $\"$vim_bin_path$\",$\r$\n\
#     ma il sistema segnala invece $\"$INSTDIR$\"."

#LangString str_msg_vim_running   ${LANG_ITALIAN} \
#    "Vim ancora in esecuzione sul vostro sistema.$\r$\n\
#     Chiudete tutte le sessioni attive di Vim per continuare."

#LangString str_msg_register_ole  ${LANG_ITALIAN} \
#    "Tentativo di registrazione di Vim con OLE. \
#     Non c'è messaggio che indica se è riuscito o no."

#LangString str_msg_unreg_ole     ${LANG_ITALIAN} \
#    "Tentativo di togliere dal Registry Vim con OLE. \
#     Non c'è messaggio che indica se è riuscito o no."

#LangString str_msg_rm_start      ${LANG_ITALIAN} \
#    "Disinstallazione della seguente versione:"

#LangString str_msg_rm_fail       ${LANG_ITALIAN} \
#    "Disinstallazione non riuscita per la seguente versione:"

#LangString str_msg_no_rm_key     ${LANG_ITALIAN} \
#    "Non riesco a trovare chiave di disinstallazione nel Registry."

#LangString str_msg_no_rm_reg     ${LANG_ITALIAN} \
#    "Non riesco a trovare programma disinstallazione nel Registry."

#LangString str_msg_no_rm_exe     ${LANG_ITALIAN} \
#    "Non riesco a trovare programma disinstallazione."

#LangString str_msg_rm_copy_fail  ${LANG_ITALIAN} \
#    "Non riesco a copiare programma disinstallazione a una \
#     directory temporanea."

#LangString str_msg_rm_run_fail   ${LANG_ITALIAN} \
#    "Non riesco a eseguire programma disinstallazione."

#LangString str_msg_abort_install ${LANG_ITALIAN} \
#    "Il programma di disinstallazione verrà chiuso senza aver fatto nulla."

LangString str_msg_install_fail  ${LANG_ITALIAN} \
    "Installazione non riuscita. Miglior fortuna alla prossima!"

LangString str_msg_rm_exe_fail   ${LANG_ITALIAN} \
    "Alcuni file in $0 non sono stati cancellati!$\r$\n\
     Dovreste cancellarli voi stessi."

#LangString str_msg_rm_root_fail  ${LANG_ITALIAN} \
#    "AVVISO: Non posso cancellare $\"$vim_install_root$\", non è vuota!"

LangString str_msg_uninstalling  ${LANG_ITALIAN} \
    "Sto disinstallando la vecchia versione..."

LangString str_msg_registering   ${LANG_ITALIAN} \
    "Sto aggiungendo Vim al Registry..."

LangString str_msg_unregistering ${LANG_ITALIAN} \
    "Sto togliendo Vim dal Registry..."


##############################################################################
# Dialog Box                                                              {{{1
##############################################################################

LangString str_vimrc_page_title    ${LANG_ITALIAN} \
    "Scelta impostazioni _vimrc"
LangString str_vimrc_page_subtitle ${LANG_ITALIAN} \
    "Scelta impostazioni per funzionalità ulteriori, tastiera e mouse."

LangString str_msg_compat_title    ${LANG_ITALIAN} \
    " comportamento come Vi / Vim "
LangString str_msg_compat_desc     ${LANG_ITALIAN} \
    "&Compatibilità e funzionalità ulteriori"
LangString str_msg_compat_vi       ${LANG_ITALIAN} \
    "Compatibile con Vi"
LangString str_msg_compat_vim      ${LANG_ITALIAN} \
    "Vim originale"
LangString str_msg_compat_defaults ${LANG_ITALIAN} \
    "Vim con alcune funzionalità ulteriori (esecuzione defaults.vim)"
LangString str_msg_compat_all      ${LANG_ITALIAN} \
    "Vim con tutte le funzionalità ulteriori (esecuzione vimrc_example.vim) (Default)"

LangString str_msg_keymap_title   ${LANG_ITALIAN} \
    " Mappature "
LangString str_msg_keymap_desc    ${LANG_ITALIAN} \
    "&Rimappatura di alcuni tasti per Windows (Ctrl-V, Ctrl-C, Ctrl-A, Ctrl-S, Ctrl-F, etc.)"
LangString str_msg_keymap_default ${LANG_ITALIAN} \
    "Non effettuare rimappature di tasti (Default)"
LangString str_msg_keymap_windows ${LANG_ITALIAN} \
    "Rimappare solo alcuni tasti"

LangString str_msg_mouse_title   ${LANG_ITALIAN} \
    " Mouse "
LangString str_msg_mouse_desc    ${LANG_ITALIAN} \
    "&Comportamento dei pulsanti destro e sinistro"
LangString str_msg_mouse_default ${LANG_ITALIAN} \
    "Destro: popup menu, Sinistro: modalità visuale (Default)"
LangString str_msg_mouse_windows ${LANG_ITALIAN} \
    "Destro: popup menu, Sinistro: seleziona modalità (Windows)"
LangString str_msg_mouse_unix    ${LANG_ITALIAN} \
    "Destro: estende selezione, Sinistro: modalità visuale (Unix)"
