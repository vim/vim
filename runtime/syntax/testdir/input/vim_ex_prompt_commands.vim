" Vim :prompt{find,repl} commands


promptfind foo
promptrepl foo

promptfind a
      \ really
      \ long
      \ search
      \ string
promptrepl a
      \ really
      \ long
      \ search
      \ string


" no tail comment or trailing bar

promptfind foo " more search string
promptfind foo | more search string


def Vim9Context()
  promptfind foo
  promptrepl foo

  promptfind a
	\ really
	\ long
	\ search
	\ string
  promptrepl a
	\ really
	\ long
	\ search
	\ string


  # no tail comment or trailing bar

  promptfind foo # more search string
  promptfind foo | more search string
enddef

