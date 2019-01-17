# vi:set ts=8 sts=4 sw=4 et fdm=marker:
#
# simpchinese.nsi: Simplified Chinese language strings for gvim NSIS
# installer.
#
# Locale ID    : 2052
# fileencoding : UTF-8
# Author       : Guopeng Wen

!insertmacro MUI_LANGUAGE "SimpChinese"


# Overwrite the default translation.
# These strings should be always English.  Otherwise dosinst.c fails.
LangString ^SetupCaption     ${LANG_SIMPCHINESE} \
        "$(^Name) Setup"
LangString ^UninstallCaption ${LANG_SIMPCHINESE} \
        "$(^Name) Uninstall"

##############################################################################
# MUI Configuration Strings                                               {{{1
##############################################################################

#LangString str_dest_folder          ${LANG_SIMPCHINESE} \
#    "安装路径 (必须以 vim 结尾)"

LangString str_show_readme          ${LANG_SIMPCHINESE} \
    "安装完成后显示 README 文件"

# Install types:
LangString str_type_typical         ${LANG_SIMPCHINESE} \
    "典型安装"

LangString str_type_minimal         ${LANG_SIMPCHINESE} \
    "最小安装"

LangString str_type_full            ${LANG_SIMPCHINESE} \
    "完全安装"


##############################################################################
# Section Titles & Description                                            {{{1
##############################################################################

LangString str_section_old_ver      ${LANG_SIMPCHINESE} \
    "卸载旧版本"
LangString str_desc_old_ver         ${LANG_SIMPCHINESE} \
    "卸载系统上旧版本的 Vim。"

LangString str_section_exe          ${LANG_SIMPCHINESE} \
    "安装 Vim 图形界面"
LangString str_desc_exe             ${LANG_SIMPCHINESE} \
    "安装 Vim 图形界面及脚本。此为必选安装。"

LangString str_section_console      ${LANG_SIMPCHINESE} \
    "安装 Vim 命令行程序"
LangString str_desc_console         ${LANG_SIMPCHINESE} \
    "安装 Vim 命令行程序 (vim.exe)。该程序在命令行窗口中运行。"

LangString str_section_batch        ${LANG_SIMPCHINESE} \
    "安装批处理文件"
LangString str_desc_batch           ${LANG_SIMPCHINESE} \
    "为 Vim 的各种变体创建批处理程序，以便在命令行下运行 Vim。"

LangString str_group_icons          ${LANG_SIMPCHINESE} \
    "创建 Vim 图标"
LangString str_desc_icons           ${LANG_SIMPCHINESE} \
    "为 Vim 创建若干图标，以方便使用 Vim。"

LangString str_section_desktop      ${LANG_SIMPCHINESE} \
    "在桌面上"
LangString str_desc_desktop         ${LANG_SIMPCHINESE} \
    "在桌面上为 Vim 创建若干图标，以方便启动 Vim。"

LangString str_section_start_menu   ${LANG_SIMPCHINESE} \
    "在启动菜单的程序菜单下"
LangString str_desc_start_menu      ${LANG_SIMPCHINESE} \
    "在启动菜单的程序菜单下添加 Vim 组。适用于 Windows 95 及以上版本。"

#LangString str_section_quick_launch ${LANG_SIMPCHINESE} \
#    "在快速启动启动栏中"
#LangString str_desc_quick_launch    ${LANG_SIMPCHINESE} \
#    "在快速启动栏中添加 Vim 图标。"

LangString str_section_edit_with    ${LANG_SIMPCHINESE} \
    "安装快捷菜单"
LangString str_desc_edit_with       ${LANG_SIMPCHINESE} \
    "将 Vim 添加到“打开方式”快捷菜单中。"

#LangString str_section_edit_with32  ${LANG_SIMPCHINESE} \
#    "32 位版本"
#LangString str_desc_edit_with32     ${LANG_SIMPCHINESE} \
#    "将 Vim 添加到 32 位程序的“打开方式”快捷菜单中。"

#LangString str_section_edit_with64  ${LANG_SIMPCHINESE} \
#    "64 位版本"
#LangString str_desc_edit_with64     ${LANG_SIMPCHINESE} \
#    "将 Vim 添加到 64 位程序的“打开方式”快捷菜单中。"

LangString str_section_vim_rc       ${LANG_SIMPCHINESE} \
    "创建缺省配置文件"
