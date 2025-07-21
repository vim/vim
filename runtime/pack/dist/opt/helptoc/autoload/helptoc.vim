vim9script noclear

# the Vim HelpTOC plugin, creates a table of contents in a popup
# Maintainer: Vim project
# Original Author: @lacygoill
# Latest Change: 2025 Jul 10
#
# Config {{{1
# g:helptoc {{{2
# Create the g:helptoc dict (used to specify the shell_prompt and other
# options) when it does not exist
g:helptoc = exists('g:helptoc') ? g:helptoc : {}

# Set the initial shell_prompt pattern matching a default bash prompt
g:helptoc.shell_prompt = get(g:helptoc, 'shell_prompt', '^\w\+@\w\+:\f\+\$\s')

# Track the prior prompt (used to reset b:toc if 'shell_prompt' changes)
g:helptoc.prior_shell_prompt = g:helptoc.shell_prompt

def UpdateUserSettings() #{{{2

    if g:helptoc.shell_prompt != g:helptoc.prior_shell_prompt
        # invalidate cache: user config has changed
        unlet! b:toc
        # reset the prior prompt to the new prompt
        g:helptoc.prior_shell_prompt = g:helptoc.shell_prompt
    endif

    # helptoc popup presentation options{{{
    # Enable users to choose whether, in toc and help text popups, to have:
    # - border (default [], which is a border, so is usually wanted)
    # - borderchars (default single box drawing; use [] for Vim's defaults)
    # - borderhighlight (default [], but a user may prefer something else)
    # - close (default 'none'; mouse users may prefer 'button')
    # - drag (default true, which is a popup_menu's default)
    # - scrollbar (default false; for long tocs/HELP_TEXT true may be better)
    # For example, in a Vim9 script .vimrc, these settings will produce tocs
    # with borders that have the same highlight group as the inactive
    # statusline, a scrollbar, and an 'X' close button:
    # g:helptoc.popup_borderchars = get(g:helptoc, 'popup_borderchars', [' '])
    # g:helptoc.popup_borderhighlight = get(g:helptoc,
    #     'popup_borderhighlight', ['StatusLineNC'])
    # g:helptoc.popup_close = get(g:helptoc, 'popup_close', 'button')
    # g:helptoc.popup_scrollbar = get(g:helptoc, 'popup_scrollbar', true)
    # }}}
    g:helptoc.popup_border = get(g:helptoc, 'popup_border', [])
    g:helptoc.popup_borderchars = get(g:helptoc, 'popup_borderchars',
        ['─', '│', '─', '│', '┌', '┐', '┘', '└'])
    g:helptoc.popup_borderhighlight = get(g:helptoc, 'popup_borderhighlight',
        [])
    g:helptoc.popup_drag = get(g:helptoc, 'popup_drag', true)
    g:helptoc.popup_close = get(g:helptoc, 'popup_close', 'none')
    g:helptoc.popup_scrollbar = get(g:helptoc, 'popup_scrollbar', false)
    # For sanitized tocs, allow the user to specify the level indicator
    g:helptoc.level_indicator = get(g:helptoc, 'level_indicator', '| ')
enddef

UpdateUserSettings()

# Syntax {{{1

# Used by sanitized tocs (asciidoc, html, markdown, tex, vim, and xhtml)
def SanitizedTocSyntax(): void
    silent execute "syntax match helptocLevel _^\\(" ..
        g:helptoc.level_indicator .. "\\)*_ contained"
    silent execute "syntax region helptocText start=_^\\(" ..
        g:helptoc.level_indicator .. "\\)*_ end=_$_ contains=helptocLevel"
    highlight link helptocText Normal
    highlight link helptocLevel NonText
enddef

# Init {{{1
# Constants {{{2
# HELP_TEXT {{{3
const HELP_TEXT: list<string> =<< trim END
    normal commands in help window
    ──────────────────────────────
    ?      hide this help window
    <C-J>  scroll down one line
    <C-K>  scroll up one line

    normal commands in TOC menu
    ───────────────────────────
    j      select next entry
    k      select previous entry
    J      same as j, and jump to corresponding line in main buffer
    K      same as k, and jump to corresponding line in main buffer
    c      select nearest entry from cursor position in main buffer
    g      select first entry
    G      select last entry
    H      collapse one level
    L      expand one level
    p      print selected entry on command-line

    P      same as p but automatically, whenever selection changes
           press multiple times to toggle feature on/off

    q      quit menu
    z      redraw menu with selected entry at center
    +      increase width of popup menu
    -      decrease width of popup menu
    /      look for given text with fuzzy algorithm
    ?      show help window

    <C-D>       scroll down half a page
    <C-U>       scroll up half a page
    <PageUp>    scroll down a whole page
    <PageDown>  scroll up a whole page
    <Home>      select first entry
    <End>       select last entry

    title meaning
    ─────────────
    example: 12/34 (5/6)
    broken down:

        12  index of selected entry
        34  index of last entry
         5  index of deepest level currently visible
         6  index of maximum possible level

    tip
    ───
    after inserting a pattern to look for with the / command,
    if you press <Esc> instead of <CR>, you can then get
    more context for each remaining entry by pressing J or K
END

# UPTOINC_H {{{3
const UPTOINC_H: string = '\v\c^%(%([<][^h][^>]*[>])|\s)*[<]h'

