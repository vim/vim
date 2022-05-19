vim9script noclear

# Config {{{1

const SHELL_PROMPT: string = get(g:, 'helptoc', {})
    ->get('shell_prompt', '^\w\+@\w\+:\f\+\$\s')

const HELP: list<string> =<< trim END
    normal commands in help window
    ──────────────────────────────
    ?      show/hide this help
    <C-J>  scroll down one line
    <C-K>  scroll up one line

    normal commands in TOC menu
    ─────────────────────────────
    j      select next entry
    k      select previous entry
    J      same as j, and jump to corresponding line in main buffer
    K      same as k, and jump to corresponding line in main buffer
    g      select first entry
    G      select last entry
    H      collapse one level
    L      expand one level
    p      print current entry on command-line

    P      same as p but automatically, whenever selection changes;
           press multiple times to toggle feature on/off

    q      quit menu
    z      redraw menu with current entry at center
    <C-D>  scroll down half a page
    <C-U>  scroll up half a page
    +      increases width of popup menu
    -      decreases width of popup menu

    title meaning
    ─────────────
    example: 12/34 (5)
    broken down:

        12  number of current entry
        34  number of last entry
         5  number of deepest level currently visible
END

# Init {{{1

const DB: dict<dict<func: bool>> = {
    help: {},

    man: {
        1: (line: string, _): bool => line =~ '^\S',
        2: (line: string, _): bool => line =~ '^\%( \{3\}\)\=\S',
    },

    markdown: {
        1: (line: string, nextline: string): bool =>
            line =~ '^#[^#]' || nextline =~ '^=\+$' && line =~ '\w',
        2: (line: string, nextline: string): bool =>
            line =~ '^##[^#]' || nextline =~ '^-\+$' && line =~ '\w',
        3: (line: string, _): bool => line =~ '^###[^#]',
        4: (line: string, _): bool => line =~ '^####[^#]',
        5: (line: string, _): bool => line =~ '^#####[^#]',
        6: (line: string, _): bool => line =~ '^######[^#]',
    },

    # We support a terminal buffer.{{{
    #
    # The TOC  should display each  shell command executed  in the buffer  as an
    # entry.  This is useful for a buffer created with `:terminal`, but also for
    # a regular Vim buffer capturing a tmux pane.
    #}}}
    terminal: {
        1: (line: string, _): bool => line =~ SHELL_PROMPT,
    }
}

const help_rulers: dict<string> = {
    '=': '^=\{40,}$',
    '-': '^-\{40,}',
}
const help_ruler: string = help_rulers->values()->join('\|')

# the regex is copied from the help syntax plugin
const help_tag: string = '\*[#-)!+-~]\+\*\%(\s\|$\)\@='

var fuzzy_toc: list<dict<any>>
var help_winid: number
var print_entry: bool
var selected_entry_match: number

# Adapted from `$VIMRUNTIME/syntax/help.vim`.{{{
#
# The original regex is:
#
#     ^[-A-Z .][-A-Z0-9 .()_]*\ze\(\s\+\*\|$\)
#
# Allowing a  space or a hyphen  at the start  can give false positives,  and is
# useless, so we don't allow them.
#}}}
const helpHeadline: string = '^\C[A-Z.][-A-Z0-9 .()_]*\%(\s\+\*+\@!\|$\)'
#                                                              ^--^
# To prevent some false positives under `:help feature-list`.

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

