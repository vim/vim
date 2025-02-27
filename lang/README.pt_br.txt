README.txt para a versão 9.1 do Vim: Vi IMproved.

O QUE É VIM?

Vim é uma versão muito melhorada do bom e velho editor UNIX Vi. Muitos novos
recursos foram adicionados: desfazer multinível, destaque de sintaxe, histórico
de linha de comando, ajuda on-line, verificação ortográfica, completar
nome de arquivo, operações de bloco, linguagem de script, etc. Há também uma
interface gráfica (GUI) disponível. Ainda assim, a compatibilidade com Vi é
mantida, aqueles que têm Vi "nos dedos" se sentirão em casa.
Veja "runtime/doc/vi_diff.txt" para diferenças com Vi.

Este editor é muito útil para editar programas e outros arquivos de texto
simples. Todos os comandos são fornecidos com caracteres normais do teclado,
então aqueles que podem digitar com dez dedos podem trabalhar muito rápido.
Além disso, as teclas de função podem ser mapeadas para comandos pelo usuário,
e o mouse pode ser usado.

O Vim funciona em MS-Windows (7, 8, 10, 11), macOS, Haiku, VMS e quase todos
os sabores do UNIX. A portabilidade para outros sistemas não deve ser difícil.
Versões mais antigas do Vim rodam em MS-DOS, MS-Windows 95/98/Me/NT/2000/XP/Vista,
Amiga DOS, Atari MiNT, BeOS, RISC OS e OS/2. Eles não são mais mantidos.

DISTRIBUIÇÃO

Você pode frequentemente usar seu gerenciador de pacotes favorito para
instalar o Vim. No Mac e Linux, uma pequena versão do Vim é pré-instalada,
você ainda precisa instalar o Vim se quiser mais recursos.

Existem distribuições separadas para Unix, PC, Amiga e alguns outros sistemas.
Este arquivo README.txt vem com o arquivo de tempo de execução. Ele inclui a
documentação, arquivos de sintaxe e outros arquivos que são usados ​​em
tempo de execução. Para executar o Vim, você deve obter um dos arquivos
binários ou um arquivo fonte. Qual deles você precisa depende do sistema em
que deseja executá-lo e se você deseja ou deve compilá-lo você mesmo.
Verifique "https://www.vim.org/download.php" para uma visão geral das
distribuições disponíveis atualmente.

Alguns lugares populares para obter o Vim mais recente:
* Confira o repositório git do github: https://github.com/vim/vim.
* Obtenha o código-fonte como um arquivo: https://github.com/vim/vim/tags.
* Obtenha um executável do Windows do repositório vim-win32-installer:
https://github.com/vim/vim-win32-installer/releases.

COMPILAR

Se você obteve uma distribuição binária, não precisa compilar o Vim. Se você
obteve uma distribuição de origem, todo o material para compilar o Vim está no
diretório "src". Veja src/INSTALL para instruções.

INSTALAÇÃO

Veja um desses arquivos para instruções específicas do sistema. No diretório
READMEdir (no repositório) ou no diretório superior (se você descompactar um
arquivo):

README_ami.txt		Amiga
README_unix.txt		Unix
README_dos.txt		MS-DOS e MS-Windows
README_mac.txt		Macintosh
README_haiku.txt	Haiku
README_vms.txt		VMS

Existem outros arquivos README_*.txt, dependendo da distribuição que você usou.

DOCUMENTAÇÃO

O tutor do Vim é um curso de treinamento de uma hora para iniciantes.
Frequentemente, ele pode ser iniciado como "vimtutor". Veja ":help tutor"
para mais informações.

O melhor é usar ":help" no Vim. Se você ainda não tem um executável, leia
"runtime/doc/help.txt". Ele contém direcionamentos para os outros arquivos
de documentação. O Manual do Usuário é lido como um livro e é recomendado
para aprender a usar o Vim. Veja ":help user-manual".

CÓPIA

O Vim é um Charityware. Você pode usá-lo e copiá-lo o quanto quiser, mas
encorajamos que faça uma doação para ajudar órfãos em Uganda. Leia o arquivo
"runtime/doc/uganda.txt" para detalhes (execute ":help uganda" dentro do Vim).

Resumo da licença: Não há restrições quanto ao uso ou distribuição de uma
cópia não modificada do Vim. Partes do Vim também podem ser distribuídas, mas
o texto da licença deve sempre ser incluído. Para versões modificadas, algumas
restrições se aplicam. A licença é compatível com GPL, você pode compilar o Vim
com bibliotecas GPL e distribuí-lo.

PATROCÍNIO

Corrigir bugs e adicionar novos recursos exige muito tempo e esforço.
Para mostrar seu apreço pelo trabalho e motivar os desenvolvedores a continuar
trabalhando no Vim, envie uma doação.

O dinheiro que você doou será usado principalmente para ajudar crianças em
Uganda. Veja "runtime/doc/uganda.txt". Mas, ao mesmo tempo, as doações aumentam
a motivação da equipe de desenvolvimento para continuar trabalhando no Vim!

Para as informações mais recentes sobre patrocínio, consulte o site do Vim:
  https://www.vim.org/sponsor/

CONTRIBUIÇÕES

Se você gostaria de ajudar a tornar o Vim melhor, veja o arquivo CONTRIBUTING.md.

INFORMAÇÕES

Se você estiver no macOS, pode usar o MacVim: https://macvim.org

As últimas notícias sobre o Vim podem ser encontradas na página inicial do Vim:
  https://www.vim.org/

Se você tiver problemas, dê uma olhada na documentação ou dicas do Vim:
  https://www.vim.org/docs.php
  https://vim.fandom.com/wiki/Vim_Tips_Wiki

Se você ainda tiver problemas ou quaisquer outras perguntas, use uma das listas
de discussão para discuti-las com usuários e desenvolvedores do Vim:
  https://www.vim.org/maillist.php

Se nada mais funcionar, relate os bugs diretamente para a lista de discussão
vim-dev:
  <vim-dev@vim.org>

AUTOR PRINCIPAL

A maior parte do Vim foi criada por Bram Moolenaar <Bram@vim.org>,
":help Bram-Moolenaar"

Envie quaisquer outros comentários, patches, flores e sugestões para
a lista de discussão vim-dev: <vim-dev@vim.org>
