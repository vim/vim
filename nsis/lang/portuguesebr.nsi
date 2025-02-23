# vi:set ts=8 sts=4 sw=4 et fdm=marker:
#
# portuguesebr.nsi: Brazilian Portuguese strings for gvim NSIS installer.
#
# Locale ID    : 1046 
# Locale Name  : pt-BR
# fileencoding : UTF-8
# Author       : Rafael Fontenelle

!insertmacro MUI_LANGUAGE "PortugueseBR"


# Overwrite the default translation.
# These strings should be always English.  Otherwise dosinst.c fails.
LangString ^SetupCaption     ${LANG_PORTUGUESEBR} \
        "Configuração do $(^Name)"
LangString ^UninstallCaption ${LANG_PORTUGUESEBR} \
        "Desinstalação do $(^Name)"

##############################################################################
# License file for the license page                                       {{{1
##############################################################################

LicenseLangString page_lic_file ${LANG_PORTUGUESEBR} "..\lang\LICENSE.pt_br.nsis.txt"

##############################################################################
# README.txt file, which is opened after installation                     {{{1
##############################################################################

LangString vim_readme_file ${LANG_PORTUGUESEBR} "README.pt_br.txt"

##############################################################################
# MUI Configuration Strings                                               {{{1
##############################################################################

#LangString str_dest_folder          ${LANG_PORTUGUESEBR} \
#    "Pasta de destino (Deve terminar com $\"vim$\")"

LangString str_show_readme          ${LANG_PORTUGUESEBR} \
    "Mostrar o README ao concluir a instalação"

# Install types:
LangString str_type_typical         ${LANG_PORTUGUESEBR} \
    "Típica"

LangString str_type_minimal         ${LANG_PORTUGUESEBR} \
    "Mínima"

LangString str_type_full            ${LANG_PORTUGUESEBR} \
    "Completa"


##############################################################################
# Section Titles & Description                                            {{{1
##############################################################################

LangString str_section_old_ver      ${LANG_PORTUGUESEBR} \
    "Desinstalar versões existentes"
LangString str_desc_old_ver         ${LANG_PORTUGUESEBR} \
    "Desinstala versões do Vim existentes no seu sistema."

LangString str_section_exe          ${LANG_PORTUGUESEBR} \
    "Vim GUI e arquivos de runtime"
LangString str_desc_exe             ${LANG_PORTUGUESEBR} \
    "Executáveis ​​e arquivos de runtime do Vim GUI. Este componente \
     é obrigatório."

LangString str_section_console      ${LANG_PORTUGUESEBR} \
    "Programa de console do Vim"
LangString str_desc_console         ${LANG_PORTUGUESEBR} \
    "Versão de console do Vim (vim.exe)."

LangString str_section_batch        ${LANG_PORTUGUESEBR} \
    "Criar arquivos .bat"
LangString str_desc_batch           ${LANG_PORTUGUESEBR} \
    "Cria arquivos .bat para variantes do Vim no diretório Windows para \
     usar na linha de comando."

LangString str_group_icons          ${LANG_PORTUGUESEBR} \
    "Criar ícones para o Vim"
LangString str_desc_icons           ${LANG_PORTUGUESEBR} \
    "Cria ícones para Vim em vários locais para facilitar o acesso rápido."

LangString str_section_desktop      ${LANG_PORTUGUESEBR} \
    "Na área de trabalho"
LangString str_desc_desktop         ${LANG_PORTUGUESEBR} \
    "Cria ícones para executáveis do gVim na área de trabalho."

LangString str_section_start_menu   ${LANG_PORTUGUESEBR} \
    "Na pasta Programas do Menu Iniciar"
LangString str_desc_start_menu      ${LANG_PORTUGUESEBR} \
    "Adiciona Vim na pasta de programas do menu iniciar."

#LangString str_section_quick_launch ${LANG_PORTUGUESEBR} \
#    "Na barra de Inicialização Rápida"
#LangString str_desc_quick_launch    ${LANG_PORTUGUESEBR} \
#    "Adiciona um atalho para o Vim na barra de inicialização rápida."

LangString str_section_edit_with    ${LANG_PORTUGUESEBR} \
    "Adicionar menu de contexto do Vim"
LangString str_desc_edit_with       ${LANG_PORTUGUESEBR} \
    "Adiciona o Vim à lista $\"Abrir com...$\" do menu de contexto."

#LangString str_section_edit_with32  ${LANG_PORTUGUESEBR} \
#    "Versão 32 bits"
#LangString str_desc_edit_with32     ${LANG_PORTUGUESEBR} \
#    "Adiciona o Vim à lista $\"Abrir com...$\" do menu de contexto \
#     para aplicativos 32 bits."

