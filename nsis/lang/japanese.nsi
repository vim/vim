# vi:set ts=8 sts=4 sw=4 et fdm=marker:
#
# japanese.nsi: Japanese language strings for gvim NSIS installer.
#
# Locale ID    : 1041
# fileencoding : UTF-8
# Author       : Ken Takata

#!include "script\helper_util.nsh"
#${VimAddLanguage} "Japanese" "ja"
!insertmacro MUI_LANGUAGE "Japanese"


# Overwrite the default translation.
# These string should be always English.  Otherwise dosinst.c fails.
LangString ^SetupCaption     ${LANG_JAPANESE} \
        "$(^Name) Setup"
LangString ^UninstallCaption ${LANG_JAPANESE} \
        "$(^Name) Uninstall"

# Workarounds for NSIS Japanese translation. The messages are too long.
# These should be better to be fixed by the NSIS upstream.
LangString ^SpaceAvailable   ${LANG_JAPANESE} \
        "利用可能なディスク容量："
LangString ^SpaceRequired    ${LANG_JAPANESE} \
        "必要なディスク容量："
# Fix another NSIS Japanese translation. The access key was missing.
LangString ^InstallBtn       ${LANG_JAPANESE} \
        "インストール(&I)"

##############################################################################
# MUI Configuration Strings                                               {{{1
##############################################################################

#LangString str_dest_folder          ${LANG_JAPANESE} \
#    "Destination Folder (Must end with $\"vim$\")"

LangString str_show_readme          ${LANG_JAPANESE} \
    "インストール完了後に README を表示する"

# Install types:
LangString str_type_typical         ${LANG_JAPANESE} \
    "通常"

LangString str_type_minimal         ${LANG_JAPANESE} \
    "最小"

LangString str_type_full            ${LANG_JAPANESE} \
    "全て"


##############################################################################
# Section Titles & Description                                            {{{1
##############################################################################

LangString str_section_old_ver      ${LANG_JAPANESE} \
    "既存のバージョンをアンインストール"
LangString str_desc_old_ver         ${LANG_JAPANESE} \
    "すでにインストールされている Vim をシステムから削除します。"

LangString str_section_exe          ${LANG_JAPANESE} \
    "Vim GUI とランタイムファイル"
LangString str_desc_exe             ${LANG_JAPANESE} \
    "Vim GUI 実行ファイルとラインタイムファイル。このコンポーネントは必須です。"

LangString str_section_console      ${LANG_JAPANESE} \
    "Vim コンソールプログラム"
LangString str_desc_console         ${LANG_JAPANESE} \
    "コンソール版の Vim (vim.exe)。"

LangString str_section_batch        ${LANG_JAPANESE} \
    ".bat ファイルを作成"
LangString str_desc_batch           ${LANG_JAPANESE} \
    "コマンドラインから Vim と関連コマンドを実行できるように、.bat ファイルを Windows ディレクトリに作成します。"

LangString str_group_icons          ${LANG_JAPANESE} \
    "Vim のアイコンを作成"
LangString str_desc_icons           ${LANG_JAPANESE} \
    "Vim を簡単に実行できるように、いくつかの場所にアイコンを作成します。"

LangString str_section_desktop      ${LANG_JAPANESE} \
    "デスクトップ上"
LangString str_desc_desktop         ${LANG_JAPANESE} \
    "gVim 実行ファイルのアイコンをデスクトップ上に作成します。"

LangString str_section_start_menu   ${LANG_JAPANESE} \
    "スタートメニューのプログラムフォルダー上"
LangString str_desc_start_menu      ${LANG_JAPANESE} \
    "Vim のアイコンをスタートメニューのプログラムフォルダー上に作成します。"

LangString str_section_quick_launch ${LANG_JAPANESE} \
    "In the Quick Launch Bar"
LangString str_desc_quick_launch    ${LANG_JAPANESE} \
    "Add Vim shortcut in the quick launch bar."

LangString str_section_edit_with    ${LANG_JAPANESE} \
    "Vim のコンテキストメニューを追加"
LangString str_desc_edit_with       ${LANG_JAPANESE} \
    "$\"Vimで編集する$\" をコンテキストメニューに追加します。"

LangString str_section_edit_with32  ${LANG_JAPANESE} \
    "32-bit Version"
LangString str_desc_edit_with32     ${LANG_JAPANESE} \
    "Add Vim to the $\"Open With...$\" context menu list \
     for 32-bit applications."

LangString str_section_edit_with64  ${LANG_JAPANESE} \
    "64-bit Version"
LangString str_desc_edit_with64     ${LANG_JAPANESE} \
    "Add Vim to the $\"Open With...$\" context menu list \
     for 64-bit applications."

LangString str_section_vim_rc       ${LANG_JAPANESE} \
    "既定のコンフィグを作成"
LangString str_desc_vim_rc          ${LANG_JAPANESE} \
    "もし無ければ、既定のコンフィグファイル (_vimrc) を作成します。"

LangString str_group_plugin         ${LANG_JAPANESE} \
    "プラグインディレクトリを作成"
LangString str_desc_plugin          ${LANG_JAPANESE} \
    "プラグインディレクトリを作成します。そこにプラグインファイルを置くことで Vim を拡張することができます。"

LangString str_section_plugin_home  ${LANG_JAPANESE} \
    "個人用"
LangString str_desc_plugin_home     ${LANG_JAPANESE} \
    "プラグインディレクトリを HOME (もし定義していれば)、または Vim のインストールディレクトリに作成します。"

LangString str_section_plugin_vim   ${LANG_JAPANESE} \
    "共用"