# MATCH_ENTRY {{{3
const MATCH_ENTRY: dict<dict<func: bool>> = {

    help: {},

    # For asciidoc, these patterns should match:
    # https://docs.asciidoctor.org/asciidoc/latest/sections/titles-and-levels/
    asciidoc: {
        1: (l: string, _): bool => l =~ '\v^%(\=|#)\s',
        2: (l: string, _): bool => l =~ '\v^%(\={2}|#{2})\s',
        3: (l: string, _): bool => l =~ '\v^%(\={3}|#{3})\s',
        4: (l: string, _): bool => l =~ '\v^%(\={4}|#{4})\s',
        5: (l: string, _): bool => l =~ '\v^%(\={5}|#{5})\s',
        6: (l: string, _): bool => l =~ '\v^%(\={6}|#{6})\s',
    },

    html: {
        1: (l: string, _): bool => l =~ $"{UPTOINC_H}1",
        2: (l: string, _): bool => l =~ $"{UPTOINC_H}2",
        3: (l: string, _): bool => l =~ $"{UPTOINC_H}3",
        4: (l: string, _): bool => l =~ $"{UPTOINC_H}4",
        5: (l: string, _): bool => l =~ $"{UPTOINC_H}5",
        6: (l: string, _): bool => l =~ $"{UPTOINC_H}6",
    },

    man: {
        1: (l: string, _): bool => l =~ '^\S',
        2: (l: string, _): bool => l =~ '\v^%( {3})=\S',
        3: (l: string, _): bool => l =~ '\v^\s+%(%(\+|-)\S+,\s+)*(\+|-)\S+'
    },

    # For markdown, these patterns should match:
    # https://spec.commonmark.org/0.31.2/#atx-headings and
    # https://spec.commonmark.org/0.31.2/#setext-headings
    markdown: {
        1: (l: string, nextline: string): bool =>
            (l =~ '\v^ {0,3}#%(\s|$)' || nextline =~ '\v^ {0,3}\=+$') &&
            l =~ '\S',
        2: (l: string, nextline: string): bool =>
            (l =~ '\v^ {0,3}##%(\s|$)' || nextline =~ '\v^ {0,3}-+$') &&
            l =~ '\S',
        3: (l: string, _): bool => l =~ '\v {0,3}#{3}%(\s|$)',
        4: (l: string, _): bool => l =~ '\v {0,3}#{4}%(\s|$)',
        5: (l: string, _): bool => l =~ '\v {0,3}#{5}%(\s|$)',
        6: (l: string, _): bool => l =~ '\v {0,3}#{6}%(\s|$)',
    },

    terminal: {
        1: (l: string, _): bool => l =~ g:helptoc.shell_prompt
    },

    # For LaTeX, this should meet
    # https://mirrors.rit.edu/CTAN/info/latex2e-help-texinfo/latex2e.pdf
    #   including:
    #   para 6.3:
    #     \section{Heading}
    #     \section[Alternative ToC Heading]{Heading}
    #   para 25.1.2:
    #     \section*{Not for the TOC heading}
    #     \addcontentsline{toc}{section}{Alternative ToC Heading}
    tex: {
        1: (l: string, _): bool => l =~ '^[\\]\(\%(part\|chapter\)' ..
            '\%([\u005B{]\)\|addcontentsline{toc}{\%(part\|chapter\)\)',
        2: (l: string, _): bool => l =~ '^[\\]\%(section' ..
            '\%([\u005B{]\)\|addcontentsline{toc}{section}\)',
        3: (l: string, _): bool => l =~ '^[\\]\%(subsection' ..
            '\%([\u005B{]\)\|addcontentsline{toc}{subsection}\)',
        4: (l: string, _): bool => l =~ '^[\\]\%(subsubsection' ..
            '\%([\u005B{]\)\|addcontentsline{toc}{subsubsection}\)',
    },

    vim: {
        1: (l: string, _): bool => l =~ '\v\{{3}1',
        2: (l: string, _): bool => l =~ '\v\{{3}2',
        3: (l: string, _): bool => l =~ '\v\{{3}3',
        4: (l: string, _): bool => l =~ '\v\{{3}4',
        5: (l: string, _): bool => l =~ '\v\{{3}5',
        6: (l: string, _): bool => l =~ '\v\{{3}6',
    },

    xhtml: {
        1: (l: string, _): bool => l =~ $"{UPTOINC_H}1",
        2: (l: string, _): bool => l =~ $"{UPTOINC_H}2",
        3: (l: string, _): bool => l =~ $"{UPTOINC_H}3",
        4: (l: string, _): bool => l =~ $"{UPTOINC_H}4",
        5: (l: string, _): bool => l =~ $"{UPTOINC_H}5",
        6: (l: string, _): bool => l =~ $"{UPTOINC_H}6",
    }
}

# HELP_RULERS {{{3
const HELP_RULERS: dict<string> = {
    '=': '^=\{40,}$',
    '-': '^-\{40,}',
}
const HELP_RULER: string = HELP_RULERS->values()->join('\|')

# HELP_TAG {{{3
# The regex is copied from the help syntax plugin
const HELP_TAG: string = '\*[#-)!+-~]\+\*\%(\s\|$\)\@='

# Adapted from `$VIMRUNTIME/syntax/help.vim`.{{{
#
# The original regex is:
#
#     ^[-A-Z .][-A-Z0-9 .()_]*\ze\(\s\+\*\|$\)
#
# Allowing a space or a hyphen at the start can give false positives, and is
# useless, so we don't allow them.
#}}}

# HELP_HEADLINE {{{3
const HELP_HEADLINE: string = '^\C[A-Z.][-A-Z0-9 .()_]*\%(\s\+\*+\@!\|$\)'
#                                                               ^--^
# To prevent some false positives under `:help feature-list`.
# Others {{{2
var lvls: dict<number>
def InitHelpLvls()
    lvls = {
        '*01.1*': 0,
        '1.': 0,
        '1.2': 0,
        '1.2.3': 0,
        'header ~': 0,
        HEADLINE: 0,
        tag: 0,
    }
enddef

var fuzzy_entries: list<dict<any>>
var help_winid: number
var print_entry: bool
var selected_entry_match: number

