vim9script

# Script to get various codes that keys send, depending on the protocol used.
#
# Usage:  vim -u keycode_check.vim
#
# Author: 	Bram Moolenaar
# Last Update: 	2022 Nov 15
#
# The codes are stored in the file "keycode_check.json", so that you can
# compare the results of various terminals.
#
# You can select what protocol to enable:
# - None
# - modifyOtherKeys level 2
# - kitty keyboard protocol

# Change directory to where this script is, so that the json file is found
# there.
exe 'cd ' .. expand('<sfile>:h')
echo 'working in directory: ' .. getcwd()

const filename = 'keycode_check.json'

# Dictionary of dictionaries with the results in the form:
# {'xterm': {protocol: 'none', 'Tab': '09', 'S-Tab': '09'},
#  'xterm2': {protocol: 'mok2', 'Tab': '09', 'S-Tab': '09'},
#  'kitty': {protocol: 'kitty', 'Tab': '09', 'S-Tab': '09'},
# }
# The values are in hex form.
var keycodes = {}

if filereadable(filename)
  keycodes = readfile(filename)->join()->json_decode()
else
  # Use some dummy entries to try out with
  keycodes = {
    'xterm': {protocol: 'none', 'Tab': '09', 'S-Tab': '09'},
    'kitty': {protocol: 'kitty', 'Tab': '09', 'S-Tab': '1b5b393b3275'},
    }
endif
var orig_keycodes = deepcopy(keycodes)  # used to detect something changed

# Write the "keycodes" variable in JSON form to "filename".
def WriteKeycodes()
  # If the file already exists move it to become the backup file.
  if filereadable(filename)
    if rename(filename, filename .. '~')
      echoerr $'Renaming {filename} to {filename}~ failed!'
      return
    endif
  endif

  if writefile([json_encode(keycodes)], filename) != 0
    echoerr $'Writing {filename} failed!'
  endif
enddef

# The key entries that we want to list, in this order.
# The first item is displayed in the prompt, the second is the key in
# the keycodes dictionary.
var key_entries = [
	['Tab', 'Tab'],
	['Shift-Tab', 'S-Tab'],
	['Ctrl-Tab', 'C-Tab'],
	['Alt-Tab', 'A-Tab'],
	['Ctrl-I', 'C-I'],
	['Shift-Ctrl-I', 'S-C-I'],
	['Esc', 'Esc'],
	['Shift-Esc', 'S-Esc'],
	['Ctrl-Esc', 'C-Esc'],
	['Alt-Esc', 'A-Esc'],
	['Space', 'Space'],
	['Shift-Space', 'S-Space'],
	['Ctrl-Space', 'C-Space'],
	['Alt-Space', 'A-Space'],
      ]


# Action: list the information in "keycodes" in a more or less nice way.
def ActionList()
  var terms = keys(keycodes)
  if len(terms) == 0
    echo 'No terminal results yet'
    return
  endif

  # Use one column of width 10 for the item name, then columns of 20
  # characters to fit most codes.  You will need to increase the terminal
  # width to avoid wrapping.
  echon printf('         ')
  for term in terms
    echon printf('%-20s', term)
  endfor
  echo "\n"

  var items = ['protocol'] + key_entries->copy()->map((_, v) => v[1])

  for item in items
    echon printf('%8s  ', item)
    for term in terms
      var val = get(keycodes[term], item, '')

      # see if we can pretty-print this one
      var pretty = val
      if val[0 : 1] == '1b'
	pretty = 'ESC'
	var idx = 2

	if val[0 : 3] == '1b5b'
	  pretty = 'CSI'
	  idx = 4
	endif

	var digits = false
	while idx < len(val)
	  var cc = val[idx : idx + 1]
	  var nr = str2nr('0x' .. cc, 16)
	  idx += 2
	  if nr >= char2nr('0') && nr <= char2nr('9')
	    if !digits
	      pretty ..= ' '
	    endif
	    digits = true
	    pretty ..= cc[1]
	  else
	    digits = false
	    if nr >= char2nr(' ') && nr <= char2nr('~')
	      # printable character
	      pretty ..= ' ' .. printf('%c', nr)
	    else
	      # non-printable, use hex code
	      pretty = val
	      break
	    endif
	  endif
	endwhile
      endif

      echon printf('%-20s', pretty)
    endfor
    echo ''
  endfor
  echo "\n"
enddef

def GetTermName(): string
  var name = input('Enter the name of the terminal: ')
  return name
enddef

# Gather key codes for terminal "name".
def DoTerm(name: string)
  var proto = inputlist([$'What protocol to enable for {name}:',
			 '1. None',
			 '2. modifyOtherKeys level 2',
			 '3. kitty',
			])
  echo "\n"
  &t_TE = "\<Esc>[>4;m"
  var proto_name = 'none'
  if proto == 1
    &t_TI = ""
  elseif proto == 2
    &t_TI = "\<Esc>[>4;2m"
    proto_name = 'mok2'
  elseif proto == 3
    &t_TI = "\<Esc>[>1u"
    proto_name = 'kitty'
  else
    echoerr 'invalid protocol choice'
    return
  endif

  # executing a dummy shell command will output t_TI
  !echo >/dev/null

  if !has_key(keycodes, name)
    keycodes[name] = {}
  endif
  keycodes[name]['protocol'] = proto_name

  echo "When a key press doesn't get to Vim (e.g. when using Alt) press Space"

  for entry in key_entries
    ch_logfile('keylog', 'w')
    echo $'Press the {entry[0]} key (q to quit):'
    var r = getcharstr()
    ch_logfile('', '')
    if r == 'q'
      break
    endif
    var log = readfile('keylog')
    delete('keylog')
    if len(log) < 2
      echoerr 'failed to read result'
      return
    endif
    var done = false
    for line in log
      if line =~ 'raw key input'
	var code = substitute(line, '.*raw key input: "\([^"]*\).*', '\1', '')

	# convert the literal bytes into hex
	var hex = ''
	for i in range(len(code))
	  hex ..= printf('%02x', char2nr(code[i]))
	endfor
	keycodes[name][entry[1]] = hex
	done = true
	break
      endif
    endfor
    if !done
      echo 'Code not found in log'
    endif
  endfor
enddef

# Action: Add key codes for a new terminal.
def ActionAdd()
  var name = input('Enter name of the terminal: ')
  echo "\n"
  if index(keys(keycodes), name) >= 0
    echoerr $'Terminal {name} already exists'
    return
  endif

  DoTerm(name)
enddef

# Action: Replace key codes for an already known terminal.
def ActionReplace()
  var terms = keys(keycodes)
  if len(terms) == 0
    echo 'No terminal results yet'
    return
  endif

  var choice = inputlist(['Select:'] + terms->copy()->map((idx, arg) => (idx + 1) .. ': ' .. arg))
  echo "\n"
  if choice > 0 && choice <= len(terms)
    DoTerm(terms[choice - 1])
  endif
  echo 'invalid index'
enddef

# Action: Quit, possibly after saving the results first.
def ActionQuit()
  # If nothing was changed just quit
  if keycodes == orig_keycodes
    quit
  endif

  while true
    var res = input("Save the changed key codes (y/n)? ")
    if res == 'n'
      quit
    endif
    if res == 'y'
      WriteKeycodes()
      quit
    endif
    echo 'invalid reply'
  endwhile
enddef

# The main loop
while true
  var action = inputlist(['Select operation:',
    			'1. List results',
			'2. Add results for a new terminal',
			'3. Replace results',
			'4. Quit',
		      ])
  echo "\n"
  if action == 1
    ActionList()
  elseif action == 2
    ActionAdd()
  elseif action == 3
    ActionReplace()
  elseif action == 4
    ActionQuit()
  endif
endwhile
