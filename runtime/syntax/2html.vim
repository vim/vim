vim9script
# Vim syntax support file
# Maintainer: Ben Fritz <fritzophrenic@gmail.com>
# Last Change: 2026 Apr 4
#
# Additional contributors:
#
#             Original by Bram Moolenaar <Bram@vim.org>
#             Modified by David Ne\v{c}as (Yeti) <yeti@physics.muni.cz>
#             XHTML support by Panagiotis Issaris <takis@lumumba.luc.ac.be>
#             Made w3 compliant by Edd Barrett <vext01@gmail.com>
#             Added html_font. Edd Barrett <vext01@gmail.com>
#             Progress bar based off code from "progressbar widget" plugin by
#               Andreas Politz, heavily modified:
#               http://www.vim.org/scripts/script.php?script_id=2006
#
#             See Mercurial change logs for more!

# Transform a file into HTML, using the current syntax highlighting.

# this file uses line continuations (but in vim9script we avoid them)
var ls_sav = &laststatus
var ei_sav = &eventignore
set eventignore+=FileType

var line_end = line('$')

var trim_tmp: list<string>
# Font
var htmlfont: string
if exists("g:html_font")
  if type(g:html_font) == type([])
    htmlfont = "'" .. join(g:html_font, "','") .. "', monospace"
  else
    htmlfont = "'" .. g:html_font .. "', monospace"
  endif
else
  htmlfont = "monospace"
endif

var settings = tohtml#GetUserSettings()

var html5: number
if settings.use_xhtml
  html5 = 0
elseif settings.use_css && !settings.no_pre
  html5 = 1
else
  html5 = 0
endif

const FOLDED_ID  = hlID("Folded")
const FOLD_C_ID  = hlID("FoldColumn")
const LINENR_ID  = hlID('LineNr')
const DIFF_D_ID  = hlID("DiffDelete")
const DIFF_A_ID  = hlID("DiffAdd")
const DIFF_C_ID  = hlID("DiffChange")
const DIFF_T_ID  = hlID("DiffText")
const CONCEAL_ID = hlID('Conceal')

# Whitespace
var whitespace: string
if settings.pre_wrap
  whitespace = "white-space: pre-wrap; "
else
  whitespace = ""
endif

var unselInputType: string
if !empty(settings.prevent_copy)
  if settings.no_invalid
    # User has decided they don't want invalid markup. Still works in
    # OpenOffice, and for text editors, but when pasting into Microsoft Word the
    # input elements get pasted too and they cannot be deleted (at least not
    # easily).
    unselInputType = ""
  else
    # Prevent from copy-pasting the input elements into Microsoft Word where
    # they cannot be deleted easily by deliberately inserting invalid markup.
    unselInputType = " type='invalid_input_type'"
  endif
endif

# When gui colors are not supported, we can only guess the colors.
# TODO - is this true anymore? Is there a way to ask the terminal what colors
# each number means or read them from some file?
var whatterm: string
var cterm_color: dict<string>
if &termguicolors || has("gui_running")
  whatterm = "gui"
else
  whatterm = "cterm"
  if &t_Co == '8'
    cterm_color = {
      0: "#808080", 1: "#ff6060", 2: "#00ff00", 3: "#ffff00",
      4: "#8080ff", 5: "#ff40ff", 6: "#00ffff", 7: "#ffffff"
    }
  else
    cterm_color = {
      0: "#000000", 1: "#c00000", 2: "#008000", 3: "#804000",
      4: "#0000c0", 5: "#c000c0", 6: "#008080", 7: "#c0c0c0",
      8: "#808080", 9: "#ff6060", 10: "#00ff00", 11: "#ffff00",
      12: "#8080ff", 13: "#ff40ff", 14: "#00ffff", 15: "#ffffff"
    }

    # Colors for 88 and 256 come from xterm.
    if &t_Co == '88'
      extend(cterm_color, {
	16: "#000000", 17: "#00008b", 18: "#0000cd", 19: "#0000ff",
	20: "#008b00", 21: "#008b8b", 22: "#008bcd", 23: "#008bff",
	24: "#00cd00", 25: "#00cd8b", 26: "#00cdcd", 27: "#00cdff",
	28: "#00ff00", 29: "#00ff8b", 30: "#00ffcd", 31: "#00ffff",
	32: "#8b0000", 33: "#8b008b", 34: "#8b00cd", 35: "#8b00ff",
	36: "#8b8b00", 37: "#8b8b8b", 38: "#8b8bcd", 39: "#8b8bff",
	40: "#8bcd00", 41: "#8bcd8b", 42: "#8bcdcd", 43: "#8bcdff",
	44: "#8bff00", 45: "#8bff8b", 46: "#8bffcd", 47: "#8bffff",
	48: "#cd0000", 49: "#cd008b", 50: "#cd00cd", 51: "#cd00ff",
	52: "#cd8b00", 53: "#cd8b8b", 54: "#cd8bcd", 55: "#cd8bff",
	56: "#cdcd00", 57: "#cdcd8b", 58: "#cdcdcd", 59: "#cdcdff",
	60: "#cdff00", 61: "#cdff8b", 62: "#cdffcd", 63: "#cdffff",
	64: "#ff0000"
      })
      extend(cterm_color, {
	65: "#ff008b", 66: "#ff00cd", 67: "#ff00ff", 68: "#ff8b00",
	69: "#ff8b8b", 70: "#ff8bcd", 71: "#ff8bff", 72: "#ffcd00",
	73: "#ffcd8b", 74: "#ffcdcd", 75: "#ffcdff", 76: "#ffff00",
	77: "#ffff8b", 78: "#ffffcd", 79: "#ffffff", 80: "#2e2e2e",
	81: "#5c5c5c", 82: "#737373", 83: "#8b8b8b", 84: "#a2a2a2",
	85: "#b9b9b9", 86: "#d0d0d0", 87: "#e7e7e7"
      })
    elseif &t_Co == '256'
      extend(cterm_color, {
	16: "#000000", 17: "#00005f", 18: "#000087", 19: "#0000af",
	20: "#0000d7", 21: "#0000ff", 22: "#005f00", 23: "#005f5f",
	24: "#005f87", 25: "#005faf", 26: "#005fd7", 27: "#005fff",
	28: "#008700", 29: "#00875f", 30: "#008787", 31: "#0087af",
	32: "#0087d7", 33: "#0087ff", 34: "#00af00", 35: "#00af5f",
	36: "#00af87", 37: "#00afaf", 38: "#00afd7", 39: "#00afff",
	40: "#00d700", 41: "#00d75f", 42: "#00d787", 43: "#00d7af",
	44: "#00d7d7", 45: "#00d7ff", 46: "#00ff00", 47: "#00ff5f",
	48: "#00ff87", 49: "#00ffaf", 50: "#00ffd7", 51: "#00ffff",
	52: "#5f0000", 53: "#5f005f", 54: "#5f0087", 55: "#5f00af",
	56: "#5f00d7", 57: "#5f00ff", 58: "#5f5f00", 59: "#5f5f5f",
	60: "#5f5f87", 61: "#5f5faf", 62: "#5f5fd7", 63: "#5f5fff",
	64: "#5f8700"
      })
      extend(cterm_color, {
	65: "#5f875f", 66: "#5f8787", 67: "#5f87af", 68: "#5f87d7",
	69: "#5f87ff", 70: "#5faf00", 71: "#5faf5f", 72: "#5faf87",
	73: "#5fafaf", 74: "#5fafd7", 75: "#5fafff", 76: "#5fd700",
	77: "#5fd75f", 78: "#5fd787", 79: "#5fd7af", 80: "#5fd7d7",
	81: "#5fd7ff", 82: "#5fff00", 83: "#5fff5f", 84: "#5fff87",
	85: "#5fffaf", 86: "#5fffd7", 87: "#5fffff", 88: "#870000",
	89: "#87005f", 90: "#870087", 91: "#8700af", 92: "#8700d7",
	93: "#8700ff", 94: "#875f00", 95: "#875f5f", 96: "#875f87",
	97: "#875faf", 98: "#875fd7", 99: "#875fff", 100: "#878700",
	101: "#87875f", 102: "#878787", 103: "#8787af", 104: "#8787d7",
	105: "#8787ff", 106: "#87af00", 107: "#87af5f", 108: "#87af87",
	109: "#87afaf", 110: "#87afd7", 111: "#87afff", 112: "#87d700"
      })
      extend(cterm_color, {
	113: "#87d75f", 114: "#87d787", 115: "#87d7af", 116: "#87d7d7",
	117: "#87d7ff", 118: "#87ff00", 119: "#87ff5f", 120: "#87ff87",
	121: "#87ffaf", 122: "#87ffd7", 123: "#87ffff", 124: "#af0000",
	125: "#af005f", 126: "#af0087", 127: "#af00af", 128: "#af00d7",
	129: "#af00ff", 130: "#af5f00", 131: "#af5f5f", 132: "#af5f87",
	133: "#af5faf", 134: "#af5fd7", 135: "#af5fff", 136: "#af8700",
	137: "#af875f", 138: "#af8787", 139: "#af87af", 140: "#af87d7",
	141: "#af87ff", 142: "#afaf00", 143: "#afaf5f", 144: "#afaf87",
	145: "#afafaf", 146: "#afafd7", 147: "#afafff", 148: "#afd700",
	149: "#afd75f", 150: "#afd787", 151: "#afd7af", 152: "#afd7d7",
	153: "#afd7ff", 154: "#afff00", 155: "#afff5f", 156: "#afff87",
	157: "#afffaf", 158: "#afffd7"
      })
      extend(cterm_color, {
	159: "#afffff", 160: "#d70000", 161: "#d7005f", 162: "#d70087",
	163: "#d700af", 164: "#d700d7", 165: "#d700ff", 166: "#d75f00",
	167: "#d75f5f", 168: "#d75f87", 169: "#d75faf", 170: "#d75fd7",
	171: "#d75fff", 172: "#d78700", 173: "#d7875f", 174: "#d78787",
	175: "#d787af", 176: "#d787d7", 177: "#d787ff", 178: "#d7af00",
	179: "#d7af5f", 180: "#d7af87", 181: "#d7afaf", 182: "#d7afd7",
	183: "#d7afff", 184: "#d7d700", 185: "#d7d75f", 186: "#d7d787",
	187: "#d7d7af", 188: "#d7d7d7", 189: "#d7d7ff", 190: "#d7ff00",
	191: "#d7ff5f", 192: "#d7ff87", 193: "#d7ffaf", 194: "#d7ffd7",
	195: "#d7ffff", 196: "#ff0000", 197: "#ff005f", 198: "#ff0087",
	199: "#ff00af", 200: "#ff00d7", 201: "#ff00ff", 202: "#ff5f00",
	203: "#ff5f5f", 204: "#ff5f87"
      })
      extend(cterm_color, {
	205: "#ff5faf", 206: "#ff5fd7", 207: "#ff5fff", 208: "#ff8700",
	209: "#ff875f", 210: "#ff8787", 211: "#ff87af", 212: "#ff87d7",
	213: "#ff87ff", 214: "#ffaf00", 215: "#ffaf5f", 216: "#ffaf87",
	217: "#ffafaf", 218: "#ffafd7", 219: "#ffafff", 220: "#ffd700",
	221: "#ffd75f", 222: "#ffd787", 223: "#ffd7af", 224: "#ffd7d7",
	225: "#ffd7ff", 226: "#ffff00", 227: "#ffff5f", 228: "#ffff87",
	229: "#ffffaf", 230: "#ffffd7", 231: "#ffffff", 232: "#080808",
	233: "#121212", 234: "#1c1c1c", 235: "#262626", 236: "#303030",
	237: "#3a3a3a", 238: "#444444", 239: "#4e4e4e", 240: "#585858",
	241: "#626262", 242: "#6c6c6c", 243: "#767676", 244: "#808080",
	245: "#8a8a8a", 246: "#949494", 247: "#9e9e9e", 248: "#a8a8a8",
	249: "#b2b2b2", 250: "#bcbcbc", 251: "#c6c6c6", 252: "#d0d0d0",
	253: "#dadada", 254: "#e4e4e4", 255: "#eeeeee"
      })
    endif
  endif