# Interface {{{1
export def Open() #{{{2
    g:helptoc.type = GetType()
    if !MATCH_ENTRY->has_key(g:helptoc.type)
        return
    endif
    if g:helptoc.type == 'terminal' && win_gettype() == 'popup'
        # trying to deal with a popup menu on top of a popup terminal seems
        # too tricky for now
        echomsg 'does not work in a popup window; only in a regular window'
        return
    endif

    UpdateUserSettings()

    # invalidate the cache if the buffer's contents has changed
    if exists('b:toc') && &filetype != 'man'
        if b:toc.changedtick != b:changedtick
        # in a terminal buffer, `b:changedtick` does not change
        || g:helptoc.type == 'terminal' && line('$') > b:toc.linecount
            unlet! b:toc
        endif
    endif

    if !exists('b:toc')
        SetToc()
    endif

    var winpos: list<number> = winnr()->win_screenpos()
    var height: number = winheight(0) - 2
    var width: number = winwidth(0)
    b:toc.width = b:toc.width ?? width / 3
    # the popup needs enough space to display the help message in its title
    if b:toc.width < 30
        b:toc.width = 30
    endif
    # Is `popup_menu()` OK with a list of dictionaries?{{{
    #
    # Yes, see `:help popup_create-arguments`.
    # Although, it expects dictionaries with the keys `text` and `props`.
    # But we use dictionaries with the keys `text` and `lnum`.
    # IOW, we abuse the feature which lets us use text properties in a popup.
    #}}}
    var winid: number = GetTocEntries()
        ->popup_menu({
            line: winpos[0],
            col: winpos[1] + width - 1,
            pos: 'topright',
            highlight: g:helptoc.type == 'terminal' ? 'Terminal' : 'Normal',
            minheight: height,
            maxheight: height,
            minwidth: b:toc.width,
            maxwidth: b:toc.width,
            filter: Filter,
            callback: Callback,
            border: g:helptoc.popup_border,
            borderchars: g:helptoc.popup_borderchars,
            borderhighlight: g:helptoc.popup_borderhighlight,
            close: g:helptoc.popup_close,
            drag: g:helptoc.popup_drag,
            scrollbar: g:helptoc.popup_scrollbar,
        })
    # Specify filetypes using sanitized toc syntax{{{
    #   Those filetypes have a normalized toc structure.  The top level is
    #   unprefixed and levels 2 to 6 are prefixed, by default, with a vertical
    #   line and space for each level below 1:
    #   Level 1
    #   | Level 2
    #   ...
    #   | | | | | Level 6  }}}
    final SanitizedTocSyntaxTypes: list<string> =
        ['asciidoc', 'html', 'markdown', 'tex', 'vim', 'xhtml']
    if index(SanitizedTocSyntaxTypes, g:helptoc.type) != -1
        # Specified types' toc popups use a common syntax
        Win_execute(winid, 'SanitizedTocSyntax()')
    else
        # Other types' toc popups use the same syntax as the buffer itself
        Win_execute(winid, [$'ownsyntax {&filetype}', '&l:conceallevel = 3'])
    endif
    # In a help file, we might reduce some noisy tags to a trailing asterisk.
    # Hide those.
    if g:helptoc.type == 'help'
        matchadd('Conceal', '\*$', 0, -1, {window: winid})
    endif
    SelectNearestEntryFromCursor(winid)

    # Can't set the title before jumping to the relevant line, otherwise the
    # indicator in the title might be wrong
    SetTitle(winid)
enddef