LangString str_desc_vim_rc          ${LANG_SIMPCHINESE} \
    "在安装目录下生成缺省的 Vim 配置文件(_vimrc)。\
     如果该文件已经存在，则略过此项。"

LangString str_group_plugin         ${LANG_SIMPCHINESE} \
    "创建插件目录"
LangString str_desc_plugin          ${LANG_SIMPCHINESE} \
    "创建(空的)插件目录结构。插件目录用于安装 Vim 扩展插件，\
     只要将文件复制到相关的子目录中即可。"

LangString str_section_plugin_home  ${LANG_SIMPCHINESE} \
    "私有插件目录"
LangString str_desc_plugin_home     ${LANG_SIMPCHINESE} \
    "Create plugin directories in HOME directory."

LangString str_section_plugin_vim   ${LANG_SIMPCHINESE} \
    "公共插件目录"
LangString str_desc_plugin_vim      ${LANG_SIMPCHINESE} \
    "在 Vim 安装目录下创建(空的)插件目录结构，系统上所有用户都能使用安装在\
     该目录下的扩展插件。"

LangString str_section_vis_vim      ${LANG_SIMPCHINESE} \
    "安装 VisVim 插件"
LangString str_desc_vis_vim         ${LANG_SIMPCHINESE} \
    "安装用于与微软 Microsoft Visual Studio 进行集成的 VisVim 插件。"

LangString str_section_nls          ${LANG_SIMPCHINESE} \
    "安装多语言支持"
LangString str_desc_nls             ${LANG_SIMPCHINESE} \
    "安装用于多语言支持的文件。"

LangString str_unsection_register   ${LANG_SIMPCHINESE} \
    "删除 Vim 系统配置"
LangString str_desc_unregister      ${LANG_SIMPCHINESE} \
    "删除和 Vim 相关的系统配置。"

LangString str_unsection_exe        ${LANG_SIMPCHINESE} \
    "删除 Vim 执行文件以及脚本"
LangString str_desc_rm_exe          ${LANG_SIMPCHINESE} \
    "删除 Vim 的所有执行文件及脚本。"

LangString str_ungroup_plugin       ${LANG_SIMPCHINESE} \
    "Remove plugin directories"
LangString str_desc_rm_plugin       ${LANG_SIMPCHINESE} \
    "Remove the plugin directories if they are empty."

LangString str_unsection_plugin_home ${LANG_SIMPCHINESE} \
    "私有插件目录"
LangString str_desc_rm_plugin_home  ${LANG_SIMPCHINESE} \
    "Remove the plugin directories from HOME directory."

LangString str_unsection_plugin_vim ${LANG_SIMPCHINESE} \
    "公共插件目录"
LangString str_desc_rm_plugin_vim   ${LANG_SIMPCHINESE} \
    "Remove the plugin directories from Vim install directory."

LangString str_unsection_rootdir    ${LANG_SIMPCHINESE} \
    "Remove the Vim root directory"
LangString str_desc_rm_rootdir      ${LANG_SIMPCHINESE} \
    "Remove the Vim root directory. It contains your Vim configuration files!"


##############################################################################
# Messages                                                                {{{1
##############################################################################

#LangString str_msg_too_many_ver  ${LANG_SIMPCHINESE} \
#    "您的系统上安装了 $vim_old_ver_count 个不同版本的 Vim，$\r$\n\
#     但本安装程序最多只能处理 ${VIM_MAX_OLD_VER} 个版本。$\r$\n\
#     请您手工删除一些旧版本以后再运行本安装程序。"

#LangString str_msg_invalid_root  ${LANG_SIMPCHINESE} \
#    "安装路径“$vim_install_root”无效！$\r$\n\
#     该路径必须以 vim 结尾。"

#LangString str_msg_bin_mismatch  ${LANG_SIMPCHINESE} \
#    "Vim 执行程序安装路径异常！$\r$\n$\r$\n\
#     该版本 Vim 的执行程序安装路径应该是“$vim_bin_path”,$\r$\n\
#     而系统却指示该路径为“$INSTDIR”。"

#LangString str_msg_vim_running   ${LANG_SIMPCHINESE} \
#    "您的系统上仍有 Vim 在运行，$\r$\n\
#     请您在执行后续步骤前退出这些 Vim。"

#LangString str_msg_register_ole  ${LANG_SIMPCHINESE} \
#    "试图注册 Vim OLE 服务器。请注意无论成功与否都不再显示进一步的信息。"

