if exists('g:loaded_tutor_mode_plugin') || &compatible
    finish
endif
let g:loaded_tutor_mode_plugin = 1

" Define this variable so that users get cmdline completion.
if !exists('g:tutor_debug')
  let g:tutor_debug = 0
endif

command! -nargs=? -complete=custom,tutor#TutorCmdComplete Tutor call tutor#TutorCmd(<q-args>)