# Core {{{1
def SetToc() #{{{2
    # Lambdas:
    # CHARACTER_REFERENCES_TO_CHARACTERS {{{3
    # These are used for AsciiDoc, Markdown, and [X]HTML, all of which allow
    # for decimal, hexadecimal, and XML predefined entities.
    #    Decimal character references: e.g., &#167; to §
    #    Hexadecimal character references: e.g., &#xA7; to §
    #    XML predefined entities to chars: e.g., &lt; to <
    # All HTML5 named character references could be handled, though is that
    # warranted for the few that may appear in a toc entry, especially when
    # they are often mnemonic?  Future: A common Vim dict/enum could be useful?
    const CHARACTER_REFERENCES_TO_CHARACTERS = (text: string): string =>
        text->substitute('\v\&#0*([1-9]\d{0,6});',
                '\=nr2char(str2nr(submatch(1), 10), 1)', 'g')
            ->substitute('\c\v\&#x0*([1-9a-f][[:xdigit:]]{1,5});',
                '\=nr2char(str2nr(submatch(1), 16), 1)', 'g')
            ->substitute('\C&amp;', '\="\u0026"', 'g')
            ->substitute('\C&apos;', "\u0027", 'g')
            ->substitute('\C&gt;', "\u003E", 'g')
            ->substitute('\C&lt;', "\u003C", 'g')
            ->substitute('\C&quot;', "\u0022", 'g')

    # SANITIZE_ASCIIDOC {{{3
    # 1 - Substitute the = or # heading markup with the level indicator
    # 2 - Substitute XML predefined, dec, and hex char refs in the entry
    #     AsciiDoc recommends only using named char refs defined in XML:
    #     https://docs.asciidoctor.org/asciidoc/latest/subs/replacements/
    const SANITIZE_ASCIIDOC = (text: string): string =>
        text->substitute('\v^(\={1,6}|#{1,6})\s+',
            '\=repeat(g:helptoc.level_indicator, len(submatch(1)) - 1)', '')
            ->CHARACTER_REFERENCES_TO_CHARACTERS()

    # SANITIZE_HTML {{{3
    #  1 - Remove any leading spaces or tabs
    #  2 - Remove any <!--HTML comments-->
    #  3 - Remove any <?processing_instructions?>
    #  4 - Remove any leading tags (and any blanks) other than <h1 to <h6
    #  5 - Remove any persisting leading blanks
    #  6 - Handle empty XHTML headings, e.g., <h6 />
    #  7 - Remove trailing content following the </h[1-6]>
    #  8 - Remove the <h1
    #  9 - Substitute the h2 to h6 heading tags with level indicator/level
    # 10 - Remove intra-heading tags like <em>, </em>, <strong>, etc.
    # 11 - Substitute XML predefined, dec and hex character references
    const SANITIZE_HTML = (text: string): string =>
        text->substitute('^\s*', '', '')
            ->substitute('[<]!--.\{-}--[>]', '', 'g')
            ->substitute('[<]?[^?]\+?[>]', '', 'g')
            ->substitute('\v%([<][^Hh][^1-6]?[^>][>])*\s*', '', '')
            ->substitute('^\s\+', '', '')
            ->substitute('\v[<][Hh]([1-6])\s*[/][>].*',
                '\=repeat(g:helptoc.level_indicator, ' ..
                'str2nr(submatch(1)) - 1) ' ..
                '.. "[Empty heading " .. submatch(1) .. "]"', '')
            ->substitute('[<][/][Hh][1-6][>].*$', '', '')
            ->substitute('[<][Hh]1[^>]*[>]', '', '')
            ->substitute('\v[<][Hh]([2-6])[^>]*[>]',
                '\=repeat(g:helptoc.level_indicator, ' ..
                'str2nr(submatch(1)) - 1)', '')
            ->substitute('[<][/]\?[[:alpha:]][^>]*[>]', '', 'g')
            ->CHARACTER_REFERENCES_TO_CHARACTERS()

    # SANITIZE_MARKDOWN #{{{3
    # 1 - Hyperlink incl image, e.g. [![Vim The editor](xxx)](\uri), to Vim...
    # 2 - Hyperlink [text](/uri) to text
    # 3 - Substitute the # ATX heading markup with the level indicator/level
    #     The omitted markup reflects CommonMark Spec:
    #     https://spec.commonmark.org/0.31.2/#atx-headings
    # 4 - Substitute decimal, hexadecimal, and XML predefined char refs
    const SANITIZE_MARKDOWN = (text: string): string =>
        text->substitute('\v[\u005B]![\u005B]([^\u005D]+)[\u005D]'
                .. '[(][^)]+[)][\u005D][(][^)]+[)]', '\1', '')
            ->substitute('\v[\u005B]([^\u005D]+)[\u005D][(][^)]+[)]',
                '\1', '')
            ->substitute('\v^ {0,3}(#{1,6})\s*',
                '\=repeat(g:helptoc.level_indicator, len(submatch(1)) - 1)',
                '')
            ->CHARACTER_REFERENCES_TO_CHARACTERS()

    # SANITIZE_TERMINAL {{{3
    # Omit the prompt, which may be very long and otherwise just adds clutter
    const SANITIZE_TERMINAL = (text: string): string =>
        text->substitute('^' .. g:helptoc.shell_prompt, '', '')

    # SANITIZE_TEX #{{{3
    # 1 - Use any [toc-title] overrides to move its content into the
    #     {heading} instead of the (non-ToC) heading's text
    # 2 - Replace \part{ or \addcontentsline{toc}{part} with '[PART] '
    # 3 - Omit \chapter{ or \addcontentsline{toc}{chapter}
    # 4 - Omit \section{ or \addcontentsline{toc}{section}
    # 5 - Omit \subsection{ or \addcontentsline{toc}{subsection}
    # 6 - Omit \subsubsection{ or \addcontentsline{toc}{subsubsection}
    # 7 - Omit the trailing }
    # 8 - Unescape common escaped characters &%$_#{}~^\
    const SANITIZE_TEX = (text: string): string =>
        text->substitute('\v^[\\](part|chapter|%(sub){0,2}section)' ..
                '[\u005B]([^\u005D]+).*', '\\\1{\2}', '')
            ->substitute('^[\\]\(part\|addcontentsline{toc}{part}\){',
                '[PART] ', '')
            ->substitute('^[\\]\(chapter\|addcontentsline{toc}{chapter}\){',
                '', '')
            ->substitute('^[\\]\(section\|addcontentsline{toc}{section}\){',
                '\=g:helptoc.level_indicator', '')
            ->substitute('^[\\]\(subsection\|' ..
                'addcontentsline{toc}{subsection}\){',
                '\=repeat(g:helptoc.level_indicator, 2)', '')
            ->substitute('^[\\]\(subsubsection\|' ..
                'addcontentsline{toc}{subsubsection}\){',
                '\=repeat(g:helptoc.level_indicator, 3)', '')
            ->substitute('}[^}]*$', '', '')
            ->substitute('\\\([&%$_#{}~\\^]\)', '\1', 'g')

    # SANITIZE_VIM {{{3
    # #1 - Omit leading Vim9 script # or vimscript " markers and blanks
    # #2 - Omit numbered 3x { markers
    const SANITIZE_VIM = (text: string): string =>
        text->substitute('\v^[#[:blank:]"]*(.+)\ze[{]{3}([1-6])',
                '\=submatch(2) == "1" ? submatch(1) : ' ..
                'repeat(g:helptoc.level_indicator, str2nr(submatch(2)) - 1)' ..
                ' .. submatch(1)', 'g')
            ->substitute('[#[:blank:]"]*{\{3}[1-6]', '', '')
    #}}}3

    final toc: dict<any> = {entries: []}
    toc.changedtick = b:changedtick
    if !toc->has_key('width')
        toc.width = 0
    endif
    # We cache the toc in `b:toc` to get better performance.{{{
    #
    # Without caching, when we press `H`, `L`, `H`, `L`, ... quickly for a few
    # seconds, there is some lag if we then try to move with `j` and `k`.
    # This can only be perceived in big man pages like with `:Man ffmpeg-all`.
    #}}}
    b:toc = toc

    if g:helptoc.type == 'help'
        SetTocHelp()
        return
    endif

    if g:helptoc.type == 'terminal'
        b:toc.linecount = line('$')
    endif

    var curline: string = getline(1)
    var nextline: string
    var lvl_and_test: list<list<any>> = MATCH_ENTRY
        ->get(g:helptoc.type, {})
        ->items()
        ->sort((l: list<any>, ll: list<any>): number =>
            l[0]->str2nr() - ll[0]->str2nr())

    var skip_next: bool = false
    var skip_fence: bool = false

    # Non-help headings processing
    for lnum: number in range(1, line('$'))
        if skip_next
            skip_next = false
            curline = nextline
            continue
        endif

        nextline = getline(lnum + 1)

        # Special handling for markdown filetype using setext headings
        if g:helptoc.type == 'markdown'
            # ignore fenced codeblock lines
            if curline =~ '^```.'
                skip_fence = true
            elseif curline =~ '^```$'
                skip_fence = !skip_fence
            endif
            if skip_fence
                curline = nextline
                continue
            endif
            # Check for setext formatted headings (= or - underlined)
            if nextline =~ '^\s\{0,3}=\+$' && curline =~ '\S'
                # Level 1 heading (one or more =, up to three spaces preceding)
                b:toc.entries->add({
                    lnum: lnum,
                    lvl: 1,
                    text: SANITIZE_MARKDOWN('# ' .. trim(curline)),
                })
                skip_next = true
                curline = nextline
                continue
            elseif nextline =~ '^\s\{0,3}-\+$' && curline =~ '\S'
                # Level 2 heading (one or more -, up to three spaces preceding)
                b:toc.entries->add({
                    lnum: lnum,
                    lvl: 2,
                    text: SANITIZE_MARKDOWN('## ' .. trim(curline)),
                })
                skip_next = true
                curline = nextline
                continue
            endif
        endif

        # Regular processing for markdown ATX-style headings + other filetypes
        for [lvl: string, IsEntry: func: bool] in lvl_and_test
            if IsEntry(curline, nextline)
                if g:helptoc.type == 'asciidoc'
                    curline = curline->SANITIZE_ASCIIDOC()
                elseif g:helptoc.type == 'html' || g:helptoc.type == 'xhtml'
                    curline = curline->SANITIZE_HTML()
                elseif g:helptoc.type == 'markdown'
                    curline = curline->SANITIZE_MARKDOWN()
                elseif g:helptoc.type == 'terminal'
                    curline = curline->SANITIZE_TERMINAL()
                elseif g:helptoc.type == 'tex'
                    curline = curline->SANITIZE_TEX()
                elseif g:helptoc.type == 'vim'
                    curline = curline->SANITIZE_VIM()
                endif
                b:toc.entries->add({
                    lnum: lnum,
                    lvl: lvl->str2nr(),
                    text: curline,
                })
                break
            endif
        endfor
        curline = nextline
    endfor

    InitMaxAndCurLvl()