# Interface {{{1
export def Open() #{{{2
    var type: string = GetType()
    if DB->keys()->index(type) == -1
        return
    endif

    # invalidate the cache if the buffer's contents has changed
    if exists('b:toc') && b:toc.changedtick != b:changedtick
        unlet! b:toc
    endif
    if !exists('b:toc')
        SetToc()
        # in a terminal buffer, `b:changedtick` does not change
        if type == 'terminal'
            autocmd ModeChanged t:nt ++once unlet! b:toc
        endif
    endif

    var winpos: list<number> = winnr()->win_screenpos()
    var height: number = winheight(0) - 2
    var width: number = winwidth(0)
    b:toc.width = b:toc.width ?? width / 3
    # The popup needs enough space to display the help message in its title.
    # Make some test in `:Man ffmpeg-all`.
    if b:toc.width < 30
        b:toc.width = 30
    endif
    # Is `popup_menu()` ok with a list of dictionaries?{{{
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
            scrollbar: false,
            highlight: 'Normal',
            border: [],
            borderchars: ['─', '│', '─', '│', '┌', '┐', '┘', '└'],
            minheight: height,
            maxheight: height,
            minwidth: b:toc.width,
            maxwidth: b:toc.width,
            filter: Filter,
            callback: Callback,
        })
    win_execute(winid, [$'ownsyntax {&filetype}', '&l:conceallevel = 3'])
    # In a help file, we might reduce some noisy tags to a trailing asterisk.
    # Hide those.
    if type == 'help'
        matchadd('Conceal', '\*$', 0, -1, {window: winid})
    endif
    SelectMostRelevantEntry(winid)

    # can't set  the title before  jumping to  the relevant line,  otherwise the
    # indicator in the title might be wrong
    SetTitle(winid)
enddef
#}}}1
# Core {{{1
def SetToc() #{{{2
    var toc: dict<any> = {entries: []}
    var type: string = GetType()
    toc.maxlvl = DB[type]->keys()->len()
    toc.curlvl = toc.maxlvl
    toc.changedtick = b:changedtick
    if !toc->has_key('width')
        toc.width = 0
    endif
    # We cache the toc in `b:toc` to get better performance.{{{
    #
    # Without caching,  when you  press `H`, `L`,  `H`, `L`, ...  quickly for  a few
    # seconds, there is some lag if you then try to move with `j` and `k`.
    # This can only be perceived in big man pages like ffmpeg-all.
    #}}}
    b:toc = toc

    if type == 'help'
        SetTocHelp()
        return
    endif

    var curline: string = getline(1)
    var nextline: string
    var lvl_and_test: list<list<any>> = DB
        ->get(type, {})
        ->items()
        ->sort((l: list<any>, ll: list<any>): number => l[0]->str2nr() - ll[0]->str2nr())

    for lnum: number in range(1, line('$'))
        nextline = getline(lnum + 1)
        for [lvl: string, IsInToc: func: bool] in lvl_and_test
            if IsInToc(curline, nextline)
                b:toc.entries += [{
                    lnum: lnum,
                    lvl: lvl->str2nr(),
                    text: curline,
                }]
                break
            endif
        endfor
        curline = nextline
    endfor
enddef