#LangString str_section_edit_with64  ${LANG_PORTUGUESEBR} \
#    "Versão 64 bits"
#LangString str_desc_edit_with64     ${LANG_PORTUGUESEBR} \
#    "Adiciona o Vim à lista $\"Abrir com...$\" do menu de contexto \
#     para aplicativos 64 bits."

LangString str_section_vim_rc       ${LANG_PORTUGUESEBR} \
    "Criar configuração padrão"
LangString str_desc_vim_rc          ${LANG_PORTUGUESEBR} \
    "Cria um arquivo de configuração padrão (_vimrc) se não já existir."

LangString str_group_plugin         ${LANG_PORTUGUESEBR} \
    "Criar diretórios de plugins"
LangString str_desc_plugin          ${LANG_PORTUGUESEBR} \
    "Cria diretórios de plugins. Diretórios de plugins permitem estender \
     o Vim adicionando um arquivo a um direótrio."

LangString str_section_plugin_home  ${LANG_PORTUGUESEBR} \
    "Privados"
LangString str_desc_plugin_home     ${LANG_PORTUGUESEBR} \
    "Cria diretórios de plugins no diretório HOME."

LangString str_section_plugin_vim   ${LANG_PORTUGUESEBR} \
    "Compartilhados"
LangString str_desc_plugin_vim      ${LANG_PORTUGUESEBR} \
    "Cria diretórios de plugins no diretório de instalação do Vim, \
     o qual é usado por todos os usuários do sistema."

LangString str_section_nls          ${LANG_PORTUGUESEBR} \
    "Suporte ao idioma nativo"
LangString str_desc_nls             ${LANG_PORTUGUESEBR} \
    "Instala arquivos para suporte ao idioma nativo."

LangString str_unsection_register   ${LANG_PORTUGUESEBR} \
    "Desregistrar Vim"
LangString str_desc_unregister      ${LANG_PORTUGUESEBR} \
    "Remove os registras do Vim do sistema."

LangString str_unsection_exe        ${LANG_PORTUGUESEBR} \
    "Remover arquivos executáveis/runtime do Vim"
LangString str_desc_rm_exe          ${LANG_PORTUGUESEBR} \
    "Remove todos os arquivos executáveis e de runtime do Vim."

LangString str_ungroup_plugin       ${LANG_PORTUGUESEBR} \
    "Remover diretórios de plugins"
LangString str_desc_rm_plugin       ${LANG_PORTUGUESEBR} \
    "Remove os diretórios de plugins se eles estiverem vazios."

LangString str_unsection_plugin_home ${LANG_PORTUGUESEBR} \
    "Privados"
LangString str_desc_rm_plugin_home  ${LANG_PORTUGUESEBR} \
    "Remove os diretórios de plugins do diretório HOME."

LangString str_unsection_plugin_vim ${LANG_PORTUGUESEBR} \
    "Compartilhados"
LangString str_desc_rm_plugin_vim   ${LANG_PORTUGUESEBR} \
    "Remove is diretórios de plugins do diretório de instalação do Vim."

LangString str_unsection_rootdir    ${LANG_PORTUGUESEBR} \
    "Remover diretório raiz do Vim"
LangString str_desc_rm_rootdir      ${LANG_PORTUGUESEBR} \
    "Remove o diretório raiz do Vim. Ele contém seus \
     arquivos de configuração do Vim!"


##############################################################################
# Messages                                                                {{{1
##############################################################################

#LangString str_msg_too_many_ver  ${LANG_PORTUGUESEBR} \
#    "Encontradas $vim_old_ver_count versões do Vim em seu sistema.$\r$\n\
#     Este instalador só consegue lidar com ${VIM_MAX_OLD_VER} versões\
#     no máximo.$\r$\n\
#     Por favor, remova algumas versões e comece novamente."

#LangString str_msg_invalid_root  ${LANG_PORTUGUESEBR} \
#    "Caminho de instalação inválido: $vim_install_root!$\r$\n\
#     Ele deve terminar com $\"vim$\"."

#LangString str_msg_bin_mismatch  ${LANG_PORTUGUESEBR} \
#    "Incompatibilidade de caminho de binários!$\r$\n$\r$\n\
#     Esperava que o caminho de binários fosse $\"$vim_bin_path$\",$\r$\n\
#     mas o sistema indica que o caminho de binários é $\"$INSTDIR$\"."

#LangString str_msg_vim_running   ${LANG_PORTUGUESEBR} \
#    "Vim ainda está em execução em seu sistema.$\r$\n\
#     Por favor, feche todas as instâncias do Vim para poder continuar."

#LangString str_msg_register_ole  ${LANG_PORTUGUESEBR} \
#    "Tentativa de registrar o Vim com OLE. \
#     Não há mensagem que indique se isso funciona ou não."