enddef

def SetTocHelp() #{{{2
    var main_ruler: string
    for line: string in getline(1, '$')
        if line =~ HELP_RULER
            main_ruler = line =~ '=' ? HELP_RULERS['='] : HELP_RULERS['-']
            break
        endif
    endfor

    var prevline: string
    var curline: string = getline(1)
    var nextline: string
    var in_list: bool
    var last_numbered_entry: number
    InitHelpLvls()
    for lnum: number in range(1, line('$'))
        nextline = getline(lnum + 1)

        if main_ruler != '' && curline =~ main_ruler
            last_numbered_entry = 0
            # The information gathered in `lvls` might not be applicable to
            # all the main sections of a help file.  Let's reset it whenever
            # we find a ruler.
            InitHelpLvls()
        endif

        # Do not assume that a list ends on an empty line.
        # See the list at `:help gdb` for a counter-example.
        if in_list
        && curline !~ '^\d\+.\s'
        && curline !~ '^\s*$'
        && curline !~ '^[< \t]'
            in_list = false
        endif

        if prevline =~ '^\d\+\.\s'
        && curline !~ '^\s*$'
        && curline !~ $'^\s*{HELP_TAG}'
            in_list = true
        endif

        # 1.
        if prevline =~ '^\d\+\.\s'
        # Let's assume that the start of a main entry is always followed by an
        # empty line, or a line starting with a tag
        && (curline =~ '^>\=\s*$' || curline =~ $'^\s*{HELP_TAG}')
        # ignore a numbered line in a list
        && !in_list
            var current_numbered_entry: number = prevline
                ->matchstr('^\d\+\ze\.\s')
                ->str2nr()
            if current_numbered_entry > last_numbered_entry
                AddEntryInTocHelp('1.', lnum - 1, prevline)
                last_numbered_entry = prevline
                    ->matchstr('^\d\+\ze\.\s')
                    ->str2nr()
            endif
        endif

        # 1.2
        if curline =~ '^\d\+\.\d\+\s'
            if curline =~ $'\%({HELP_TAG}\s*\|\~\)$'
            || (prevline =~ $'^\s*{HELP_TAG}' || nextline =~ $'^\s*{HELP_TAG}')
            || (prevline =~ HELP_RULER || nextline =~ HELP_RULER)
            || (prevline =~ '^\s*$' && nextline =~ '^\s*$')
                AddEntryInTocHelp('1.2', lnum, curline)
            endif
        # 1.2.3
        elseif curline =~ '^\s\=\d\+\.\d\+\.\d\+\s'
            AddEntryInTocHelp('1.2.3', lnum, curline)
        endif

        # HEADLINE
        if curline =~ HELP_HEADLINE
        && curline !~ '^CTRL-'
        &&  prevline->IsSpecialHelpLine()
        && (nextline ->IsSpecialHelpLine()
            || nextline =~ '^\s*(\|^\t\|^N[oO][tT][eE]:')
            AddEntryInTocHelp('HEADLINE', lnum, curline)
        endif

        # header ~
        if curline =~ '\~$'
        && curline =~ '\w'
        && curline !~ '^[ \t<]\|\t\|---+---\|^NOTE:'
        && curline !~ '^\d\+\.\%(\d\+\%(\.\d\+\)\=\)\=\s'
        && prevline !~ $'^\s*{HELP_TAG}'
        && prevline !~ '\~$'
        && nextline !~ '\~$'
            AddEntryInTocHelp('header ~', lnum, curline)
        endif

        # *some_tag*
        if curline =~ HELP_TAG
            AddEntryInTocHelp('tag', lnum, curline)
        endif

        # In the Vim user manual, a main section is a special case.{{{
        #
        # It's not a simple numbered section:
        #
        #     01.1
        #
        # It's used as a tag:
        #
        #     *01.1*  Two manuals
        #     ^    ^
        #}}}
        if prevline =~ main_ruler && curline =~ '^\*\d\+\.\d\+\*'
            AddEntryInTocHelp('*01.1*', lnum, curline)
        endif

        [prevline, curline] = [curline, nextline]
    endfor

    # let's ignore the tag on the first line (not really interesting)
    if b:toc.entries->get(0, {})->get('lnum') == 1
        b:toc.entries->remove(0)
    endif

    # let's also ignore anything before the first `1.` line
    var i: number = b:toc.entries
        ->copy()
        ->map((_, entry: dict<any>) => entry.text)
        ->match('^\s*1\.\s')
    if i > 0
        b:toc.entries->remove(0, i - 1)
    endif

    InitMaxAndCurLvl()

    # set level of tag entries to the deepest level
    var has_tag: bool = b:toc.entries
        ->copy()
        ->map((_, entry: dict<any>) => entry.text)
        ->match(HELP_TAG) >= 0
    if has_tag
        ++b:toc.maxlvl
    endif
    b:toc.entries
        ->map((_, entry: dict<any>) => entry.lvl == 0
            ? entry->extend({lvl: b:toc.maxlvl})
            : entry)

    # fix indentation
    var min_lvl: number = b:toc.entries
        ->copy()
        ->map((_, entry: dict<any>) => entry.lvl)
        ->min()
    for entry: dict<any> in b:toc.entries
        entry.text = entry.text
            ->substitute('^\s*', () =>
                repeat(' ', (entry.lvl - min_lvl) * 3), '')
    endfor