def SetTocHelp() #{{{2
    var main_ruler: string
    for line: string in getline(1, '$')
        if line =~ help_ruler
            main_ruler = line =~ '=' ? help_rulers['='] : help_rulers['-']
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
            # The information gathered in `lvls`  might not be applicable to all
            # the main sections of a help file.  Let's reset it whenever we find
            # a ruler.
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
        && curline !~ $'^\s*{help_tag}'
            in_list = true
        endif

        # 1.
        if prevline =~ '^\d\+\.\s'
        # let's assume that the  start of a main entry is  always followed by an
        # empty line, or a line starting with a tag
        && (curline =~ '^>\=\s*$' || curline =~ $'^\s*{help_tag}')
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
            if curline =~ $'\%({help_tag}\s*\|\~\)$'
            || (prevline =~ $'^\s*{help_tag}' || nextline =~ $'^\s*{help_tag}')
            || (prevline =~ help_ruler || nextline =~ help_ruler)
            || (prevline =~ '^\s*$' && nextline =~ '^\s*$')
                AddEntryInTocHelp('1.2', lnum, curline)
            endif
        # 1.2.3
        elseif curline =~ '^\s\=\d\+\.\d\+\.\d\+\s'
            AddEntryInTocHelp('1.2.3', lnum, curline)
        endif

        # HEADLINE
        if curline =~ helpHeadline
        && curline !~ '^CTRL-'
        &&  prevline->IsSpecialHelpLine()
        && (nextline->IsSpecialHelpLine() || nextline =~ '^\s*(\|^\t\|^N[oO][tT][eE]:')
            AddEntryInTocHelp('HEADLINE', lnum, curline)
        endif

        # header ~
        if curline =~ '\~$'
        && curline =~ '\w'
        && curline !~ '^[ \t<]\|\t\|---+---\|^NOTE:'
        && curline !~ '^\d\+\.\%(\d\+\%(\.\d\+\)\=\)\=\s'
        && prevline !~ $'^\s*{help_tag}'
        && prevline !~ '\~$'
        && nextline !~ '\~$'
            AddEntryInTocHelp('header ~', lnum, curline)
        endif

        # *some_tag*
        if curline =~ help_tag
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
        #     ^----^
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

    b:toc.maxlvl = b:toc.entries
        ->copy()
        ->map((_, entry: dict<any>) => entry.lvl)
        ->max()
    b:toc.curlvl = b:toc.maxlvl

    # set level of tag entries to the deepest level
    var has_tag: bool = b:toc.entries
        ->copy()
        ->map((_, entry: dict<any>) => entry.text)
        ->match(help_tag) >= 0
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
            ->substitute('^\s*', () => repeat(' ', (entry.lvl - min_lvl) * 3), '')
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
        text->substitute(help_tag, () => !!tags->add(submatch(0)), 'g')
        text = tags
            # we ignore errors and warnings because those are meaningless in
            # a TOC where no context is available
            ->filter((_, tag: string) => tag !~ '\*[EW]\d\+\*')
            ->join()
        if text !~ $'{help_tag}'
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
    #     14. Linking groups              *:hi-link* *:highlight-link* *E412* *E413*
    #                                     ^----------------------------------------^
    #                                     ^\s*\d\+\.\%(\d\+\.\=\)*\s\+.\{-}\zs\*.*
    # ---
    #
    # We don't use conceal because then, `matchfuzzypos()` could match concealed
    # characters, which would be confusing.
    #}}}
    #     MAKING YOUR OWN SYNTAX FILES                            *mysyntaxfile*
    #                                                             ^------------^
    #                                                             ^\s*[A-Z].\{-}\*\zs.*
    #
    var after_HEADLINE: string = '^\s*[A-Z].\{-}\*\zs.*'
    #     14. Linking groups              *:hi-link* *:highlight-link* *E412* *E413*
    #                                     ^----------------------------------------^
    #                                     ^\s*\d\+\.\%(\d\+\.\=\)*\s\+.\{-}\*\zs.*
    var after_numbered: string = '^\s*\d\+\.\%(\d\+\.\=\)*\s\+.\{-}\*\zs.*'
    #     01.3    Using the Vim tutor                             *tutor* *vimtutor*
    #                                                             ^----------------^
    var after_numbered_tutor: string = '^\*\d\+\.\%(\d\+\.\=\)*.\{-}\t\*\zs.*'
    var noisy_tags: string = $'{after_HEADLINE}\|{after_numbered}\|{after_numbered_tutor}'
    text = text->substitute(noisy_tags, '', '')
    # We  don't remove  the trailing  asterisk, because  the help  syntax plugin
    # might need it to highlight some headlines.

    b:toc.entries += [{
        lnum: lnum,
        lvl: lvls[type],
        text: text,
        type: type,
    }]
enddef

def Popup_settext(winid: number, entries: list<dict<any>>) #{{{2
    var text: list<any>
    # When we  fuzzy search  the toc,  the dictionaries  in `entries`  contain a
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
enddef

def SetTitle(winid: number) #{{{2
    var lastlnum: number = line('$', winid)
    var newtitle: string = printf(' %*d/%d (%d)',
        len(lastlnum), line('.', winid),
        lastlnum,
        b:toc.curlvl)

    var width: number = winid->popup_getoptions().minwidth
    newtitle = printf('%s%*s',
        newtitle,
        width - newtitle->strlen(),
        'press ? for help ')

    popup_setoptions(winid, {title: newtitle})
    redraw
enddef