LangString str_desc_plugin_vim      ${LANG_JAPANESE} \
    "プラグインディレクトリを Vim のインストールディレクトリに作成します。システムの全員で共有されます。"

LangString str_section_vis_vim      ${LANG_JAPANESE} \
    "VisVim 拡張"
LangString str_desc_vis_vim         ${LANG_JAPANESE} \
    "Microsoft Visual Studio 統合用の VisVim 拡張。"

LangString str_section_nls          ${LANG_JAPANESE} \
    "多言語サポート"
LangString str_desc_nls             ${LANG_JAPANESE} \
    "多言語サポート用のファイルをインストールします。"

LangString str_unsection_register   ${LANG_JAPANESE} \
    "Vim を登録解除"
LangString str_desc_unregister      ${LANG_JAPANESE} \
    "Vim をシステムから登録解除します。"

LangString str_unsection_exe        ${LANG_JAPANESE} \
    "Vim の実行ファイル/ランタイムファイルを削除"
LangString str_desc_rm_exe          ${LANG_JAPANESE} \
    "全ての Vim の実行ファイルとランタイムファイルを削除します。"

LangString str_unsection_vimfiles   ${LANG_JAPANESE} \
    "vimfiles ディレクトリを削除"
LangString str_desc_rm_vimfiles     ${LANG_JAPANESE} \
    "vimfiles ディレクトリとその中の全てのファイルを削除します。"

LangString str_unsection_rootdir    ${LANG_JAPANESE} \
    "Vim のトップディレクトリを削除"
LangString str_desc_rm_rootdir      ${LANG_JAPANESE} \
    "Vim のトップディレクトリを削除します。あなたの Vim の設定ファイルも含まれていることに注意してください！"

LangString str_vimrc_page_title     ${LANG_JAPANESE} \
    "_vimrc の設定を選んでください"
LangString str_vimrc_page_subtitle  ${LANG_JAPANESE} \
    "キーのリマッピングとマウスの動作の設定を選んでください。"


##############################################################################
# Messages                                                                {{{1
##############################################################################

LangString str_msg_too_many_ver  ${LANG_JAPANESE} \
    "Found $vim_old_ver_count Vim versions on your system.$\r$\n\
     This installer can only handle ${VIM_MAX_OLD_VER} versions \
     at most.$\r$\n\
     Please remove some versions and start again."

LangString str_msg_invalid_root  ${LANG_JAPANESE} \
    "Invalid install path: $vim_install_root!$\r$\n\
     It should end with $\"vim$\"."

LangString str_msg_bin_mismatch  ${LANG_JAPANESE} \
    "Binary path mismatch!$\r$\n$\r$\n\
     Expect the binary path to be $\"$vim_bin_path$\",$\r$\n\
     but system indicates the binary path is $\"$INSTDIR$\"."

LangString str_msg_vim_running   ${LANG_JAPANESE} \
    "Vim is still running on your system.$\r$\n\
     Please close all instances of Vim before you continue."

LangString str_msg_register_ole  ${LANG_JAPANESE} \
    "Attempting to register Vim with OLE. \
     There is no message indicates whether this works or not."

LangString str_msg_unreg_ole     ${LANG_JAPANESE} \
    "Attempting to unregister Vim with OLE. \
     There is no message indicates whether this works or not."

LangString str_msg_rm_start      ${LANG_JAPANESE} \
    "Uninstalling the following version:"

LangString str_msg_rm_fail       ${LANG_JAPANESE} \
    "Fail to uninstall the following version:"

LangString str_msg_no_rm_key     ${LANG_JAPANESE} \
    "Cannot find uninstaller registry key."

LangString str_msg_no_rm_reg     ${LANG_JAPANESE} \
    "Cannot find uninstaller from registry."

LangString str_msg_no_rm_exe     ${LANG_JAPANESE} \
    "Cannot access uninstaller."

LangString str_msg_rm_copy_fail  ${LANG_JAPANESE} \
    "Fail to copy uninstaller to temporary directory."

LangString str_msg_rm_run_fail   ${LANG_JAPANESE} \
    "Fail to run uninstaller."

LangString str_msg_abort_install ${LANG_JAPANESE} \
    "Installer will abort."

LangString str_msg_install_fail  ${LANG_JAPANESE} \
    "Installation failed. Better luck next time."

LangString str_msg_rm_exe_fail   ${LANG_JAPANESE} \
    "Some files in $vim_bin_path have not been deleted!$\r$\n\
     You must do it manually."

LangString str_msg_rm_root_fail  ${LANG_JAPANESE} \
    "WARNING: Cannot remove $\"$vim_install_root$\", it is not empty!"

LangString str_msg_keymap_title   ${LANG_JAPANESE} \
    " キーのリマッピング "
LangString str_msg_keymap_default ${LANG_JAPANESE} \
    " Windows用にキーをリマップしない(既定)"
LangString str_msg_keymap_windows ${LANG_JAPANESE} \
    " Windowsの動作に合わせていくつかのキーをリマップする$\n (例: <C-V>, <C-C>, <C-A>, <C-S>, <C-F> など)"

LangString str_msg_mouse_title   ${LANG_JAPANESE} \
    " マウスの動作 "
LangString str_msg_mouse_default ${LANG_JAPANESE} \
    " 既定:$\n     右ボタンはポップアップメニュー、左ボタンはビジュアルモードを開始"
LangString str_msg_mouse_windows ${LANG_JAPANESE} \
    " Windows:$\n     右ボタンはポップアップメニュー、左ボタンは選択モードを開始"
LangString str_msg_mouse_unix    ${LANG_JAPANESE} \
    " Unix:$\n     右ボタンは選択を拡張、左ボタンはビジュアルモードを開始"