endif

# Return good color specification: in GUI no transformation is done, in
# terminal return RGB values of known colors and empty string for unknown
if whatterm == "gui"
  def HtmlColor(color: string): string
    return color
  enddef
else
  def HtmlColor(color: string): string
    if has_key(cterm_color, color)
      return cterm_color[color]
    else
      return ""
    endif
  enddef
endif

# Find out the background and foreground color for use later
var fgc = HtmlColor(synIDattr(synIDtrans(hlID("Normal")), "fg#", whatterm))
var bgc = HtmlColor(synIDattr(synIDtrans(hlID("Normal")), "bg#", whatterm))
if fgc == ""
  fgc = (&background == "dark" ? "#ffffff" : "#000000")
endif
if bgc == ""
  bgc = (&background == "dark" ? "#000000" : "#ffffff")
endif

if !settings.use_css
  # Return opening HTML tag for given highlight id
  def HtmlOpening(id: number, extra_attrs: string): string
    var a = ""
    var translated_ID = synIDtrans(id)
    if synIDattr(translated_ID, "inverse") !=# ''
      # For inverse, we always must set both colors (and exchange them)
      var x = HtmlColor(synIDattr(translated_ID, "fg#", whatterm))
      a = a .. '<span ' .. extra_attrs .. 'style="background-color: ' .. (x != "" ? x : fgc) .. '">'
      x = HtmlColor(synIDattr(translated_ID, "bg#", whatterm))
      a = a .. '<font color="' .. (x != "" ? x : bgc) .. '">'
    else
      var x = HtmlColor(synIDattr(translated_ID, "bg#", whatterm))
      if x != ""
	a = a .. '<span ' .. extra_attrs .. 'style="background-color: ' .. x .. '">'
      elseif !empty(extra_attrs)
	a = a .. '<span ' .. extra_attrs .. '>'
      endif
      x = HtmlColor(synIDattr(translated_ID, "fg#", whatterm))
      if x != "" | a = a .. '<font color="' .. x .. '">' | endif
    endif
    if synIDattr(translated_ID, "bold") !=# '' | a = a .. "<b>" | endif
    if synIDattr(translated_ID, "italic") !=# '' | a = a .. "<i>" | endif
    if synIDattr(translated_ID, "underline") !=# '' | a = a .. "<u>" | endif
    return a
  enddef

  # Return closing HTML tag for given highlight id
  def HtmlClosing(id: number, has_extra_attrs: bool): string
    var a = ""
    var translated_ID = synIDtrans(id)
    if synIDattr(translated_ID, "underline") !=# '' | a = a .. "</u>" | endif
    if synIDattr(translated_ID, "italic") !=# '' | a = a .. "</i>" | endif
    if synIDattr(translated_ID, "bold") !=# '' | a = a .. "</b>" | endif
    if synIDattr(translated_ID, "inverse") !=# ''
      a = a .. '</font></span>'
    else
      var x = HtmlColor(synIDattr(translated_ID, "fg#", whatterm))
      if x != "" | a = a .. '</font>' | endif
      x = HtmlColor(synIDattr(translated_ID, "bg#", whatterm))
      if x != "" || has_extra_attrs | a = a .. '</span>' | endif
    endif
    return a
  enddef
endif

# Use a different function for formatting based on user options. This way we
# can avoid a lot of logic during the actual execution.
#
# Build the function line by line containing only what is needed for the options
# in use for maximum code sharing with minimal branch logic for greater speed.
#
# Note, 'exec' commands do not recognize line continuations, so must concatenate
# lines rather than continue them.
var wrapperfunc_lines: list<string>
if settings.use_css
  # save CSS to a list of rules to add to the output at the end of processing

  # first, get the style names we need
  wrapperfunc_lines =<< trim eval ENDLET
    def BuildStyleWrapper(style_id: number, diff_style_id: number, extra_attrs: string, text: string, make_unselectable: bool, unformatted: string): string
      var style_name = synIDattr(style_id, "name", whatterm)
  ENDLET
  if &diff
    trim_tmp =<< trim eval ENDLET
      var diff_style_name = synIDattr(diff_style_id, "name", whatterm)
    ENDLET
    wrapperfunc_lines += trim_tmp

    # Add normal groups and diff groups to separate lists so we can order them to
    # allow diff highlight to override normal highlight

    # if primary style IS a diff style, grab it from the diff cache instead
    # (always succeeds because we pre-populate it)
    trim_tmp =<< trim eval ENDLET
      if style_id == DIFF_D_ID || style_id == DIFF_A_ID || style_id == DIFF_C_ID || style_id == DIFF_T_ID
	var saved_style = get(diffstylelist, style_id)
      else
    ENDLET
    wrapperfunc_lines += trim_tmp
  endif

  # get primary style info from cache or build it on the fly if not found
  trim_tmp =<< trim ENDLET
	var saved_style = get(stylelist, style_id)
	if type(saved_style) == type(0)
	  saved_style = CSS1(style_id)
	  if saved_style != ""
	    saved_style = "." .. style_name .. " { " .. saved_style .. "}"
	  endif
	  stylelist[style_id] = saved_style
	endif
  ENDLET
  wrapperfunc_lines += trim_tmp
  if &diff
    trim_tmp =<< trim eval ENDLET
      endif
    ENDLET
    wrapperfunc_lines += trim_tmp
  endif
  # Ignore this comment, just bypassing a highlighting issue: if

  # Build the wrapper tags around the text. It turns out that caching these
  # gives pretty much zero performance gain and adds a lot of logic.

  trim_tmp =<< trim eval ENDLET
      if saved_style == "" && empty(extra_attrs)
  ENDLET
  wrapperfunc_lines += trim_tmp
  if &diff
    trim_tmp =<< trim eval ENDLET
	if diff_style_id <= 0
    ENDLET
    wrapperfunc_lines += trim_tmp
  endif
  # no surroundings if neither primary nor diff style has any info
  trim_tmp =<< trim eval ENDLET
	  return text
  ENDLET
  wrapperfunc_lines += trim_tmp
  if &diff
    # no primary style, but diff style
    trim_tmp =<< trim ENDLET
	else
	  return $'<span class="{diff_style_name}">{text}</span>'
	endif
    ENDLET
    wrapperfunc_lines += trim_tmp
  endif
  # Ignore this comment, just bypassing a highlighting issue: if

  # open tag for non-empty primary style
  trim_tmp =<< trim eval ENDLET
      else
  ENDLET
  wrapperfunc_lines += trim_tmp
  # non-empty primary style. handle either empty or non-empty diff style.
  #
  # separate the two classes by a space to apply them both if there is a diff
  # style name, unless the primary style is empty, then just use the diff style
  # name
  var diffstyle: string
  if &diff
    diffstyle = '(diff_style_id <= 0 ? "" : " " .. diff_style_name) .. '
  else
    diffstyle = ''
  endif
  if settings.prevent_copy == ""
    trim_tmp =<< trim eval ENDLET
	return "<span " .. extra_attrs .. 'class="' .. style_name .. {diffstyle}'">' .. text .. "</span>"
    ENDLET
    wrapperfunc_lines += trim_tmp
  else
    # New method: use generated content in the CSS. The only thing needed here
    # is a span with no content, with an attribute holding the desired text.
    #
    # Old method: use an <input> element when text is unsectable. This is still
    # used in conditional comments for Internet Explorer, where the new method
    # doesn't work.
    #
    # Wrap the <input> in a <span> to allow fixing the stupid bug in some fonts
    # which cause browsers to display a 1px gap between lines when these
    # <input>s have a background color (maybe not really a bug, this isn't
    # well-defined)
    #
    # use strwidth, because we care only about how many character boxes are
    # needed to size the input, we don't care how many characters (including
    # separately counted composing chars, from strchars()) or bytes (from
    # len())the string contains. strdisplaywidth() is not needed because none of
    # the unselectable groups can contain tab characters (fold column, fold
    # text, line number).
    #
    # Note, if maxlength property needs to be added in the future, it will need
    # to use strchars(), because HTML specifies that the maxlength parameter
    # uses the number of unique codepoints for its limit.
    trim_tmp =<< trim eval ENDLET
	if make_unselectable
	  var return_span = "<span " .. extra_attrs .. 'class="' .. style_name .. diffstyle .. '"'
    ENDLET
    wrapperfunc_lines += trim_tmp
    if settings.use_input_for_pc !=# 'all'
      trim_tmp =<< trim eval ENDLET
	  return_span ..= " data-" .. style_name .. '-content="' .. text .. '"'
      ENDLET
      wrapperfunc_lines += trim_tmp
    endif
    trim_tmp =<< trim eval ENDLET
	  return_span ..= '>'
    ENDLET
    wrapperfunc_lines += trim_tmp
    if settings.use_input_for_pc !=# 'none'
      trim_tmp =<< trim eval ENDLET
	  return_span ..= '<input' .. unselInputType .. ' class="' .. style_name .. diffstyle .. '"'
	  return_span ..= ' value="' .. substitute(unformatted, '\s\+$', "", "") .. '"'
	  return_span ..= " onselect='this.blur(); return false;'"
	  return_span ..= " onmousedown='this.blur(); return false;'"
	  return_span ..= " onclick='this.blur(); return false;'"
	  return_span ..= " readonly='readonly'"
	  return_span ..= ' size="' .. strwidth(unformatted) .. '"'
	  return_span ..= (settings.use_xhtml ? '/>' : '>')
      ENDLET
      wrapperfunc_lines += trim_tmp
    endif
    trim_tmp =<< trim eval ENDLET
	  return return_span .. '</span>'
	else
	  return "<span " .. extra_attrs .. 'class="' .. style_name .. diffstyle .. '">' .. text .. "</span>"
	endif
    ENDLET
    wrapperfunc_lines += trim_tmp
  endif
  trim_tmp =<< trim eval ENDLET
      endif
    enddef
  ENDLET
  wrapperfunc_lines += trim_tmp