enddef

def AddEntryInTocHelp(type: string, lnum: number, line: string) #{{{2
    # don't add a duplicate entry
    if lnum == b:toc.entries->get(-1, {})->get('lnum')
        # For a numbered line containing a tag, *do* add an entry.
        # But only for its numbered prefix, not for its tag.
        # The former is the line's most meaningful representation.
        if b:toc.entries->get(-1, {})->get('type') == 'tag'
            b:toc.entries->remove(-1)
        else
            return
        endif
    endif

    var text: string = line
    if type == 'tag'
        var tags: list<string>
        text->substitute(HELP_TAG, () => !!tags->add(submatch(0)), 'g')
        text = tags
            # we ignore errors and warnings because those are meaningless in
            # a TOC where no context is available
            ->filter((_, tag: string) => tag !~ '\*[EW]\d\+\*')
            ->join()
        if text !~ HELP_TAG
            return
        endif
    endif

    var maxlvl: number = lvls->values()->max()
    if type == 'tag'
        lvls[type] = 0
    elseif type == '1.2'
        lvls[type] = lvls[type] ?? lvls->get('1.', maxlvl) + 1
    elseif type == '1.2.3'
        lvls[type] = lvls[type] ?? lvls->get('1.2', maxlvl) + 1
    else
        lvls[type] = lvls[type] ?? maxlvl + 1
    endif

    # Ignore noisy tags.{{{
    #
    #     14. Linking groups        *:hi-link* *:highlight-link* *E412* *E413*
    #                               ^----------------------------------------^
    #                               ^\s*\d\+\.\%(\d\+\.\=\)*\s\+.\{-}\zs\*.*
    # ---
    #
    # We don't use conceal because then, `matchfuzzypos()` could match
    # concealed characters, which would be confusing.
    #}}}
    #     MAKING YOUR OWN SYNTAX FILES                  *mysyntaxfile*
    #                                                   ^------------^
    #                                                   ^\s*[A-Z].\{-}\*\zs.*
    #
    var after_HEADLINE: string = '^\s*[A-Z].\{-}\*\zs.*'
    #     14. Linking groups       *:hi-link* *:highlight-link* *E412* *E413*
    #                              ^----------------------------------------^
    #                              ^\s*\d\+\.\%(\d\+\.\=\)*\s\+.\{-}\*\zs.*
    var after_numbered: string = '^\s*\d\+\.\%(\d\+\.\=\)*\s\+.\{-}\*\zs.*'
    #     01.3    Using the Vim tutor                      *tutor* *vimtutor*
    #                                                      ^----------------^
    var after_numbered_tutor: string = '^\*\d\+\.\%(\d\+\.\=\)*.\{-}\t\*\zs.*'
    var noisy_tags: string =
        $'{after_HEADLINE}\|{after_numbered}\|{after_numbered_tutor}'
    text = text->substitute(noisy_tags, '', '')
    # We don't remove the trailing asterisk, because the help syntax plugin
    # might need it to highlight some headlines.

    b:toc.entries->add({
        lnum: lnum,
        lvl: lvls[type],
        text: text,
        type: type,
    })
enddef

def InitMaxAndCurLvl() #{{{2
    b:toc.maxlvl = b:toc.entries
        ->copy()
        ->map((_, entry: dict<any>) => entry.lvl)
        ->max()
    b:toc.curlvl = b:toc.maxlvl
enddef

def Popup_settext(winid: number, entries: list<dict<any>>) #{{{2
    var text: list<any>
    # When we fuzzy search the toc, the dictionaries in `entries` contain a
    # `props` key, to highlight each matched character individually.
    # We don't want to process those dictionaries further.
    # The processing should already have been done by the caller.
    if entries->get(0, {})->has_key('props')
        text = entries
    else
        text = entries
            ->copy()
            ->map((_, entry: dict<any>): string => entry.text)
    endif
    popup_settext(winid, text)
    SetTitle(winid)
    redraw
enddef

def SetTitle(winid: number) #{{{2
    var curlnum: number
    var lastlnum: number = line('$', winid)
    var is_empty: bool = lastlnum == 1
        && winid->winbufnr()->getbufoneline(1) == ''
    if is_empty
        [curlnum, lastlnum] = [0, 0]
    else
        curlnum = line('.', winid)
    endif
    var newtitle: string = printf(' %*d/%d (%d/%d)',
        len(lastlnum), curlnum,
        lastlnum,
        b:toc.curlvl,
        b:toc.maxlvl,
    )

    var width: number = winid->popup_getoptions().minwidth
    newtitle = printf('%s%*s',
        newtitle,
        width - newtitle->strlen(),
        'press ? for help ')

    popup_setoptions(winid, {title: newtitle})
enddef

def SelectNearestEntryFromCursor(winid: number) #{{{2
    var lnum: number = line('.')
    var firstline: number = b:toc.entries
        ->copy()
        ->filter((_, line: dict<any>): bool =>
            line.lvl <= b:toc.curlvl && line.lnum <= lnum)
        ->len()
    if firstline == 0
        return
    endif
    Win_execute(winid, $'normal! {firstline}Gzz')
enddef

