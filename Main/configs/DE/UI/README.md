# Spacegray.vim

Spacegray is a colorscheme for Vim loosely modeled after
the [spacegray](https://github.com/zdne/spacegray-xcode) theme for Xcode.

## Options

You can tweak Spacegray by enabling the following disabled options:

1. Underlined Search: Underline search text instead of using highlight color.
   Put the following in your `~/.vimrc` to enable it:

  > `let g:spacegray_underline_search = 1`

2. Use Italics: Use italics when appropriate, e.g. for comments. (_note_:
   terminal must support italics). Put the following in your `~/.vimrc` to
   enable it:

  > `let g:spacegray_use_italics = 1`

3. Use lower contrast: Use a low contrast variant of Spacegray. Put the
   following in your `~/.vimrc` to enable it:

  > `let g:spacegray_low_contrast = 1`

## Screenshots

### Syntax Groups
![Syntax Groups](screenshots/hl_groups.png)

### Spacegray Low Contrast
![Spacegray Low Contrast](screenshots/low_contrast.png)

### Spacegray Dark
![Spacegray Dark](screenshots/dark.png)

## Installation

If you use Vim 8 or better, simply copy and paste:

    git clone git://github.com/ajh17/Spacegray.vim ~/.vim/pack/vendor/start/Spacegray

Then in your ~/.vimrc, add this line:

    colorscheme spacegray

## Terminal Environment

If you use Spacegray inside a Terminal, please make sure you use a Terminal
with 256 color support. Most these days are. Ensure that the default TERM
contains the string `256color`. An example would be `xterm-256color` or
if using tmux or screen, `screen-256color`.

NOTE: If you use Vim 7.4.1778 or higher, you can now use Spacegray's GUI colors
inside terminal Vim as long as your terminal supports true colors (24-bit
colors). To enable this, put `:set termguicolors` and ignore the rest of the
terminal color sections of this document.

### Terminal Color Palette

Spacegray will look good in a dark terminal colorscheme, but if you use
Spacegray's color palette, it will look beautiful.

### Terminal Colorschemes

On OS X, colorschemes for iTerm2 and Terminal.app are provided with the download.
Simply double click to install.

### Terminator

Spacegray.terminator is provided for Terminator and can be installed by
copying to `~/.config/terminator/config` on Linux or
`$XDG_CONFIG_HOME/terminator/config` if you're running OS X.

### Gnome Terminal

For gnome terminal, you can configure the terminal with the following set of
gsettings commands:

    profile_key=$(gsettings get org.gnome.Terminal.ProfilesList default | sed -e "s/'//g" | tr -d "\n")
    gsettings set org.gnome.Terminal.Legacy.Profile:/org/gnome/terminal/legacy/profiles:/:$profile_key/ visible-name "'Spacegray'"
    gsettings set org.gnome.Terminal.Legacy.Profile:/org/gnome/terminal/legacy/profiles:/:$profile_key/ background-color "'rgb(17,19,20)'"
    gsettings set org.gnome.Terminal.Legacy.Profile:/org/gnome/terminal/legacy/profiles:/:$profile_key/ foreground-color "'rgb(183,187,183)'"
    gsettings set org.gnome.Terminal.Legacy.Profile:/org/gnome/terminal/legacy/profiles:/:$profile_key/ use-theme-colors "false"
    gsettings set org.gnome.Terminal.Legacy.Profile:/org/gnome/terminal/legacy/profiles:/:$profile_key/ palette "['rgb(44,47,51)', 'rgb(176,76,80)', 'rgb(145,150,82)', 'rgb(226,153,92)', 'rgb(102,137,157)', 'rgb(141,100,148)', 'rgb(82,124,119)', 'rgb(96,99,96)', 'rgb(75,80,86)', 'rgb(176,76,80)', 'rgb(148,152,91)', 'rgb(226,153,92)', 'rgb(102,137,157)', 'rgb(141,100,148)', 'rgb(82,124,119)', 'rgb(221,227,220)']"

gnome-terminal should then immediately reflect Spacegray colors.

### Xcode

Use Xcode? Try out [Spacegray-Xcode](https://github.com/ajh17/spacegray-xcode).

### Xresources

For Linux/BSD users, here is a sample ~/.Xresources:

    *background: #111314
    *foreground: #B7BBB7
    ! black
    *color0: #2C2F33
    *color8: #4B5056
    ! red
    *color1: #B04C50
    *color9: #B04C50
    ! green
    *color2: #919652
    *color10: #94985B
    ! yellow
    *color3: #E2995C
    *color11: #E2995C
    ! blue
    *color4: #66899D
    *color12: #66899D
    ! magenta
    *color5: #8D6494
    *color13: #8D6494
    ! cyan
    *color6: #527C77
    *color14: #527C77
    ! white
    *color7: #606360
    *color15: #DDE3DC

For lower contrast Spacegray, use a background color of #242424

# License
Copyright (c) Akshay Hegde. Distributed under the same terms as Vim itself. See `:help license`