else
  # Non-CSS method just needs the wrapper.
  #
  # Functions used to get opening/closing automatically return null strings if
  # no styles exist.
  if &diff
    wrapperfunc_lines =<< trim ENDLET
      def BuildStyleWrapper(style_id: number, diff_style_id: number, extra_attrs: string, text: string, unusedarg: bool, unusedarg2: string): string
	if diff_style_id <= 0
	  var diff_opening = HtmlOpening(diff_style_id, "")
	  var diff_closing = HtmlClosing(diff_style_id, false)
	else
	  var diff_opening = ""
	  var diff_closing = ""
	endif
	return HtmlOpening(style_id, extra_attrs) .. diff_opening .. text .. diff_closing .. HtmlClosing(style_id, !empty(extra_attrs))
      enddef
    ENDLET
  else
    wrapperfunc_lines =<< trim ENDLET
      def BuildStyleWrapper(style_id: number, diff_style_id: number, extra_attrs: string, text: string, unusedarg: bool, unusedarg2: string): string
	return HtmlOpening(style_id, extra_attrs) .. text .. HtmlClosing(style_id, !empty(extra_attrs))
      enddef
    ENDLET
  endif
endif

# create the function we built line by line above
execute join(wrapperfunc_lines, "\n")

var diff_mode = &diff

# Return HTML valid characters enclosed in a span of class style_name with
# unprintable characters expanded and double spaces replaced as necessary.
#
# TODO: eliminate unneeded logic like done for BuildStyleWrapper
def HtmlFormat(text: string, style_id: number, diff_style_id: number, extra_attrs: string, make_unselectable: bool): string
  # Replace unprintable characters
  var unformatted = strtrans(text)

  var formatted = unformatted

  # Replace the reserved html characters
  formatted = substitute(formatted, '&', '\&amp;',  'g')
  formatted = substitute(formatted, '<', '\&lt;',   'g')
  formatted = substitute(formatted, '>', '\&gt;',   'g')
  formatted = substitute(formatted, '"', '\&quot;', 'g')
  # &apos; is not valid in HTML but it is in XHTML, so just use the numeric
  # reference for it instead. Needed because it could appear in quotes
  # especially if unselectable regions is turned on.
  formatted = substitute(formatted, '"', '\&#0039;', 'g')

  # Replace a "form feed" character with HTML to do a page break
  # TODO: need to prevent this in unselectable areas? Probably it should never
  # BE in an unselectable area...
  formatted = substitute(formatted, "\x0c", '<hr class="PAGE-BREAK">', 'g')

  # Replace double spaces, leading spaces, and trailing spaces if needed
  if ' ' != HtmlSpace
    formatted = substitute(formatted, '  ', HtmlSpace .. HtmlSpace, 'g')
    formatted = substitute(formatted, '^ ', HtmlSpace, 'g')
    formatted = substitute(formatted, ' \+$', HtmlSpace, 'g')
  endif

  # Enclose in the correct format
  return BuildStyleWrapper(style_id, diff_style_id, extra_attrs, formatted, make_unselectable, unformatted)
enddef

# set up functions to call HtmlFormat in certain ways based on whether the
# element is supposed to be unselectable or not
if settings.prevent_copy =~# 'n'
  if settings.number_lines
    if settings.line_ids
      def HtmlFormat_n(text: string, style_id: number, diff_style_id: number, lnr: number): string
	if lnr > 0
	  return HtmlFormat(text, style_id, diff_style_id, 'id="' .. (exists('g:html_diff_win_num') ? 'W' .. g:html_diff_win_num : "") .. 'L' .. lnr .. settings.id_suffix .. '" ', true)
	else
	  return HtmlFormat(text, style_id, diff_style_id, "", true)
	endif
      enddef
    else
      def HtmlFormat_n(text: string, style_id: number, diff_style_id: number, lnr: number): string
	return HtmlFormat(text, style_id, diff_style_id, "", true)
      enddef
    endif
  elseif settings.line_ids
    # if lines are not being numbered the only reason this function gets called
    # is to put the line IDs on each line; "text" will be empty but lnr will
    # always be non-zero, however we don't want to use the <input> because that
    # won't work as nice for empty text
    def HtmlFormat_n(text: string, style_id: number, diff_style_id: number, lnr: number): string
      return HtmlFormat(text, style_id, diff_style_id, 'id="' .. (exists('g:html_diff_win_num') ? 'W' .. g:html_diff_win_num : "") .. 'L' .. lnr .. settings.id_suffix .. '" ', false)
    enddef
  endif
else
  if settings.line_ids
    def HtmlFormat_n(text: string, style_id: number, diff_style_id: number, lnr: number): string
      if lnr > 0
	return HtmlFormat(text, style_id, diff_style_id, 'id="' .. (exists('g:html_diff_win_num') ? 'W' .. g:html_diff_win_num : "") .. 'L' .. lnr .. settings.id_suffix .. '" ', false)
      else
	return HtmlFormat(text, style_id, diff_style_id, "", false)
      endif
    enddef
  else
    def HtmlFormat_n(text: string, style_id: number, diff_style_id: number, lnr: number): string
      return HtmlFormat(text, style_id, diff_style_id, "", false)
    enddef
  endif
endif

if settings.prevent_copy =~# 'd'
  def HtmlFormat_d(text: string, style_id: number, diff_style_id: number): string
    return HtmlFormat(text, style_id, diff_style_id, "", true)
  enddef
else
  def HtmlFormat_d(text: string, style_id: number, diff_style_id: number): string
    return HtmlFormat(text, style_id, diff_style_id, "", false)
  enddef
endif
if settings.prevent_copy =~# 'f'
  if settings.use_input_for_pc ==# 'none'
    def FoldColumn_build(char: string, len: number, numfill: number, char2: string, class: string, click: string): string
      return "<a href='#' class='" .. class .. "' onclick='" .. click .. "' data-FoldColumn-content='" ..
	repeat(char, len) .. char2 .. repeat(' ', numfill) ..
	"'></a>"
    enddef
    def FoldColumn_fill(): string
      return HtmlFormat(repeat(' ', foldcolumn), FOLD_C_ID, 0, "", true)
    enddef
  else
    # Note the <input> elements for fill spaces will have a single space for
    # content, to allow active cursor CSS selection to work.
    #
    # Wrap the whole thing in a span for the 1px padding workaround for gaps.
    #
    # Build the function line by line containing only what is needed for the
    # options in use for maximum code sharing with minimal branch logic for
    # greater speed.
    #
    # Note, 'exec' commands do not recognize line continuations, so must
    # concatenate lines rather than continue them.
    var build_fun_lines: list<string> = []
    build_fun_lines =<< trim ENDLET
      def FoldColumn_build(char: string, len: number, numfill: number, char2: string, class: string, click: string): string
	var input_open = "<input readonly='readonly'" .. unselInputType
	input_open ..= " onselect='this.blur(); return false;'"
	input_open ..= " onmousedown='this.blur(); " .. click .. " return false;'"
	input_open ..= " onclick='return false;' size='"
	input_open ..= string(len + (empty(char2) ? 0 : 1) + numfill) .. "' "
	var common_attrs = "class='FoldColumn' value='"
	var input_close = settings.use_xhtml ? "' />" : "'>"
	var return_span = "<span class='" .. class .. "'>"
	return_span ..= input_open .. common_attrs .. repeat(char, len) .. char2
	return_span ..= input_close
    ENDLET
    if settings.use_input_for_pc ==# 'fallback'
      trim_tmp =<< trim ENDLET
	return_span ..= "<a href='#' class='FoldColumn' onclick='" .. click .. "'"
	return_span ..= " data-FoldColumn-content='"
	return_span ..= repeat(char, len) .. char2 .. repeat(' ', numfill)
	return_span ..= "'></a>"
      ENDLET
      build_fun_lines += trim_tmp
    endif
    trim_tmp =<< trim ENDLET
	return_span ..= "</span>"
	return return_span
      enddef
    ENDLET
    build_fun_lines += trim_tmp
    execute join(build_fun_lines, "\n")

    def FoldColumn_fill(): string
      return FoldColumn_build(' ', foldcolumn, 0, '', 'FoldColumn', '')
    enddef
  endif
else
  # For normal fold columns, simply space-pad to the desired width (note that
  # the FoldColumn definition includes a whitespace:pre rule)
  def FoldColumn_build(char: string, len: number, numfill: number, char2: string, class: string, click: string): string
    return "<a href='#' class='" .. class .. "' onclick='" .. click .. "'>" ..
      repeat(char, len) .. char2 .. repeat(' ', numfill) ..
      "</a>"
  enddef
  def FoldColumn_fill(): string
    return HtmlFormat(repeat(' ', foldcolumn), FOLD_C_ID, 0, "", false)
  enddef