def Filter(winid: number, key: string): bool #{{{2
    # support various normal commands for moving/scrolling
    if [
        'j', 'J', 'k', 'K', "\<Down>", "\<Up>", "\<C-N>", "\<C-P>",
        "\<C-D>", "\<C-U>",
        "\<PageUp>", "\<PageDown>",
        'g', 'G', "\<Home>", "\<End>",
        'z'
       ]->index(key) >= 0
        var scroll_cmd: string = {
            J: 'j',
            K: 'k',
            g: '1G',
            "\<Home>": '1G',
            "\<End>": 'G',
            z: 'zz'
        }->get(key, key)

        var old_lnum: number = line('.', winid)
        Win_execute(winid, $'normal! {scroll_cmd}')
        var new_lnum: number = line('.', winid)

        if print_entry
            PrintEntry(winid)
        endif

        # wrap around the edges
        if new_lnum == old_lnum
            scroll_cmd = {
                j: '1G',
                J: '1G',
                k: 'G',
                K: 'G',
                "\<Down>": '1G',
                "\<Up>": 'G',
                "\<C-N>": '1G',
                "\<C-P>": 'G',
            }->get(key, '')
            if !scroll_cmd->empty()
                Win_execute(winid, $'normal! {scroll_cmd}')
            endif
        endif

        # move the cursor to the corresponding line in the main buffer
        if key == 'J' || key == 'K'
            var lnum: number = GetBufLnum(winid)
            execute $'normal! 0{lnum}zt'
            # Install a match in the regular buffer to highlight the position
            # of the entry in the latter
            MatchDelete()
            selected_entry_match = matchaddpos('IncSearch', [lnum], 0, -1)
        endif
        SetTitle(winid)

        return true

    elseif key == 'c'
        SelectNearestEntryFromCursor(winid)
        return true

    # when we press `p`, print the selected line (useful when it's truncated)
    elseif key == 'p'
        PrintEntry(winid)
        return true

    # same thing, but automatically
    elseif key == 'P'
        print_entry = !print_entry
        if print_entry
            PrintEntry(winid)
        else
            echo ''
        endif
        return true

    elseif key == 'q'
        popup_close(winid, -1)
        return true

    elseif key == '?'
        ToggleHelp(winid)
        return true

    # scroll help window
    elseif key == "\<C-J>" || key == "\<C-K>"
        var scroll_cmd: string = {"\<C-J>": 'j', "\<C-K>": 'k'}->get(key, key)
        if scroll_cmd == 'j' && line('.', help_winid) == line('$', help_winid)
            scroll_cmd = '1G'
        elseif scroll_cmd == 'k' && line('.', help_winid) == 1
            scroll_cmd = 'G'
        endif
        Win_execute(help_winid, $'normal! {scroll_cmd}')
        return true

    # increase/decrease the popup's width
    elseif key == '+' || key == '-'
        var width: number = winid->popup_getoptions().minwidth
        if key == '-' && width == 1
        || key == '+' && winid->popup_getpos().col == 1
            return true
        endif
        width = width + (key == '+' ? 1 : -1)
        # remember the last width if we close and re-open the TOC later
        b:toc.width = width
        popup_setoptions(winid, {minwidth: width, maxwidth: width})
        return true

    elseif key == 'H' && b:toc.curlvl > 1
        || key == 'L' && b:toc.curlvl < b:toc.maxlvl
        CollapseOrExpand(winid, key)
        return true

    elseif key == '/'
        # This is probably what the user expects if they've started a first
        # fuzzy search, press Escape, then start a new one.
        DisplayNonFuzzyToc(winid)

        [{
            group: 'HelpToc',
            event: 'CmdlineChanged',
            pattern: '@',
            cmd: $'FuzzySearch({winid})',
            replace: true,
        }, {
            group: 'HelpToc',
            event: 'CmdlineLeave',
            pattern: '@',
            cmd: 'TearDown()',
            replace: true,
        }]->autocmd_add()

        # Need to evaluate `winid` right now{{{
        # with an `eval`'ed and `execute()`'ed heredoc because:
        #
        #  - the mappings can only access the script-local namespace
        #  - `winid` is in the function namespace; not in the script-local one
        #}}}
        var input_mappings: list<string> =<< trim eval END
          cnoremap <buffer><nowait> <Down> <ScriptCmd>Filter({winid}, 'j')<CR>
          cnoremap <buffer><nowait> <Up> <ScriptCmd>Filter({winid}, 'k')<CR>
          cnoremap <buffer><nowait> <C-N> <ScriptCmd>Filter({winid}, 'j')<CR>
          cnoremap <buffer><nowait> <C-P> <ScriptCmd>Filter({winid}, 'k')<CR>
        END
        input_mappings->execute()

        var look_for: string
        try
            popup_setoptions(winid, {mapping: true})
            look_for = input('look for: ', '', $'custom,{Complete->string()}')
                | redraw
                | echo ''
        catch /Vim:Interrupt/
            TearDown()
        finally
            popup_setoptions(winid, {mapping: false})
        endtry
        return look_for == '' ? true : popup_filter_menu(winid, "\<CR>")
    endif

    return popup_filter_menu(winid, key)
enddef

def FuzzySearch(winid: number) #{{{2
    var look_for: string = getcmdline()
    if look_for == ''
        DisplayNonFuzzyToc(winid)
        return
    endif

    # We match against *all* entries; not just the currently visible ones.
    # Rationale: If we use a (fuzzy) search, we're probably lost.  We don't
    # know where the info is.
    var matches: list<list<any>> = b:toc.entries
        ->copy()
        ->matchfuzzypos(look_for, {key: 'text'})

    fuzzy_entries = matches->get(0, [])->copy()
    var pos: list<list<number>> = matches->get(1, [])

    var text: list<dict<any>>
    if !has('textprop')
        text = matches->get(0, [])
    else
        var buf: number = winid->winbufnr()
        if prop_type_get('help-fuzzy-toc', {bufnr: buf}) == {}
            prop_type_add('help-fuzzy-toc', {
                bufnr: buf,
                combine: false,
                highlight: 'IncSearch',
            })
        endif
        text = matches
            ->get(0, [])
            ->map((i: number, match: dict<any>) => ({
                text: match.text,
                props: pos[i]->copy()->map((_, col: number) => ({
                    col: col + 1,
                    length: 1,
                    type: 'help-fuzzy-toc',
            }))}))
    endif
    Win_execute(winid, 'normal! 1Gzt')
    Popup_settext(winid, text)
enddef

def DisplayNonFuzzyToc(winid: number) #{{{2
    fuzzy_entries = null_list
    Popup_settext(winid, GetTocEntries())
enddef

def PrintEntry(winid: number) #{{{2
    echo GetTocEntries()[line('.', winid) - 1]['text']