def SelectMostRelevantEntry(winid: number) #{{{2
    var lnum: number = line('.')
    var firstline: number = b:toc.entries
        ->copy()
        ->filter((_, line: dict<any>): bool => line.lvl <= b:toc.curlvl && line.lnum <= lnum)
        ->len()
    if firstline == 0
        return
    endif
    win_execute(winid, $'normal! {firstline}Gzz')
enddef

def Filter(winid: number, key: string): bool #{{{2
    def PrintEntry()
        echo GetTocEntries()[line('.', winid) - 1]['text']
    enddef

    # support various normal commands for moving/scrolling
    if [
        'j', 'J', 'k', 'K', "\<Down>", "\<Up>", "\<C-N>", "\<C-P>",
        'g', 'G', 'z', "\<C-D>", "\<C-U>",
       ]->index(key) >= 0
        if print_entry
            PrintEntry()
        endif
        var scroll_cmd: string = {
            J: 'j',
            K: 'k',
            g: '1G',
            z: 'zz'
        }->get(key, key)

        var old_lnum: number = line('.', winid)
        win_execute(winid, $'normal! {scroll_cmd}')
        var new_lnum: number = line('.', winid)

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
                win_execute(winid, $'normal! {scroll_cmd}')
            endif
        endif

        # move the cursor to the corresponding line in the main buffer
        if key == 'J' || key == 'K'
            var lnum: number = GetBufLnum(winid)
            execute $'normal! 0{lnum}zt'
            # install a match in the regular buffer to highlight the position of
            # the entry in the latter
            MatchDelete()
            selected_entry_match = matchaddpos('IncSearch', [lnum], 0, -1)
        endif
        SetTitle(winid)
        return true

    # when we press `p`, print the selected line (useful when it's truncated)
    elseif key == 'p'
        PrintEntry()
        return true

    # same thing, but automatically
    elseif key == 'P'
        print_entry = !print_entry
        if print_entry
            PrintEntry()
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
        win_execute(help_winid, $'normal! {scroll_cmd}')
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
        var augroup: string = 'HelpToc'
        var autocmd: list<string> =<< trim eval END
            augroup {augroup}
                autocmd!
                autocmd CmdlineLeave @ TearDown()
                autocmd CmdlineChanged @ FuzzyToc({winid})
            augroup END

            cnoremap <buffer><nowait> <Down> <ScriptCmd>Filter({winid}, 'j')<Bar>redraw<CR>
            cnoremap <buffer><nowait> <Up> <ScriptCmd>Filter({winid}, 'k')<Bar>redraw<CR>
            cnoremap <buffer><nowait> <C-N> <ScriptCmd>Filter({winid}, 'j')<Bar>redraw<CR>
            cnoremap <buffer><nowait> <C-P> <ScriptCmd>Filter({winid}, 'k')<Bar>redraw<CR>

            if !exists('*TearDown')
                def TearDown()
                    autocmd! {augroup}
                    augroup! {augroup}
                    cunmap <buffer> <Down>
                    cunmap <buffer> <Up>
                    cunmap <buffer> <C-N>
                    cunmap <buffer> <C-P>
                enddef
            endif
        END
        autocmd->execute()
        popup_setoptions(winid, {mapping: true})
        var look_for: string = input('look for: ', '', $'custom,{Complete->string()}') | redraw | echo ''
        popup_setoptions(winid, {mapping: false})
        if look_for == ''
            # restore the TOC as it was originally
            Popup_settext(winid, GetTocEntries())
            return true
        else
            return popup_filter_menu(winid, "\<CR>")
        endif
    endif

    return popup_filter_menu(winid, key)
enddef