endif
if settings.prevent_copy =~# 't'
  # put an extra empty span at the end for dynamic folds, so the linebreak can
  # be surrounded. Otherwise do it as normal.
  #
  # TODO: isn't there a better way to do this, than placing it here and using a
  # substitute later?
  if settings.dynamic_folds
    def HtmlFormat_t(text: string, style_id: number, diff_style_id: number): string
      return HtmlFormat(text, style_id, diff_style_id, "", true) ..
	HtmlFormat("", style_id, 0, "", false)
    enddef
  else
    def HtmlFormat_t(text: string, style_id: number, diff_style_id: number): string
      return HtmlFormat(text, style_id, diff_style_id, "", true)
    enddef
  endif
else
  def HtmlFormat_t(text: string, style_id: number, diff_style_id: number): string
    return HtmlFormat(text, style_id, diff_style_id, "", false)
  enddef
endif

# Return CSS style describing given highlight id (can be empty)
def CSS1(id: number): string
  var a = ""
  var translated_ID = synIDtrans(id)
  if !synIDattr(translated_ID, "inverse")->empty()
    # For inverse, we always must set both colors (and exchange them)
    var x = HtmlColor(synIDattr(translated_ID, "bg#", whatterm))
    a = a .. "color: " .. (x != "" ? x : bgc) .. "; "
    x = HtmlColor(synIDattr(translated_ID, "fg#", whatterm))
    a = a .. "background-color: " .. (x != "" ? x : fgc) .. "; "
  else
    var x = HtmlColor(synIDattr(translated_ID, "fg#", whatterm))
    if x != "" | a = a .. "color: " .. x .. "; " | endif
    x = HtmlColor(synIDattr(translated_ID, "bg#", whatterm))
    if x != ""
      a = a .. "background-color: " .. x .. "; "
      # stupid hack because almost every browser seems to have at least one font
      # which shows 1px gaps between lines which have background
      a = a .. "padding-bottom: 1px; "
    elseif (translated_ID == FOLDED_ID || translated_ID == LINENR_ID || translated_ID == FOLD_C_ID) && !empty(settings.prevent_copy)
      # input elements default to a different color than the rest of the page
      a = a .. "background-color: " .. bgc .. "; "
    endif
  endif
  if !synIDattr(translated_ID, "bold")->empty() | a = a .. "font-weight: bold; " | endif
  if !synIDattr(translated_ID, "italic")->empty() | a = a .. "font-style: italic; " | endif
  if !synIDattr(translated_ID, "underline")->empty() | a = a .. "text-decoration: underline; " | endif
  return a
enddef

if settings.dynamic_folds
  # compares two folds as stored in our list of folds
  # A fold is "less" than another if it starts at an earlier line number,
  # or ends at a later line number, ties broken by fold level
  def FoldCompare(f1: dict<any>, f2: dict<any>): number
    if f1.firstline != f2.firstline
      # put it before if it starts earlier
      return f1.firstline - f2.firstline
    elseif f1.lastline != f2.lastline
      # put it before if it ends later
      return f2.lastline - f1.lastline
    else
      # if folds begin and end on the same lines, put lowest fold level first
      return f1.level - f2.level
    endif
  enddef
endif


# Set some options to make it work faster.
# Don't report changes for :substitute, there will be many of them.
# Don't change other windows; turn off scroll bind temporarily
var old_title = &title
var old_icon = &icon
var old_et = &l:et
var old_bind = &l:scrollbind
var old_report = &report
var old_search = @/
var old_more = &more
set notitle noicon
setlocal et
set nomore
set report=1000000
setlocal noscrollbind

var current_syntax: string
if exists(':ownsyntax') == 2 && exists('w:current_syntax')
  current_syntax = w:current_syntax
elseif exists('b:current_syntax')
  current_syntax = b:current_syntax
else
  current_syntax = 'none'
endif

if current_syntax == ''
  current_syntax = 'none'
endif

# If the user is sourcing this script directly then the plugin version isn't
# known because the main plugin script didn't load. In the usual case where the
# user still has the full Vim runtime installed, or has this full plugin
# installed in a package or something, then we can extract the version from the
# main plugin file at it's usual spot relative to this file. Otherwise the user
# is assembling their runtime piecemeal and we have no idea what versions of
# other files may be present so don't even try to make a guess or assume the
# presence of other specific files with specific meaning.
#
# We don't want to actually source the main plugin file here because the user
# may have a good reason not to (e.g. they define their own TOhtml command or
# something).
#
# If this seems way too complicated and convoluted, it is. Probably I should
# have put the version information in the autoload file from the start. But the
# version has been in the global variable for so long that changing it could
# break a lot of user scripts.
var pluginversion: string
if exists("g:loaded_2html_plugin")
  pluginversion = g:loaded_2html_plugin
else
  if !exists("g:unloaded_tohtml_plugin")
    var main_plugin_path = expand("<sfile>:p:h:h") .. "/plugin/tohtml.vim"
    if filereadable(main_plugin_path)
      var lines = readfile(main_plugin_path, "", 20)
      filter(lines, (_, val) => val =~ "loaded_2html_plugin = ")
      if empty(lines)
	g:unloaded_tohtml_plugin = "unknown"
      else
	g:unloaded_tohtml_plugin = substitute(lines[0], '.*loaded_2html_plugin = \([''"]\)\(\%(\1\@!.\)\+\)\1', '\2', '')
      endif
    else
      g:unloaded_tohtml_plugin = "unknown"
    endif
  endif
  pluginversion = g:unloaded_tohtml_plugin
endif

# Split window to create a buffer with the HTML file.
var orgbufnr = winbufnr(0)
var origwin_stl = &l:stl
if expand("%") == ""
  if exists('g:html_diff_win_num')
    execute 'new Untitled_win' .. g:html_diff_win_num .. '.' .. (settings.use_xhtml ? 'xhtml' : 'html')
  else
    execute 'new Untitled.' .. (settings.use_xhtml ? 'xhtml' : 'html')
  endif
else
  execute 'new %.' .. (settings.use_xhtml ? 'xhtml' : 'html')
endif

# Resize the new window to very small in order to make it draw faster
var old_winheight = winheight(0)
var old_winfixheight = &l:winfixheight
if old_winheight > 2
  resize 1 # leave enough room to view one line at a time
  norm! G
  norm! zt
endif
setlocal winfixheight

var newwin_stl = &l:stl

# on the new window, set the least time-consuming fold method
var old_fen = &foldenable
setlocal foldmethod=manual
setlocal nofoldenable

var newwin = winnr()
var orgwin = bufwinnr(orgbufnr)

setlocal modifiable
:%d
var old_paste = &paste
set paste
var old_magic = &magic
set magic

# set the fileencoding to match the charset we'll be using
&l:fileencoding = settings.vim_encoding

# According to http://www.w3.org/TR/html4/charset.html#doc-char-set, the byte
# order mark is highly recommend on the web when using multibyte encodings. But,
# it is not a good idea to include it on UTF-8 files. Otherwise, let Vim
# determine when it is actually inserted.
if settings.vim_encoding == 'utf-8'
  setlocal nobomb
else
  setlocal bomb
endif

var lines = []

var tag_close: string
if settings.use_xhtml
  if settings.encoding != ""
    add(lines, "<?xml version=\"1.0\" encoding=\"" .. settings.encoding .. "\"?>")
  else
    add(lines, "<?xml version=\"1.0\"?>")
  endif
  tag_close = ' />'
else
  tag_close = '>'
endif

var HtmlSpace = ' '
var LeadingSpace = ' '
var HtmlEndline = ''
if settings.no_pre
  HtmlEndline = '<br' .. tag_close
  LeadingSpace = settings.use_xhtml ? '&#160;' : '&nbsp;'
  HtmlSpace = '\' .. LeadingSpace
endif