#LangString str_msg_unreg_ole     ${LANG_SIMPCHINESE} \
#    "试图注销 Vim OLE 服务器。请注意无论成功与否都不再显示进一步的信息。"

#LangString str_msg_rm_start      ${LANG_SIMPCHINESE} \
#    "开始卸载以下版本："

#LangString str_msg_rm_fail       ${LANG_SIMPCHINESE} \
#    "以下版本卸载失败："

#LangString str_msg_no_rm_key     ${LANG_SIMPCHINESE} \
#    "找不到卸载程序的注册表键。"

#LangString str_msg_no_rm_reg     ${LANG_SIMPCHINESE} \
#    "在注册表中未找到卸载程序路径。"

#LangString str_msg_no_rm_exe     ${LANG_SIMPCHINESE} \
#    "找不到卸载程序。"

#LangString str_msg_rm_copy_fail  ${LANG_SIMPCHINESE} \
#    "无法将卸载程序复制到临时目录。"

#LangString str_msg_rm_run_fail   ${LANG_SIMPCHINESE} \
#    "执行卸载程序失败。"

#LangString str_msg_abort_install ${LANG_SIMPCHINESE} \
#    "安装程序将退出。"

LangString str_msg_install_fail  ${LANG_SIMPCHINESE} \
    "安装失败。祝您下次好运。"

LangString str_msg_rm_exe_fail   ${LANG_SIMPCHINESE} \
    "目录“$0”下有部分文件删除失败！$\r$\n\
     您只能手工删除该目录。"

#LangString str_msg_rm_root_fail  ${LANG_SIMPCHINESE} \
#    "警告：无法删除 Vim 安装目录“$vim_install_root”，\
#     该目录下仍有其他文件。"

LangString str_msg_uninstalling  ${LANG_SIMPCHINESE} \
    "Uninstalling the old version..."

LangString str_msg_registering   ${LANG_SIMPCHINESE} \
    "Registering..."

LangString str_msg_unregistering ${LANG_SIMPCHINESE} \
    "Unregistering..."


##############################################################################
# Dialog Box                                                              {{{1
##############################################################################

LangString str_vimrc_page_title    ${LANG_SIMPCHINESE} \
    "Choose _vimrc settings"
LangString str_vimrc_page_subtitle ${LANG_SIMPCHINESE} \
    "Choose the settings for enhancement, keyboard and mouse."

LangString str_msg_compat_title    ${LANG_SIMPCHINESE} \
    " Vi / Vim behavior "
LangString str_msg_compat_desc     ${LANG_SIMPCHINESE} \
    "&Compatibility and enhancements"
LangString str_msg_compat_vi       ${LANG_SIMPCHINESE} \
    "Vi compatible"
LangString str_msg_compat_vim      ${LANG_SIMPCHINESE} \
    "Vim original"
LangString str_msg_compat_defaults ${LANG_SIMPCHINESE} \
    "Vim with some enhancements (load defaults.vim)"
LangString str_msg_compat_all      ${LANG_SIMPCHINESE} \
    "Vim with all enhancements (load vimrc_example.vim) (Default)"

LangString str_msg_keymap_title   ${LANG_SIMPCHINESE} \
    " Mappings "
LangString str_msg_keymap_desc    ${LANG_SIMPCHINESE} \
    "&Remap a few keys for Windows (Ctrl-V, Ctrl-C, Ctrl-A, Ctrl-S, Ctrl-F, etc)"
LangString str_msg_keymap_default ${LANG_SIMPCHINESE} \
    "Do not remap keys (Default)"
LangString str_msg_keymap_windows ${LANG_SIMPCHINESE} \
    "Remap a few keys"

LangString str_msg_mouse_title   ${LANG_SIMPCHINESE} \
    " Mouse "
LangString str_msg_mouse_desc    ${LANG_SIMPCHINESE} \
    "&Behavior of right and left buttons"
LangString str_msg_mouse_default ${LANG_SIMPCHINESE} \
    "Right: popup menu, Left: visual mode (Default)"
LangString str_msg_mouse_windows ${LANG_SIMPCHINESE} \
    "Right: popup menu, Left: select mode (Windows)"
LangString str_msg_mouse_unix    ${LANG_SIMPCHINESE} \
    "Right: extends selection, Left: visual mode (Unix)"