#LangString str_msg_unreg_ole     ${LANG_PORTUGUESEBR} \
#    "Tentando desregistrar o Vim com OLE. \
#     Não há mensagem que indique se isso funciona ou não."

#LangString str_msg_rm_start      ${LANG_PORTUGUESEBR} \
#    "Desinstalando a seguinte versão:"

#LangString str_msg_rm_fail       ${LANG_PORTUGUESEBR} \
#    "Falha ao desinstalar a seguinte versão:"

#LangString str_msg_no_rm_key     ${LANG_PORTUGUESEBR} \
#    "Não foi possível encontrar a chave de registro do desinstalador."

#LangString str_msg_no_rm_reg     ${LANG_PORTUGUESEBR} \
#    "Não foi possível encontrar o desinstalador a partir do registro."

#LangString str_msg_no_rm_exe     ${LANG_PORTUGUESEBR} \
#    "Não foi possível acessar o desinstalador."

#LangString str_msg_rm_copy_fail  ${LANG_PORTUGUESEBR} \
#    "Falha ao copiar o desinstalador para um diretório temporário."

#LangString str_msg_rm_run_fail   ${LANG_PORTUGUESEBR} \
#    "Falha ao executar o desinstalador."

#LangString str_msg_abort_install ${LANG_PORTUGUESEBR} \
#    "Instalador será interrompido."

LangString str_msg_install_fail  ${LANG_PORTUGUESEBR} \
    "A instalação falhou. Mais sorte na próxima vez."

LangString str_msg_rm_exe_fail   ${LANG_PORTUGUESEBR} \
    "Alguns arquivos em $0 não foram excluídos!$\r$\n\
     Você deve fazê-lo manualmente."

#LangString str_msg_rm_root_fail  ${LANG_PORTUGUESEBR} \
#    "AVISO: Não foi possível remover $\"$vim_install_root$\", \
#    pois não está vazio!"

LangString str_msg_uninstalling  ${LANG_PORTUGUESEBR} \
    "Desinstalando a versão antiga..."

LangString str_msg_registering   ${LANG_PORTUGUESEBR} \
    "Registrando..."

LangString str_msg_unregistering ${LANG_PORTUGUESEBR} \
    "Desregistrando..."


##############################################################################
# Dialog Box                                                              {{{1
##############################################################################

LangString str_vimrc_page_title    ${LANG_PORTUGUESEBR} \
    "Escolher configurações do _vimrc"
LangString str_vimrc_page_subtitle ${LANG_PORTUGUESEBR} \
    "Escolha as configurações para melhorias, teclado e mouse."

LangString str_msg_compat_title    ${LANG_PORTUGUESEBR} \
    " Comportamento Vi / Vim "
LangString str_msg_compat_desc     ${LANG_PORTUGUESEBR} \
    "&Compatibilidade e melhorias"
LangString str_msg_compat_vi       ${LANG_PORTUGUESEBR} \
    "Compatível com Vi"
LangString str_msg_compat_vim      ${LANG_PORTUGUESEBR} \
    "Vim original"
LangString str_msg_compat_defaults ${LANG_PORTUGUESEBR} \
    "Vim com algumas melhorias (carrega defaults.vim)"
LangString str_msg_compat_all      ${LANG_PORTUGUESEBR} \
    "Vim com todas as melhorias (carrega vimrc_example.vim) (Padrão)"

LangString str_msg_keymap_title   ${LANG_PORTUGUESEBR} \
    " Mapeamentos "
LangString str_msg_keymap_desc    ${LANG_PORTUGUESEBR} \
    "&Remapear algumas teclas para o Windows (Ctrl-V, Ctrl-C, Ctrl-A, Ctrl-S, Ctrl-F, etc)"
LangString str_msg_keymap_default ${LANG_PORTUGUESEBR} \
    "Não remapear teclas (Padrão)"
LangString str_msg_keymap_windows ${LANG_PORTUGUESEBR} \
    "Remapear algumas teclas"

LangString str_msg_mouse_title   ${LANG_PORTUGUESEBR} \
    " Mouse "
LangString str_msg_mouse_desc    ${LANG_PORTUGUESEBR} \
    "&Comportamento de botões direito e esquerdo"
LangString str_msg_mouse_default ${LANG_PORTUGUESEBR} \
    "Direito: menu popup, Esquerdo: modo visual (Padrão)"
LangString str_msg_mouse_windows ${LANG_PORTUGUESEBR} \
    "Direito: menu popup, Esquerdo: modo de seleção (Windows)"
LangString str_msg_mouse_unix    ${LANG_PORTUGUESEBR} \
    "Direito: estende seleção, Esquerdo: modo visual (Unix)"