# HTML header, with the title and generator ;-). Left free space for the CSS,
# to be filled at the end.
if !settings.no_doc
  extend(lines, [
    "<html>",
    "<head>"])
  # include encoding as close to the top as possible, but only if not already
  # contained in XML information (to avoid haggling over content type)
  if settings.encoding != "" && !settings.use_xhtml
    if html5
      add(lines, '<meta charset="' .. settings.encoding .. '"' .. tag_close)
    else
      add(lines, "<meta http-equiv=\"content-type\" content=\"text/html; charset=" .. settings.encoding .. '"' .. tag_close)
    endif
  endif
  extend(lines, [
    ("<title>" .. expand("%:p:~") .. "</title>"),
    ("<meta name=\"Generator\" content=\"Vim/" .. v:version / 100 .. "." .. v:version % 100 .. '"' .. tag_close),
    ("<meta name=\"plugin-version\" content=\"" .. pluginversion .. '"' .. tag_close)
  ])
  add(lines, '<meta name="syntax" content="' .. current_syntax .. '"' .. tag_close)
  add(lines, '<meta name="settings" content="' ..
    join(filter(keys(settings), (_, k) => {
      var v = settings[k]
      if type(v) == v:t_number
	return v != 0
      elseif type(v) == v:t_bool
	return !empty(v)
      else
	return false
      endif
    }), ',') ..
    ',prevent_copy=' .. settings.prevent_copy ..
    ',use_input_for_pc=' .. settings.use_input_for_pc ..
    '"' .. tag_close)
  add(lines, '<meta name="colorscheme" content="' ..
    (exists('g:colors_name')
    ? g:colors_name
    : 'none') .. '"' .. tag_close)

  if settings.use_css
    extend(lines, [
      "<style" .. (html5 ? "" : " type=\"text/css\"") .. ">",
      settings.use_xhtml ? "" : "<!--"])
    var ieonly: list<string> = []
    if settings.dynamic_folds
      if settings.hover_unfold
	# if we are doing hover_unfold, use css 2 with css 1 fallback for IE6
	extend(lines, [
	  ".FoldColumn { text-decoration: none; white-space: pre; }",
	  "",
	  "body * { margin: 0; padding: 0; }", "",
	  ".open-fold   > span.Folded { display: none;  }",
	  ".open-fold   > .fulltext   { display: inline; }",
	  ".closed-fold > .fulltext   { display: none;  }",
	  ".closed-fold > span.Folded { display: inline; }",
	  "",
	  ".open-fold   > .toggle-open   { display: none;   }",
	  ".open-fold   > .toggle-closed { display: inline; }",
	  ".closed-fold > .toggle-open   { display: inline; }",
	  ".closed-fold > .toggle-closed { display: none;   }",
	  "", "",
	  '/* opening a fold while hovering won''t be supported by IE6 and other',
	  "similar browsers, but it should fail gracefully. */",
	  ".closed-fold:hover > .fulltext      { display: inline; }",
	  ".closed-fold:hover > .toggle-filler { display: none; }",
	  ".closed-fold:hover > .Folded        { display: none; }"])
	# TODO: IE6 is REALLY old and I can't even test it anymore. Maybe we
	# should remove this? Leave it in for now, it was working at one point,
	# and doesn't affect any modern browsers. Even newer IE versions should
	# support the above code and ignore the following.
	ieonly = [
	  "<!--[if lt IE 7]><style type=\"text/css\">",
	  ".open-fold   .fulltext      { display: inline; }",
	  ".open-fold   span.Folded    { display: none; }",
	  ".open-fold   .toggle-open   { display: none; }",
	  ".open-fold   .toggle-closed { display: inline; }",
	  "",
	  ".closed-fold .fulltext      { display: none; }",
	  ".closed-fold span.Folded    { display: inline; }",
	  ".closed-fold .toggle-open   { display: inline; }",
	  ".closed-fold .toggle-closed { display: none; }",
	  "</style>",
	  "<![endif]-->",
	]
      else
	# if we aren't doing hover_unfold, use CSS 1 only
	extend(lines, [
	  ".FoldColumn { text-decoration: none; white-space: pre; }",
	  ".open-fold   .fulltext      { display: inline; }",
	  ".open-fold   span.Folded    { display: none; }",
	  ".open-fold   .toggle-open   { display: none; }",
	  ".open-fold   .toggle-closed { display: inline; }",
	  "",
	  ".closed-fold .fulltext      { display: none; }",
	  ".closed-fold span.Folded    { display: inline; }",
	  ".closed-fold .toggle-open   { display: inline; }",
	  ".closed-fold .toggle-closed { display: none; }",
	])
      endif
    endif
    # else we aren't doing any dynamic folding, no need for any special rules

    extend(lines, [
      settings.use_xhtml ? "" : '-->',
      "</style>",
    ])
    extend(lines, ieonly)
  endif

  var uses_script = settings.dynamic_folds || settings.line_ids

  # insert script tag if needed
  if uses_script
    extend(lines, [
      "",
      "<script" .. (html5 ? "" : " type='text/javascript'") .. ">",
      settings.use_xhtml ? '//<![CDATA[' : "<!--"])
  endif

  # insert javascript to toggle folds open and closed
  if settings.dynamic_folds
    extend(lines, [
      "",
      "function toggleFold(objID)",
      "{",
      "  var fold;",
      "  fold = document.getElementById(objID);",
      "  if (fold.className == 'closed-fold')",
      "  {",
      "    fold.className = 'open-fold';",
      "  }",
      "  else if (fold.className == 'open-fold')",
      "  {",
      "    fold.className = 'closed-fold';",
      "  }",
      "}"
    ])
  endif

  if settings.line_ids
    # insert javascript to get IDs from line numbers, and to open a fold before
    # jumping to any lines contained therein
    extend(lines, [
      "",
      "/* function to open any folds containing a jumped-to line before jumping to it */",
      "function JumpToLine()",
      "{",
      "  var lineNum;",
      "  lineNum = window.location.hash;",
      "  lineNum = lineNum.substr(1); /* strip off '#' */",
      "",
    "  if (lineNum.indexOf('L') == -1) {",
    "    lineNum = 'L'+lineNum;",
    "  }",
    "  var lineElem = document.getElementById(lineNum);"
    ])

    if settings.dynamic_folds
      extend(lines, [
	"",
      "  /* navigate upwards in the DOM tree to open all folds containing the line */",
      "  var node = lineElem;",
      "  while (node && node.id != 'vimCodeElement" .. settings.id_suffix .. "')",
      "  {",
      "    if (node.className == 'closed-fold')",
      "    {",
      "      node.className = 'open-fold';",
      "    }",
      "    node = node.parentNode;",
      "  }",
      ])
    endif
    extend(lines, [
      "  /* Always jump to new location even if the line was hidden inside a fold, or",
      "   * we corrected the raw number to a line ID.",
      "   */",
      "  if (lineElem) {",
      "    lineElem.scrollIntoView(true);",
      "  }",
      "  return true;",
      "}",
      "if ('onhashchange' in window) {",
      "  window.onhashchange = JumpToLine;",
      "}"
    ])
  endif

  # insert script closing tag if needed
  if uses_script
    extend(lines, [
      '',
      settings.use_xhtml ? '//]]>' : '-->',
      "</script>"
    ])
  endif

  extend(lines, ["</head>",
    "<body" .. (settings.line_ids ? " onload='JumpToLine();'" : "") .. ">"])
endif

if settings.no_pre
  # if we're not using CSS we use a font tag which can't have a div inside
  if settings.use_css
    extend(lines, ["<div id='vimCodeElement" .. settings.id_suffix .. "'>"])
  endif
else
  extend(lines, ["<pre id='vimCodeElement" .. settings.id_suffix .. "'>"])
endif

execute $":{orgwin}wincmd w"

# caches of style data
# initialize to include line numbers if using them
var stylelist: dict<string>
if settings.number_lines
  stylelist = { LINENR_ID: ".LineNr { " .. CSS1(LINENR_ID) .. "}" }
else
  stylelist = {}
endif
var diffstylelist: dict<string> = {
  DIFF_A_ID: ".DiffAdd { " .. CSS1(DIFF_A_ID) .. "}",
  DIFF_C_ID: ".DiffChange { " .. CSS1(DIFF_C_ID) .. "}",
  DIFF_D_ID: ".DiffDelete { " .. CSS1(DIFF_D_ID) .. "}",
  DIFF_T_ID: ".DiffText { " .. CSS1(DIFF_T_ID) .. "}"
}

var last_colors_name: string
# set up progress bar in the status line
# ProgressBar Indicator
# Progressbar specific functions

def SetProgbarColor()
  if hlID("TOhtmlProgress") != 0
    hi! link TOhtmlProgress_auto TOhtmlProgress
  elseif hlID("TOhtmlProgress_auto") == 0 ||
      last_colors_name != null || !exists("g:colors_name") ||
      g:colors_name != last_colors_name
    last_colors_name = exists("g:colors_name") ? g:colors_name : "none"

    var diffatr = !synIDattr(synIDtrans(hlID("DiffDelete")), "reverse", whatterm)->empty() ? "fg#" : "bg#"
    var stlatr = !synIDattr(synIDtrans(hlID("StatusLine")), "reverse", whatterm)->empty() ? "fg#" : "bg#"

    var progbar_color = synIDattr(synIDtrans(hlID("DiffDelete")), diffatr, whatterm)
    var stl_color = synIDattr(synIDtrans(hlID("StatusLine")), stlatr, whatterm)

    if empty(progbar_color)
      progbar_color = !synIDattr(synIDtrans(hlID("DiffDelete")), "reverse", whatterm)->empty() ? fgc : bgc
    endif
    if empty(stl_color)
      stl_color = !synIDattr(synIDtrans(hlID("StatusLine")), "reverse", whatterm)->empty() ? fgc : bgc
    endif

    if progbar_color == stl_color
      if whatterm == 'cterm'
	var nr = str2nr(progbar_color)
	if nr >= (str2nr(&t_Co) / 2)
	  nr -= 1
	else
	  nr += 1
	endif
	progbar_color = string(nr)
      else
	var rgb = map(matchlist(progbar_color, '#\zs\x\x\ze\(\x\x\)\(\x\x\)')[: 2], (_, v) => str2nr(v, 16))
	var avg = (rgb[0] + rgb[1] + rgb[2]) / 3
	if avg >= 128
	  var avg_new = avg
	  while avg - avg_new < 0x15
	    rgb = map(rgb, (_, v) => v * 3 / 4)
	    avg_new = (rgb[0] + rgb[1] + rgb[2]) / 3
	  endwhile
	else
	  var avg_new = avg
	  while avg_new - avg < 0x15
	    rgb = map(rgb, (_, v) => min([max([v, 4]) * 5 / 4, 255]))
	    avg_new = (rgb[0] + rgb[1] + rgb[2]) / 3
	  endwhile
	endif
	progbar_color = printf("#%02x%02x%02x", rgb[0], rgb[1], rgb[2])
      endif
      echomsg "diff detected progbar color set to" progbar_color
    endif
    execute "hi TOhtmlProgress_auto" whatterm .. "bg=" .. progbar_color
  endif
enddef

