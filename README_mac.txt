README_mac.txt for version 7.4 of Vim: Vi IMproved.

This file explains the installation of Vim on Macintosh systems.
See "README.txt" for general information about Vim.

1. Clone vim from github and build its source by running
```
git clone https://github.com/vim/vim.git
cd vim/src
./configure 
make
```

2. Test vim by running `./vim`. This will open the version you just built
rather than the OSX binary version.
Make sure everything is working smoothly.

3. To use this installation as the default on your system you can
overwrite the OSX binary by running
```
sudo cp vim /usr/bin/vim
```
Otherwise, to keep the both versions of vim on your system you will probably
want to make an alias in you shell's rc.
```
alias vim7.4="/path/to/vim/src/vim"
```


