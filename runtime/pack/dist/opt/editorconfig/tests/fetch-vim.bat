:: fetch-vim.bat: Fetch vim if necessary
:: For use in the editorconfig-vim Appveyor build
:: Copyright (c) 2018--2019 Chris White.  All rights reserved.
:: Licensed Apache 2.0, or any later version, at your option.

:: If it's already been loaded from the cache, we're done
if exist C:\vim\vim\vim80\vim.exe exit

:: Otherwise, download and unzip it.
appveyor DownloadFile https://github.com/cxw42/editorconfig-core-vimscript/releases/download/v0.1.0/vim.7z

7z x vim.7z -oC:\vim