# ProgressBar object
class ProgressBar
  public var title: string
  public var max_value: number
  public var winnr: number
  public var cur_value: number = 0
  public var items: dict<any>
  public var last_value: number = 0
  public var needs_redraw: number = 0
  public var pb_len: number = 0
  public var max_len: number = 0
  public var progress_ticks: list<number> = []
  var subtractedlen: number

  def new(title: string, max_value: number, winnr: number)
    this.title = title .. ' '
    this.max_value = max_value
    this.winnr = winnr

    this.items = {
      'title': { 'color': 'Statusline' },
      'bar': { 'color': 'Statusline', 'fillcolor': 'TOhtmlProgress_auto', 'bg': 'Statusline' },
      'counter': { 'color': 'Statusline' }
    }
    # Note that you must use len(split) instead of len() if you want to use
    # unicode in title.
    #
    # Subtract 3 for spacing around the title.
    # Subtract 4 for the percentage display.
    # Subtract 2 for spacing before this.
    # Subtract 2 more for the '|' on either side of the progress bar
    this.subtractedlen = len(split(this.title, '\zs')) + 3 + 4 + 2 + 2
    set laststatus=2
  enddef

  def CalculateTicks(pb_len: number)
    if pb_len <= 0
      this.progress_ticks = range(pb_len + 1)->map((_, v) => v * this.max_value / 100)
    else
      this.progress_ticks = range(pb_len + 1)->map((_, v) => v * this.max_value / pb_len)
    endif
  enddef

  def Paint()
    # Recalculate widths.
    var max_len = winwidth(this.winnr)
    var pb_len = 0
    var cur_value: number
    # always true on first call because of initial value of this.max_len
    if max_len != this.max_len
      this.max_len = max_len

      # Progressbar length
      pb_len = max_len - this.subtractedlen

      this.CalculateTicks(pb_len)

      this.needs_redraw = 1
      pb_len = 0
      this.pb_len = pb_len
    else
      # start searching at the last found index to make the search for the
      # appropriate tick value normally take 0 or 1 comparisons
      cur_value = this.last_value
      pb_len = this.pb_len
    endif

    var cur_val_max = pb_len > 0 ? pb_len : 100

    # find the current progress bar position based on precalculated thresholds
    while cur_value < cur_val_max && this.cur_value > this.progress_ticks[cur_value]
      cur_value += 1
    endwhile

    # update progress bar
    if this.last_value != cur_value || this.needs_redraw || this.cur_value == this.max_value
      this.needs_redraw = 1
      this.last_value = cur_value

      var t_color  = this.items.title.color
      var b_fcolor = this.items.bar.fillcolor
      var b_color  = this.items.bar.color
      var c_color  = this.items.counter.color

      var stl =  $"%#{t_color}#%-( {this.title} %)%#{b_color}#" ..
	(pb_len > 0 ? $'|%#{b_fcolor}#%-({repeat(" ", cur_value)}%)%#{b_color}#{repeat(" ", pb_len - cur_value)}|"' : (''))
	.. $"%=%#{c_color}#%( {printf("%3.d ", 100 * this.cur_value / this.max_value)}%% %)"
      setwinvar(this.winnr, '&stl', stl)
    endif
  enddef

  def Incr(delta: number = 1)
    this.cur_value += delta
    # if we were making a general-purpose progress bar, we'd need to limit to a
    # lower limit as well, but since we always increment with a positive value
    # in this script, we only need limit the upper value
    this.cur_value = (this.cur_value > this.max_value ? this.max_value : this.cur_value)
    this.Paint()
  enddef
endclass

var pgb: ProgressBar
if !settings.no_progress
  if settings.dynamic_folds
    # to process folds we make two passes through each line
    pgb = ProgressBar.new("Processing folds:", line('$') * 2, orgwin)
  endif

  SetProgbarColor()
endif

var build_fun_lines: list<string>
build_fun_lines =<< trim ENDLET
  def Add_diff_fill(_lnum: number)
    var filler = diff_filler(_lnum)
    if filler > 0
      var to_insert = filler
      while to_insert > 0
	var new = repeat(difffillchar, 3)

	if to_insert > 2 && to_insert < filler && !settings.whole_filler
	  new = new .. " " .. filler .. " inserted lines "
	  to_insert = 2
	endif
ENDLET
if !settings.no_pre
  trim_tmp =<< trim ENDLET
	# HTML line wrapping is off--go ahead and fill to the margin
	# TODO: what about when CSS wrapping is turned on?
	new = new .. repeat(difffillchar, &columns - strlen(new) - margin)
  ENDLET
  build_fun_lines += trim_tmp
else
  trim_tmp =<< trim ENDLET
	new = new .. repeat(difffillchar, 3)
  ENDLET
  build_fun_lines += trim_tmp
endif
trim_tmp =<< trim ENDLET
	new = HtmlFormat_d(new, DIFF_D_ID, 0)
ENDLET
build_fun_lines += trim_tmp
if settings.number_lines
  trim_tmp =<< trim ENDLET
	# Indent if line numbering is on. Indent gets style of line number
	# column.
	new = HtmlFormat_n(repeat(' ', margin), LINENR_ID, 0, 0) .. new
  ENDLET
  build_fun_lines += trim_tmp
endif
if settings.dynamic_folds && !settings.no_foldcolumn
  trim_tmp =<< trim ENDLET
	if foldcolumn > 0
	  # Indent for foldcolumn if there is one. Assume it's empty, there should
	  # not be a fold for deleted lines in diff mode.
	  new = FoldColumn_fill() .. new
	endif
  ENDLET
  build_fun_lines += trim_tmp
endif
# Ignore this comment, just bypassing a highlighting issue: if
trim_tmp =<< trim ENDLET
	add(lines, new .. HtmlEndline)
	to_insert -= 1
      endwhile
    endif
  enddef
ENDLET
build_fun_lines += trim_tmp
execute join(build_fun_lines, "\n")

# First do some preprocessing for dynamic folding. Do this for the entire file
# so we don't accidentally start within a closed fold or something.
var allfolds: list<any> = []

var foldcolumn: number
if settings.dynamic_folds
  var lnum = 1
  var end = line('$')
  # save the fold text and set it to the default so we can find fold levels
  var foldtext_save = &foldtext
  setlocal foldtext&

  # we will set the foldcolumn in the html to the greater of the maximum fold
  # level and the current foldcolumn setting
  foldcolumn = &foldcolumn

  # get all info needed to describe currently closed folds
  while lnum <= end
    if foldclosed(lnum) == lnum
      # default fold text has '+-' and then a number of dashes equal to fold
      # level, so subtract 2 from index of first non-dash after the dashes
      # in order to get the fold level of the current fold
      var level = match(foldtextresult(lnum), '+-*\zs[^-]') - 2
      # store fold info for later use
      var newfold = {firstline: lnum, lastline: foldclosedend(lnum), level: level, type: "closed-fold"}
      add(allfolds, newfold)
      # open the fold so we can find any contained folds
      execute $":{lnum}foldopen"
    else
      if !settings.no_progress
	pgb.Incr()
	if pgb.needs_redraw
	  redrawstatus
	  pgb.needs_redraw = 0
	endif
      endif
      lnum = lnum + 1
    endif
  endwhile

  # close all folds to get info for originally open folds
  silent! %foldclose!
  lnum = 1

  # the originally open folds will be all folds we encounter that aren't
  # already in the list of closed folds
  while lnum <= end
    if foldclosed(lnum) == lnum
      # default fold text has '+-' and then a number of dashes equal to fold
      # level, so subtract 2 from index of first non-dash after the dashes
      # in order to get the fold level of the current fold
      var level = match(foldtextresult(lnum), '+-*\zs[^-]') - 2
      var newfold = {firstline: lnum, lastline: foldclosedend(lnum), level: level, type: "closed-fold"}
      # only add the fold if we don't already have it
      if empty(allfolds) || index(allfolds, newfold) == -1
	newfold.type = "open-fold"
	add(allfolds, newfold)
      endif
      # open the fold so we can find any contained folds
      execute $":{lnum}foldopen"
    else
      if !settings.no_progress
	pgb.Incr()
	if pgb.needs_redraw
	  redrawstatus
	  pgb.needs_redraw = 0
	endif
      endif
      lnum = lnum + 1
    endif
  endwhile

  # sort the folds so that we only ever need to look at the first item in the
  # list of folds
  sort(allfolds, FoldCompare)

  &l:foldtext = foldtext_save

  # close all folds again so we can get the fold text as we go
  silent! %foldclose!

  # Go through and remove folds we don't need to (or cannot) process in the
  # current conversion range
  #
  # If a fold is removed which contains other folds, which are included, we need
  # to adjust the level of the included folds as used by the conversion logic
  # (avoiding special cases is good)
  #
  # Note any time we remove a fold, either all of the included folds are in it,
  # or none of them, because we only remove a fold if neither its start nor its
  # end are within the conversion range.
  var leveladjust = 0
  for afold in allfolds
    var removed = 0
    if exists("g:html_start_line") && exists("g:html_end_line")
      if afold.firstline < g:html_start_line
	if afold.lastline <= g:html_end_line && afold.lastline >= g:html_start_line
	  # if a fold starts before the range to convert but stops within the
	  # range, we need to include it. Make it start on the first converted
	  # line.
	  afold.firstline = g:html_start_line
	else
	  # if the fold lies outside the range or the start and stop enclose
	  # the entire range, don't bother parsing it
	  remove(allfolds, index(allfolds, afold))
	  removed = 1
	  if afold.lastline > g:html_end_line
	    leveladjust += 1
	  endif
	endif
      elseif afold.firstline > g:html_end_line
	# If the entire fold lies outside the range we need to remove it.
	remove(allfolds, index(allfolds, afold))
	removed = 1
      endif
    elseif exists("g:html_start_line")
      if afold.firstline < g:html_start_line
	# if there is no last line, but there is a first line, the end of the
	# fold will always lie within the region of interest, so keep it
	afold.firstline = g:html_start_line
      endif
    elseif exists("g:html_end_line")
      # if there is no first line we default to the first line in the buffer so
      # the fold start will always be included if the fold itself is included.
      # If however the entire fold lies outside the range we need to remove it.
      if afold.firstline > g:html_end_line
	remove(allfolds, index(allfolds, afold))
	removed = 1
      endif
    endif
    if !removed
      afold.level -= leveladjust
      if afold.level + 1 > foldcolumn
	foldcolumn = afold.level + 1
      endif
    endif
  endfor

  # if we've removed folds containing the conversion range from processing,
  # getting foldtext as we go won't know to open the removed folds, so the
  # foldtext would be wrong; open them now.
  #
  # Note that only when a start and an end line is specified will a fold
  # containing the current range ever be removed.
  while leveladjust > 0
    execute $":{g:html_start_line}foldopen"
    leveladjust -= 1
  endwhile
endif

# Now loop over all lines in the original text to convert to html.
# Use html_start_line and html_end_line if they are set.
var lnum: number
if exists("g:html_start_line")
  lnum = g:html_start_line
  if lnum < 1 || lnum > line("$")
    lnum = 1
  endif
else
  lnum = 1
endif
var end: number
if exists("g:html_end_line")
  end = g:html_end_line
  if end < lnum || end > line("$")
    end = line("$")
  endif
else
  end = line("$")
endif

# stack to keep track of all the folds containing the current line
var foldstack: list<any> = []

if !settings.no_progress
  pgb = ProgressBar.new("Processing lines:", end - lnum + 1, orgwin)
endif

var margin: number
if settings.number_lines
  margin = strlen(end) + 1
else
  margin = 0
endif

var foldfillchar: string
if has('folding') && !settings.ignore_folding
  foldfillchar = &fillchars[matchend(&fillchars, 'fold:')]
  if foldfillchar == ''
    foldfillchar = '-'
  endif
