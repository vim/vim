" Vim plugin for converting a syntax highlighted file to HTML.
" Maintainer: Ben Fritz <fritzophrenic@gmail.com>
" Last Change: 2010 Oct 28
"
" The core of the code is in $VIMRUNTIME/autoload/tohtml.vim and
" $VIMRUNTIME/syntax/2html.vim
"
" TODO:
"   * Patch to leave tab characters in when noexpandtab set (Andy Spencer)
"   * Make folds show up when using a range and dynamic folding
"   * Remove fold column when there are no folds and using dynamic folding
"   * Restore open/closed folds and cursor position after processing each file
"     with option not to restore for speed increase
"   * Add extra meta info (generation time, etc.)
"   * Tidy up so we can use strict doctype in even more situations?
"   * Implementation detail: add threshold for writing the lines to the html
"     buffer before we're done (5000 or so lines should do it)
"   * TODO comments for code cleanup scattered throughout
"
"
" Changelog:
"   7.3_v7 (this version): see betas released on vim_dev below:
"                7.3_v7b3: Fixed bug, convert Unicode to UTF-8 all the way.
"                7.3_v7b2: Remove automatic detection of encodings that are not
"                          supported by all major browsers according to
"                          http://wiki.whatwg.org/wiki/Web_Encodings and convert
"                          to UTF-8 for all Unicode encodings. Make HTML
"                          encoding to Vim encoding detection be
"                          case-insensitive for built-in pairs.
"                7.3_v7b1: Remove use of setwinvar() function which cannot be
"                          called in restricted mode (Andy Spencer). Use
"                          'fencoding' instead of 'encoding' to determine by
"                          charset, and make sure the 'fenc' of the generated
"                          file matches its indicated charset. Add charsets for
"                          all of Vim's natively supported encodings.
"   7.3_v6 (0d3f0e3d289b): Really fix bug with 'nowrapscan', 'magic' and other
"                          user settings interfering with diff mode generation,
"                          trailing whitespace (e.g. line number column) when
"                          using html_no_pre, and bugs when using
"                          html_hover_unfold.
"   7.3_v5 ( unreleased ): Fix bug with 'nowrapscan' and also with out-of-sync
"                          folds in diff mode when first line was folded.
"   7.3_v4 (7e008c174cc3): Bugfixes, especially for xhtml markup, and diff mode.
"   7.3_v3 (a29075150aee): Refactor option handling and make html_use_css
"                          default to true when not set to anything. Use strict
"                          doctypes where possible. Rename use_xhtml option to
"                          html_use_xhtml for consistency. Use .xhtml extension
"                          when using this option. Add meta tag for settings.
"   7.3_v2 (80229a724a11): Fix syntax highlighting in diff mode to use both the
"                          diff colors and the normal syntax colors
"   7.3_v1 (e7751177126b): Add conceal support and meta tags in output
"   Pre-v1 baseline: Mercurial changeset 3c9324c0800e

if exists('g:loaded_2html_plugin')
  finish
endif
let g:loaded_2html_plugin = 'vim7.3_v7'

" Define the :TOhtml command when:
" - 'compatible' is not set
" - this plugin was not already loaded
" - user commands are available.
if !&cp && !exists(":TOhtml") && has("user_commands")
  command -range=% TOhtml :call tohtml#Convert2HTML(<line1>, <line2>)
endif

" Make sure any patches will probably use consistent indent
"   vim: ts=8 sw=2 sts=2 noet