def FuzzyToc(winid: number) #{{{2
    var look_for: string = getcmdline()
    if look_for == ''
        Popup_settext(winid, b:toc.entries)
        return
    endif

    # We  match against  *all* entries;  not  just the  currently visible  ones.
    # Rationale: If we use a (fuzzy) search, we're probably lost.  We don't know
    # where the info is.
    var matches: list<list<any>> = b:toc.entries
        ->copy()
        ->matchfuzzypos(look_for, {key: 'text'})

    fuzzy_toc = matches->get(0, [])->copy()
    var pos: list<list<number>> = matches->get(1, [])

    var text: list<dict<any>>
    if !has('textprop')
        text = matches->get(0, [])
    else
        var buf: number = winid->winbufnr()
        if prop_type_list({bufnr: buf})->index('help-fuzzy-toc') == -1
            prop_type_add('help-fuzzy-toc', {
                bufnr: buf,
                combine: false,
                highlight: 'WarningMsg',
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
    Popup_settext(winid, text)
    win_execute(winid, 'normal! 1Gzt')
enddef

def CollapseOrExpand(winid: number, key: string) #{{{2
    # Must  be  saved  before  we  reset  the  popup  contents,  so  we  can
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
            if b:toc.curlvl == 1 || did_change
                break
            endif
        endwhile
    # find the nearest upper level for which the contents of the TOC changes
    else
        while b:toc.curlvl < b:toc.maxlvl
            var old: list<dict<any>> = GetTocEntries()
            ++b:toc.curlvl
            var did_change: bool = GetTocEntries() != old
            if b:toc.curlvl == b:toc.maxlvl || did_change
                break
            endif
        endwhile
    endif

    # update the popup contents
    var toc_entries: list<dict<any>> = GetTocEntries()
    Popup_settext(winid, toc_entries)

    # Try to  select the same entry;  if it's no longer  visible, select its
    # direct parent.
    var toc_lnum: number = 0
    for entry: dict<any> in toc_entries
        if entry.lnum > buf_lnum
            break
        endif
        ++toc_lnum
    endfor
    win_execute(winid, $'normal! {toc_lnum ?? 1}Gzz')
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
        return
    endif

    var lnum: number = (fuzzy_toc ?? GetTocEntries())
        ->get(choice - 1, {})
        ->get('lnum')

    fuzzy_toc = null_list

    if lnum == 0
        return
    endif

    cursor(lnum, 1)
    normal! zvzt
enddef

def ToggleHelp(menu_winid: number) #{{{2
    if help_winid == 0
        var height: number = [HELP->len(), winheight(0) * 2 / 3]->min()
        var longest_line: number = HELP
            ->copy()
            ->map((_, line: string) => line->strcharlen())
            ->max()
        var width: number = [longest_line, winwidth(0) * 2 / 3]->min()
        var col: number = popup_getpos(menu_winid).col
        --col
        var zindex: number = popup_getoptions(menu_winid).zindex
        ++zindex
        help_winid = HELP->popup_create({
            line: 2,
            col: col,
            pos: 'topright',
            minheight: height,
            maxheight: height,
            minwidth: width,
            maxwidth: width,
            border: [],
            borderchars: ['─', '│', '─', '│', '┌', '┐', '┘', '└'],
            highlight: 'Normal',
            scrollbar: false,
            zindex: zindex,
        })

        setwinvar(help_winid, '&cursorline', true)
        matchadd('Special', '^<\S\+\|^\S\{,2}  \@=', 0, -1, {window: help_winid})
        matchadd('Title', '^[a-z]\{2,}\%(\s*\w*\)*$', 0, -1, {window: help_winid})
        matchadd('Number', '\d\+', 0, -1, {window: help_winid})

    else
        if IsVisible(help_winid)
            popup_hide(help_winid)
        else
            popup_show(help_winid)
        endif
    endif
enddef
#}}}1
# Util {{{1
def GetType(): string #{{{2
    return &buftype == 'terminal' ?  'terminal' : &filetype
enddef

def GetTocEntries(): list<dict<any>> #{{{2
    return b:toc.entries
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
        || line =~ help_ruler
        || line =~ helpHeadline
enddef

def Complete(..._): string #{{{2
    return b:toc.entries
        ->copy()
        ->map((_, entry: dict<any>) => entry.text)
        ->join(' ')
        ->split('\s\+')
        ->filter((_, token: string): bool => token =~ '^\w\+$')
        ->sort()
        ->uniq()
        ->join("\n")
enddef

