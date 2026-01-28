Vim: Vi IMproved 9.1 版本的 README.txt 文件


什 么 是 VIM ？

Vim 是经典 UNIX 编辑器 Vi 的一个极大改进版本。它新增了许多功能：多级撤销、语法高
亮、命令行历史、在线帮助、拼写检查、文件名补全、块操作、脚本语言等。同时也提供了
图形用户界面（GUI）。尽管如此，Vi 兼容性依然得以保留，习惯使用 Vi 的用户操作时仍
会感到得心应手。与 Vi 的差异请参阅 "runtime/doc/vi_diff.txt"。

此编辑器对于编辑代码和其他纯文本文件非常有用。所有命令都通过常规键盘字符输入，因
此熟练盲打的用户能够高效工作。此外，用户可以将功能键映射到命令，并且可以使用鼠标。

Vim 也致力于提供一个（基本）符合 POSIX 标准的 vi 实现。当它以最小功能集（通常称
为 vim.tiny）编译时，被许多 Linux 发行版用作默认的 vi 编辑器。

Vim 可在 MS-Windows (7, 8, 10, 11)、macOS、Haiku、VMS 以及几乎所有 UNIX 变体上运
行。移植到其他系统应该不太困难。旧版本的 Vim 曾在 Amiga DOS、Atari MiNT、BeOS、
MS-DOS、MS-Windows 95/98/Me/NT/2000/XP/Vista、RISC OS 和 OS/2 上运行。这些版本的
维护现已终止。


获 取 途 径

通常你可以使用你喜欢的软件包管理器来安装 Vim。在 Mac 和 Linux 上，会预装一个简化
版的 Vim，如果你需要更多功能，仍需要安装完整的 Vim。

有针对 Unix、PC、Amiga 和其他一些系统的独立发行版。本 README.txt 文件随运行时存
档一起提供。该存档包含文档、语法文件以及其他运行时使用的文件。要运行 Vim，你必须
获取二进制存档或源代码存档之一。您需要哪一种取决于您想要运行 Vim 的系统以及您是
否希望或必须自行编译。请查阅 "https://www.vim.org/download.php" 以了解当前可用的
发行版概览。

获取最新版 Vim 的常见方式：
* 从 github 检出 git 仓库：https://github.com/vim/vim。
* 以存档形式获取源代码：https://github.com/vim/vim/tags。
* 从 vim-win32-installer 仓库获取 Windows 可执行文件：
  https://github.com/vim/vim-win32-installer/releases。


编 译

如果你获得的是二进制发行版，则无需编译 Vim。如果你获得的是源代码发行版，编译 Vim
所需的所有内容都在 "src" 目录中。请参阅 src/INSTALL 文件中的说明。


安 装

请查阅以下文件之一以获取系统特定的安装说明。这些文件位于仓库中的 READMEdir 目录，
或者在你解压缩存档后的顶级目录中：

README_ami.txt		Amiga
README_unix.txt		Unix
README_dos.txt		MS-DOS 和 MS-Windows
README_mac.txt		Macintosh
README_haiku.txt	Haiku
README_vms.txt		VMS

根据你使用的发行版，可能还有其他 README_*.txt 文件。


文 档

Vim tutor 是为初学者设计的一小时培训课程。通常可以通过 "vimtutor" 命令启动。更多
信息请参阅 ":help tutor"。

最佳方式是在 Vim 中使用 ":help" 命令。如果您尚未安装可执行文件，请阅读
"runtime/doc/help.txt"。该文件包含指向其他文档文件的指引。用户手册采用书籍体例编
排，是学习使用 Vim 的推荐资料。具体请参阅 ":help user-manual"。


复 制 与 版 权

Vim 是慈善软件。您可以尽情使用和复制它，但鼓励您捐款以帮助乌干达的孤儿。请阅读
"runtime/doc/uganda.txt" 文件了解详情（在 Vim 中执行 ":help uganda"）。

许可摘要：对于未经修改的 Vim 副本，其使用或分发不受任何限制。Vim 的部分内容亦可
分发，但必须始终包含许可文本。对于修改版本，则需遵循若干限制条款。本许可证与 GPL
兼容，您可使用 GPL 库编译 Vim 并进行分发。


赞 助

修复错误与增添新功能均需投入大量时间与精力。为支持开发工作并激励开发者持续完善
Vim，敬请通过捐赠表达您的认可。

您捐赠的资金将主要用于帮助乌干达的儿童。请参阅 "runtime/doc/uganda.txt"。但同时，
您的捐赠也将激励开发团队持续投入 Vim 的开发工作。

关于赞助的最新信息，请查看 Vim 网站：
	https://www.vim.org/sponsor/


贡 献

如果您想帮助改进 Vim，请参阅 CONTRIBUTING.md 文件。


信 息 与 支 持

如果您在 macOS 上，可以使用 MacVim：https://macvim.org

关于 Vim 的最新消息可以在 Vim 主页上找到：
	https://www.vim.org/

如果您遇到问题，请查阅 Vim 文档或使用技巧：
	https://www.vim.org/docs.php
	https://vim.fandom.com/wiki/Vim_Tips_Wiki

如果您仍有问题或其他疑问，请使用其中一个邮件列表与 Vim 用户和开发者讨论：
	https://www.vim.org/maillist.php

如果其他方法都无效，请直接将错误报告发送到 vim-dev 邮件列表：
	<vim-dev@vim.org>


主 要 作 者

Vim 主要由 Bram Moolenaar <Bram@vim.org> 创建，可通过 ":help Bram-Moolenaar" 命
令了解更多信息。

请将任何其他评论、补丁、鲜花和建议发送到 vim-dev 邮件列表：<vim-dev@vim.org>