endif
var difffillchar = &fillchars[matchend(&fillchars, 'diff:')]
if difffillchar == ''
  difffillchar = '-'
endif

var foldId = 0
var old_isprint: string
if !settings.expand_tabs
  # If keeping tabs, add them to printable characters so we keep them when
  # formatting text (strtrans() doesn't replace printable chars)
  old_isprint = &isprint
  setlocal isprint+=9
endif

while lnum <= end

  # If there are filler lines for diff mode, show these above the line.
  Add_diff_fill(lnum)

  # Start the line with the line number.
  var numcol: string
  if settings.number_lines
    numcol = repeat(' ', margin - 1 - strlen(lnum)) .. lnum .. ' '
  endif

  var new: string

  if has('folding') && !settings.ignore_folding && foldclosed(lnum) > -1 && !settings.dynamic_folds
    #
    # This is the beginning of a folded block (with no dynamic folding)
    new = foldtextresult(lnum)
    if !settings.no_pre
      # HTML line wrapping is off--go ahead and fill to the margin
      new ..= repeat(foldfillchar, &columns - strlen(new))
    endif

    # put numcol in a separate group for sake of unselectable text
    new = (settings.number_lines ? HtmlFormat_n(numcol, FOLDED_ID, 0, lnum): "") .. HtmlFormat_t(new, FOLDED_ID, 0)

    # Skip to the end of the fold
    var new_lnum = foldclosedend(lnum)

    if !settings.no_progress
      pgb.Incr(new_lnum - lnum)
    endif

    lnum = new_lnum

  else
    #
    # A line that is not folded, or doing dynamic folding.
    #
    var line = getline(lnum)
    var len = strlen(line)

    if settings.dynamic_folds
      # First insert a closing for any open folds that end on this line
      while !empty(foldstack) && get(foldstack, 0).lastline == lnum-1
	new ..= "</span></span>"
	remove(foldstack, 0)
      endwhile

      # Now insert an opening for any new folds that start on this line
      var firstfold = 1
      while !empty(allfolds) && get(allfolds, 0).firstline == lnum
	foldId = foldId + 1
	new ..= "<span id='"
	new ..= (exists('g:html_diff_win_num') ? "win" .. g:html_diff_win_num : "")
	new ..= "fold" .. foldId .. settings.id_suffix .. "' class='" .. allfolds[0].type .. "'>"


	# Unless disabled, add a fold column for the opening line of a fold.
	#
	# Note that dynamic folds require using css so we just use css to take
	# care of the leading spaces rather than using &nbsp; in the case of
	# html_no_pre to make it easier
	if !settings.no_foldcolumn
	  # add fold column that can open the new fold
	  if allfolds[0].level > 1 && firstfold
	    new ..= FoldColumn_build('|', allfolds[0].level - 1, 0, "",
	      'toggle-open FoldColumn', 'javascript:toggleFold("fold' .. foldstack[0].id .. settings.id_suffix .. '");')
	  endif
	  # add the filler spaces separately from the '+' char so that it can be
	  # shown/hidden separately during a hover unfold
	  new ..= FoldColumn_build("+", 1, 0, "",
	    'toggle-open FoldColumn', 'javascript:toggleFold("fold' .. foldId .. settings.id_suffix .. '");')
	  # If this is not the last fold we're opening on this line, we need
	  # to keep the filler spaces hidden if the fold is opened by mouse
	  # hover. If it is the last fold to open in the line, we shouldn't hide
	  # them, so don't apply the toggle-filler class.
	  new ..= FoldColumn_build(" ", 1, foldcolumn - allfolds[0].level - 1, "",
	    'toggle-open FoldColumn' .. (get(allfolds, 1, {firstline: 0}).firstline == lnum ? " toggle-filler" : ""),
	    'javascript:toggleFold("fold' .. foldId .. settings.id_suffix .. '");')

	  # add fold column that can close the new fold
	  # only add extra blank space if we aren't opening another fold on the
	  # same line
	  var extra_space: number
	  if get(allfolds, 1, {firstline: 0}).firstline != lnum
	    extra_space = foldcolumn - allfolds[0].level
	  else
	    extra_space = 0
	  endif
	  if firstfold
	    # the first fold in a line has '|' characters from folds opened in
	    # previous lines, before the '-' for this fold
	    new ..= FoldColumn_build('|', allfolds[0].level - 1, extra_space, '-',
	      'toggle-closed FoldColumn', 'javascript:toggleFold("fold' .. foldId .. settings.id_suffix .. '");')
	  else
	    # any subsequent folds in the line only add a single '-'
	    new ..= FoldColumn_build("-", 1, extra_space, "",
	      'toggle-closed FoldColumn', 'javascript:toggleFold("fold' .. foldId .. settings.id_suffix .. '");')
	  endif
	  firstfold = 0
	endif

	# Add fold text, moving the span ending to the next line so collapsing
	# of folds works correctly.
	# Put numcol in a separate group for sake of unselectable text.
	new ..= (settings.number_lines ? HtmlFormat_n(numcol, FOLDED_ID, 0, 0) : "") .. substitute(HtmlFormat_t(foldtextresult(lnum), FOLDED_ID, 0), '</span>', HtmlEndline .. '\n\0', '')
	new ..= "<span class='fulltext'>"

	# open the fold now that we have the fold text to allow retrieval of
	# fold text for subsequent folds
	execute $":{lnum}foldopen"
	insert(foldstack, remove(allfolds, 0), 0)
	foldstack[0].id = foldId
      endwhile

      # Unless disabled, add a fold column for other lines.
      #
      # Note that dynamic folds require using css so we just use css to take
      # care of the leading spaces rather than using &nbsp; in the case of
      # html_no_pre to make it easier
      if !settings.no_foldcolumn
	if empty(foldstack)
	  # add the empty foldcolumn for unfolded lines if there is a fold
	  # column at all
	  if foldcolumn > 0
	    new = new .. FoldColumn_fill()
	  endif
	else
	  # add the fold column for folds not on the opening line
	  if get(foldstack, 0).firstline < lnum
	    new = new .. FoldColumn_build('|', foldstack[0].level, foldcolumn - foldstack[0].level, "",
	      'FoldColumn', 'javascript:toggleFold("fold' .. foldstack[0].id .. settings.id_suffix .. '");')
	  endif
	endif
      endif
    endif

    # Now continue with the unfolded line text
    if settings.number_lines
      new ..= HtmlFormat_n(numcol, LINENR_ID, 0, lnum)
    elseif settings.line_ids
      new ..= HtmlFormat_n("", LINENR_ID, 0, lnum)
    endif

    # Get the diff attribute, if any.
    var diffattr = diff_hlID(lnum, 1)

    # initialize conceal info to act like not concealed, just in case
    var concealinfo = [0, '']

    # Loop over each character in the line
    var col = 1

    # most of the time we won't use the diff_id, initialize to zero
    var diff_id = 0

    while col <= len || (col == 1 && diffattr)
      var id: number
      var startcol = col # The start column for processing text
      if !settings.ignore_conceal && has('conceal')
	concealinfo = synconcealed(lnum, col)
      endif
      if !settings.ignore_conceal && concealinfo[0]
	col = col + 1
	# Speed loop (it's small - that's the trick)
	# Go along till we find a change in the match sequence number (ending
	# the specific concealed region) or until there are no more concealed
	# characters.
	while col <= len && concealinfo == synconcealed(lnum, col) | col = col + 1 | endwhile
      elseif diffattr
	diff_id = diff_hlID(lnum, col)
	id = synID(lnum, col, 1)
	col = col + 1
	# Speed loop (it's small - that's the trick)
	# Go along till we find a change in hlID
	while col <= len && id == synID(lnum, col, 1) && diff_id == diff_hlID(lnum, col)
	  col = col + 1
	endwhile
	if len < &columns && !settings.no_pre
	  # Add spaces at the end of the raw text line to extend the changed
	  # line to the full width.
	  line = line .. repeat(' ', &columns - virtcol([lnum, len]) - margin)
	  len = &columns
	endif
      else
	id = synID(lnum, col, 1)
	col = col + 1
	# Speed loop (it's small - that's the trick)
	# Go along till we find a change in synID
	while col <= len && id == synID(lnum, col, 1) | col = col + 1 | endwhile
      endif
      var expandedtab: string
      if settings.ignore_conceal || !concealinfo[0]
	# Expand tabs if needed
	expandedtab = strpart(line, startcol - 1, col - startcol)
	if settings.expand_tabs
	  var offset = 0
	  var idx = stridx(expandedtab, "\t")
	  var tablist = split(&vts, ',')
	  if empty(tablist)
	    tablist = [ &ts ]
	  endif
	  var tabidx = 0
	  var tabwidth = 0
	  var i: number
	  while idx >= 0
	    if startcol + idx == 1
	      i = tablist[0]
	    else
	      # Get the character, which could be multiple bytes, which falls
	      # immediately before the found tab. Extract it by matching a
	      # character just prior to the column where the tab matches.
	      # We'll use this to get the byte index of the character
	      # immediately preceding the tab, so we can then look up the
	      # virtual column that character appears in, to determine how
	      # much of the current tabstop has been used up.
	      var prevc: string
	      if idx == 0
		# if the found tab is the first character in the text being
		# processed, we need to get the character prior to the text,
		# given by startcol.
		prevc = matchstr(line, '.\%' .. (startcol + offset) .. 'c')
	      else
		# Otherwise, the byte index of the tab into expandedtab is
		# given by idx.
		prevc = matchstr(expandedtab, '.\%' .. (idx + 1) .. 'c')
	      endif
	      var vcol = virtcol([lnum, startcol + idx + offset - len(prevc)])

	      # find the tabstop interval to use for the tab we just found. Keep
	      # adding tabstops (which could be variable) until we would exceed
	      # the virtual screen position of the start of the found tab.
	      while vcol >= tabwidth + tablist[tabidx]
		tabwidth += tablist[tabidx]
		if tabidx < len(tablist) - 1
		  tabidx = tabidx + 1
		endif
	      endwhile
	      i = tablist[tabidx] - (vcol - tabwidth)
	    endif
	    # update offset to keep the index within the line corresponding to
	    # actual tab characters instead of replaced spaces; idx reflects
	    # replaced spaces in expandedtab, offset cancels out all but
	    # the tab character itself.
	    offset -= i - 1
	    expandedtab = substitute(expandedtab, '\t', repeat(' ', i), '')
	    idx = stridx(expandedtab, "\t")
	  endwhile
	endif

	# get the highlight group name to use
	id = synIDtrans(id)
      else
	# use Conceal highlighting for concealed text
	id = CONCEAL_ID
	expandedtab = concealinfo[1]
      endif

      # Output the text with the same synID, with class set to the highlight ID
      # name, unless it has been concealed completely.
      if strlen(expandedtab) > 0
	new = new .. HtmlFormat(expandedtab, id, diff_id, "", false)
      endif
    endwhile
  endif

  extend(lines, split(new .. HtmlEndline, '\n', 1))
  if !settings.no_progress && pgb.needs_redraw
    redrawstatus
    pgb.needs_redraw = 0
  endif
  lnum = lnum + 1

  if !settings.no_progress
    pgb.Incr()
  endif