enddef

def CollapseOrExpand(winid: number, key: string) #{{{2
    # Must be saved before we reset the popup contents, so we can
    # automatically select the least unexpected entry in the updated popup.
    var buf_lnum: number = GetBufLnum(winid)

    # find the nearest lower level for which the contents of the TOC changes
    if key == 'H'
        while b:toc.curlvl > 1
            var old: list<dict<any>> = GetTocEntries()
            --b:toc.curlvl
            var new: list<dict<any>> = GetTocEntries()
            # In `:help`, there are only entries in levels 3.
            # We don't want to collapse to level 2, nor 1.
            # It would clear the TOC which is confusing.
            if new->empty()
                ++b:toc.curlvl
                break
            endif
            var did_change: bool = new != old
            if did_change || b:toc.curlvl == 1
                break
            endif
        endwhile
    # find the nearest upper level for which the contents of the TOC changes
    else
        while b:toc.curlvl < b:toc.maxlvl
            var old: list<dict<any>> = GetTocEntries()
            ++b:toc.curlvl
            var did_change: bool = GetTocEntries() != old
            if did_change || b:toc.curlvl == b:toc.maxlvl
                break
            endif
        endwhile
    endif

    # Update the popup contents
    var toc_entries: list<dict<any>> = GetTocEntries()
    Popup_settext(winid, toc_entries)

    # Try to select the same entry;  if it's no longer visible, select its
    # direct parent.
    var toc_lnum: number = 0
    for entry: dict<any> in toc_entries
        if entry.lnum > buf_lnum
            break
        endif
        ++toc_lnum
    endfor
    Win_execute(winid, $'normal! {toc_lnum ?? 1}Gzz')
enddef

def MatchDelete() #{{{2
    if selected_entry_match == 0
        return
    endif

    selected_entry_match->matchdelete()
    selected_entry_match = 0
enddef

def Callback(winid: number, choice: number) #{{{2
    MatchDelete()

    if help_winid != 0
        help_winid->popup_close()
        help_winid = 0
    endif

    if choice == -1
        fuzzy_entries = null_list
        return
    elseif choice == -2  # Button X is clicked (when close: 'button')
        return
    endif

    var lnum: number = GetTocEntries()
        ->get(choice - 1, {})
        ->get('lnum')

    fuzzy_entries = null_list

    if lnum == 0
        return
    endif

    cursor(lnum, 1)
    normal! zvzt
enddef

def ToggleHelp(menu_winid: number) #{{{2
    # Show/hide HELP_TEXT in a second popup when '?' is typed{{{
    # (when a helptoc popup is open).  A scrollbar on this popup makes sense
    # because it is very long and, even if it's not used for scrolling, works
    # well as an indicator of how far through the HELP_TEXT popup you are. }}}
    if help_winid == 0
        var height: number = [HELP_TEXT->len(), winheight(0) * 2 / 3]->min()
        var longest_line: number = HELP_TEXT
            ->copy()
            ->map((_, line: string) => line->strcharlen())
            ->max()
        var width: number = [longest_line, winwidth(0) - 4]->min()
        var zindex: number = popup_getoptions(menu_winid).zindex
        ++zindex
        help_winid = HELP_TEXT->popup_create({
            pos: 'center',
            minheight: height,
            maxheight: height,
            minwidth: width,
            maxwidth: width,
            highlight: &buftype == 'terminal' ? 'Terminal' : 'Normal',
            zindex: zindex,
            border: g:helptoc.popup_border,
            borderchars: g:helptoc.popup_borderchars,
            borderhighlight: g:helptoc.popup_borderhighlight,
            close: g:helptoc.popup_close,
            scrollbar: true,
        })

        setwinvar(help_winid, '&cursorline', true)
        setwinvar(help_winid, '&linebreak', true)
        matchadd('Special', '^<\S\+\|^\S\{,2}  \@=', 0, -1,
            {window: help_winid})
        matchadd('Number', '\d\+', 0, -1, {window: help_winid})
        for lnum: number in HELP_TEXT->len()->range()
            if HELP_TEXT[lnum] =~ '^─\+$'
                matchaddpos('Title', [lnum], 0, -1, {window: help_winid})
            endif
        endfor

    else
        if IsVisible(help_winid)
            popup_hide(help_winid)
        else
            popup_show(help_winid)
        endif
    endif
enddef

def Win_execute(winid: number, cmd: any) #{{{2
# wrapper around `win_execute()` to enforce a redraw, which might be necessary
# whenever we change the cursor position
    win_execute(winid, cmd)
    redraw
enddef

def TearDown() #{{{2
    autocmd_delete([{group: 'HelpToc'}])
    cunmap <buffer> <Down>
    cunmap <buffer> <Up>
    cunmap <buffer> <C-N>
    cunmap <buffer> <C-P>
enddef
# Util {{{1
def GetType(): string #{{{2
    return &buftype == 'terminal' ? 'terminal' : &filetype
enddef

def GetTocEntries(): list<dict<any>> #{{{2
    return fuzzy_entries ?? b:toc.entries
        ->copy()
        ->filter((_, entry: dict<any>): bool => entry.lvl <= b:toc.curlvl)
enddef

def GetBufLnum(winid: number): number #{{{2
    var toc_lnum: number = line('.', winid)
    return GetTocEntries()
        ->get(toc_lnum - 1, {})
        ->get('lnum')
enddef

def IsVisible(win: number): bool #{{{2
    return win->popup_getpos()->get('visible')
enddef

def IsSpecialHelpLine(line: string): bool #{{{2
    return line =~ '^[<>]\=\s*$'
        || line =~ '^\s*\*'
        || line =~ HELP_RULER
        || line =~ HELP_HEADLINE
enddef

def Complete(..._): string #{{{2
    return b:toc.entries
        ->copy()
        ->map((_, entry: dict<any>) =>
            entry.text->trim(' ~')->substitute('*', '', 'g'))
        ->filter((_, text: string): bool => text =~ '^[-a-zA-Z0-9_() ]\+$')
        ->sort()
        ->uniq()
        ->join("\n")
enddef  #}}}2
#}}}1
# vim:et:ft=vim:fdm=marker:
