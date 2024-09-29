" Test for all command modifiers in

def Test_cmdmods_array()
  # Get all the command modifiers from ex_cmds.h.
  var lines = readfile('../ex_cmds.h')->filter((_, l) => l =~ 'ex_wrongmodifier,')
  var cmds = lines->map((_, v) => substitute(v, '.*"\(\k*\)".*', '\1', ''))

  # :hide is both a command and a modifier
  cmds->extend(['hide'])

  # Get the entries of cmdmod_info_tab[] in ex_docmd.c
  edit ../ex_docmd.c
  var top = search('^static cmdmod_info_T cmdmod_info_tab[') + 1
  var bot = search('^};.*\/\/ cmdmod_info_tab') - 1
  lines = getline(top, bot)
  var mods = lines->map((_, v) => substitute(v, '.*"\(\k*\)".*', '\1', ''))

  # Add the other commands that use ex_wrongmodifier.
  mods->extend([
                'endclass',
                'endenum',
                'endinterface',
                'public',
                'static',
                'this',
              ])

  # Check the lists are equal.  Convert them to a dict to get a clearer error
  # message.
  var cmds_dict = {}
  for v in cmds
    cmds_dict[v] = 1
  endfor
  var mods_dict = {}
  for v in mods
    mods_dict[v] = 1
  endfor
  assert_equal(cmds_dict, mods_dict)

  bwipe!
enddef

def Test_keep_cmdmods_names()
  # :k only available in legacy script
  legacy call assert_equal('k', fullcommand(':k'))
  legacy call assert_equal('k', fullcommand(':ke'))
  # single character commands not supported in Vim9
  assert_equal('', fullcommand(':k'))
  assert_equal('keepmarks', fullcommand(':ke'))
  assert_equal('keepmarks', fullcommand(':kee'))
  assert_equal('keepmarks', fullcommand(':keep'))
  assert_equal('keepmarks', fullcommand(':keepm'))
  assert_equal('keepmarks', fullcommand(':keepma'))
  assert_equal('keepmarks', fullcommand(':keepmar'))
  assert_equal('keepmarks', fullcommand(':keepmark'))
  assert_equal('keepmarks', fullcommand(':keepmarks'))
  assert_equal('keepalt', fullcommand(':keepa'))
  assert_equal('keepalt', fullcommand(':keepal'))
  assert_equal('keepalt', fullcommand(':keepalt'))
  assert_equal('keepjumps', fullcommand(':keepj'))
  assert_equal('keepjumps', fullcommand(':keepju'))
  assert_equal('keepjumps', fullcommand(':keepjum'))
  assert_equal('keepjumps', fullcommand(':keepjump'))
  assert_equal('keepjumps', fullcommand(':keepjumps'))
  assert_equal('keeppatterns', fullcommand(':keepp'))
  assert_equal('keeppatterns', fullcommand(':keeppa'))
  assert_equal('keeppatterns', fullcommand(':keeppat'))
  assert_equal('keeppatterns', fullcommand(':keeppatt'))
  assert_equal('keeppatterns', fullcommand(':keeppatte'))
  assert_equal('keeppatterns', fullcommand(':keeppatter'))
  assert_equal('keeppatterns', fullcommand(':keeppattern'))
  assert_equal('keeppatterns', fullcommand(':keeppatterns'))
enddef

def Test_cmdmod_completion()
  assert_equal('edit', getcompletion('keepalt ed',      'cmdline')[0])
  assert_equal('edit', getcompletion('keepjumps ed',    'cmdline')[0])
  assert_equal('edit', getcompletion('keepmarks ed',    'cmdline')[0])
  assert_equal('edit', getcompletion('keeppatterns ed', 'cmdline')[0])
enddef

" vim: shiftwidth=2 sts=2 expandtab