endwhile

# Diff filler is returned based on what needs inserting *before* the given line.
# So to get diff filler at the end of the buffer, we need to use last line + 1
Add_diff_fill(end + 1)

if settings.dynamic_folds
  # finish off any open folds
  while !empty(foldstack)
    lines[-1] ..= "</span></span>"
    remove(foldstack, 0)
  endwhile

  # add fold column to the style list if not already there
  var id = FOLD_C_ID
  if !has_key(stylelist, id)
    stylelist[id] = '.FoldColumn { ' .. CSS1(id) .. '}'
  endif
endif

if settings.no_pre
  if !settings.use_css
    # Close off the font tag that encapsulates the whole <body>
    extend(lines, ["</font>"])
  else
    extend(lines, ["</div>"])
  endif
else
  extend(lines, ["</pre>"])
endif
if !settings.no_doc
  extend(lines, ["</body>", "</html>"])
endif

execute $":{newwin}wincmd w"
setline(1, lines)

# Mangle modelines so Vim doesn't try to use HTML text as a modeline if editing
# this file in the future; need to do this after generating all the text in case
# the modeline text has different highlight groups which all turn out to be
# stripped from the final output.
:%s!\v(%(^|\s+)%([Vv]i%(m%([<=>]?\d+)?)?|ex)):!\1\&#0058;!ge

# The generated HTML is admittedly ugly and takes a LONG time to fold.
# Make sure the user doesn't do syntax folding when loading a generated file,
# using a modeline.
if !settings.no_modeline
  append(line('$'), "<!-- vim: set foldmethod=manual : -->")
endif

# Now, when we finally know which, we define the colors and styles
if settings.use_css && !settings.no_doc
  :1;/<style\>/+1

  # Normal/global attributes
  if settings.no_pre
    append('.', "body { color: " .. fgc .. "; background-color: " .. bgc .. "; font-family: " .. htmlfont .. "; }")
    :+
  else
    append('.', "pre { " .. whitespace .. "font-family: " .. htmlfont .. "; color: " .. fgc .. "; background-color: " .. bgc .. "; }")
    :+
    yank
    put
    execute "normal! ^cwbody\e"
    # body should not have the wrap formatting, only the pre section
    if whitespace != ''
      execute 's#' .. whitespace
    endif
  endif
  # fix browser inconsistencies (sometimes within the same browser) of different
  # default font size for different elements
  append('.', '* { font-size: 1em; }')
  :+
  # use color scheme styles for links
  # browser-default blue/purple colors for links don't look like the existing theme and are unreadable on dark backgrounds
  append('.', 'a { color: inherit; }')
  :+
  # if we use any input elements for unselectable content, make sure they look
  # like normal text
  if !empty(settings.prevent_copy)
    if settings.use_input_for_pc !=# "none"
      append('.', 'input { border: none; margin: 0; padding: 0; font-family: ' .. htmlfont .. '; }')
      :+
      # ch units for browsers which support them, em units for a somewhat
      # reasonable fallback.
      for w in range(1, 20, 1)
	append('.', [
	  "input[size='" .. w .. "'] { width: " .. w .. "em; width: " .. w .. "ch; }"
	])
	:+
      endfor
    endif

    if settings.use_input_for_pc !=# 'all'
      var unselectable_styles: list<string> = []
      if settings.prevent_copy =~# 'f'
	add(unselectable_styles, 'FoldColumn')
      endif
      if settings.prevent_copy =~# 'n'
	add(unselectable_styles, 'LineNr')
      endif
      if settings.prevent_copy =~# 't' && !settings.ignore_folding
	add(unselectable_styles, 'Folded')
      endif
      if settings.prevent_copy =~# 'd'
	add(unselectable_styles, 'DiffDelete')
      endif
      if settings.use_input_for_pc !=# 'none'
	append('.', [
	  '/* Note: IE does not support @supports conditionals, but also does not fully support',
	  '   "content:" with custom content, so we *want* the check to fail */',
	  '@supports ( content: attr(data-custom-content) ) {'
	])
	:+3
      endif
      # The line number column inside the foldtext is styled just like the fold
      # text in Vim, but it should use the prevent_copy settings of line number
      # rather than fold text. Apply the prevent_copy styles to foldtext
      # specifically for line numbers, which always come after the fold column,
      # or at the beginning of the line.
      if settings.prevent_copy =~# 'n' && !settings.ignore_folding
	append('.', [
	  '  .FoldColumn + .Folded, .Folded:first-child { user-select: none; }',
	  '  .FoldColumn + [data-Folded-content]::before, [data-Folded-content]:first-child::before { content: attr(data-Folded-content); }',
	  '  .FoldColumn + [data-Folded-content]::before, [data-Folded-content]:first-child::before { padding-bottom: 1px; display: inline-block; /* match the 1-px padding of standard items with background */ }',
	  '  .FoldColumn + span[data-Folded-content]::before, [data-Folded-content]:first-child::before { cursor: default; }',
	])
	:+4
      endif
      for style_name in unselectable_styles
	append('.', [
	  '  .' .. style_name .. ' { user-select: none; }',
	  '  [data-' .. style_name .. '-content]::before { content: attr(data-' .. style_name .. '-content); }',
	  '  [data-' .. style_name .. '-content]::before { padding-bottom: 1px; display: inline-block; /* match the 1-px padding of standard items with background */ }',
	  '  span[data-' .. style_name .. '-content]::before { cursor: default; }',
	])
	:+4
      endfor
      if settings.use_input_for_pc !=# 'none'
	# Note, the extra '}' is to match the "@supports" above
	append('.', [
	  '  input { display: none; }',
	  '}'
	])
	:+2
      endif
    endif

    # Fix mouse cursor shape for the fallback <input> method of uncopyable text
    if settings.use_input_for_pc !=# 'none'
      if settings.prevent_copy =~# 'f'
	# Make the cursor show active fold columns as active areas, and empty fold
	# columns as not interactive.
	append('.', ['input.FoldColumn { cursor: pointer; }',
	  'input.FoldColumn[value="' .. repeat(' ', foldcolumn) .. '"] { cursor: default; }'
	])
	:+2
	if settings.use_input_for_pc !=# 'all'
	  append('.', [
	    'a[data-FoldColumn-content="' .. repeat(' ', foldcolumn) .. '"] { cursor: default; }'
	  ])
	  :+1
	endif
      endif
      # make line number column show as non-interactive if not selectable
      if settings.prevent_copy =~# 'n'
	append('.', 'input.LineNr { cursor: default; }')
	:+
      endif
      # make fold text and line number column within fold text show as
      # non-interactive if not selectable
      if (settings.prevent_copy =~# 'n' || settings.prevent_copy =~# 't') && !settings.ignore_folding
	append('.', 'input.Folded { cursor: default; }')
	:+
      endif
      # make diff filler show as non-interactive if not selectable
      if settings.prevent_copy =~# 'd'
	append('.', 'input.DiffDelete { cursor: default; }')
	:+
      endif
    endif
  endif
endif

if !settings.use_css && !settings.no_doc
  # For Netscape 4, set <body> attributes too, though, strictly speaking, it's
  # incorrect.
  execute $':%s/<body\([^>]*\)/<body bgcolor="{bgc}" text="{fgc}"\1>\r<font face="{htmlfont}"'
endif

# Gather attributes for all other classes. Do diff first so that normal
# highlight groups are inserted before it.
if settings.use_css && !settings.no_doc
  if diff_mode
    append('.', filter(map(keys(diffstylelist), (_, k) => diffstylelist[k]), (_, v) => v != ""))
  endif
  if !empty(stylelist)
    append('.', filter(map(keys(stylelist), (_, k) => stylelist[k]), (_, v) => v != ""))
  endif
endif

# Add hyperlinks
if !settings.no_links
  :%s+\(https\=://\S\{-}\)\(\([.,;:}]\=\(\s\|$\)\)\|[\\"'<>]\|&gt;\|&lt;\|&quot;\)+<a href="\1">\1</a>\2+ge
endif

# The DTD
if !settings.no_doc
  if settings.use_xhtml
    execute "normal! gg$a\n<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">"
  elseif html5
    execute "normal! gg0i<!DOCTYPE html>\n"
  else
    execute "normal! gg0i<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\" \"http://www.w3.org/TR/html4/loose.dtd\">\n"
  endif
endif

if settings.use_xhtml && !settings.no_doc
  execute "normal! gg/<html/e\na xmlns=\"http://www.w3.org/1999/xhtml\"\e"
endif

# Cleanup
:%s/\s\+$//e

# Restore old settings (new window first)
#
# Don't bother restoring foldmethod in case it was syntax because the markup is
# so weirdly formatted it can take a LONG time.
&l:foldenable = old_fen
&report = old_report
&title = old_title
&icon = old_icon
&paste = old_paste
&magic = old_magic
@/ = old_search
&more = old_more

# switch to original window to restore those settings
execute $":{orgwin}wincmd w"

if !settings.expand_tabs
  &l:isprint = old_isprint
endif
&l:stl = origwin_stl
&l:et = old_et
&l:scrollbind = old_bind

# and back to the new window again to end there
execute $":{newwin}wincmd w"

&l:stl = newwin_stl
execute 'resize' old_winheight
&l:winfixheight = old_winfixheight

&laststatus = ls_sav
&eventignore = ei_sav

# Make sure any patches will probably use consistent indent
#   vim: ts=8 sw=2 sts=2 noet
