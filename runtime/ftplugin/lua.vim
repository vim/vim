" Vim filetype plugin file.
" Language:		Lua
" Maintainer:		Doug Kearns <dougkearns@gmail.com>
" Previous Maintainer:	Max Ischenko <mfi@ukr.net>
" Contributor:		Dorai Sitaram <ds26@gte.com>
"			C.D. MacEachern <craig.daniel.maceachern@gmail.com>
"			Tyler Miller <tmillr@proton.me>
" Last Change:		2024 Jan 14

if exists("b:did_ftplugin")
  finish
endif
let b:did_ftplugin = 1

let s:cpo_save = &cpo
set cpo&vim

setlocal comments=:---,:--
setlocal commentstring=--\ %s
setlocal formatoptions-=t formatoptions+=croql

let &l:define = '\<function\|\<local\%(\s\+function\)\='

" TODO: handle init.lua
setlocal includeexpr=tr(v:fname,'.','/')
setlocal suffixesadd=.lua

let b:undo_ftplugin = "setlocal cms< com< def< fo< inex< sua<"

if exists("loaded_matchit") && !exists("b:match_words")
  let b:match_ignorecase = 0
  let b:match_words =
	\ '\<\%(do\|function\|if\)\>:' ..
	\ '\<\%(return\|else\|elseif\)\>:' ..
	\ '\<end\>,' ..
	\ '\<repeat\>:\<until\>,' ..
	\ '\%(--\)\=\[\(=*\)\[:]\1]'
  let b:undo_ftplugin ..= " | unlet! b:match_words b:match_ignorecase"
endif

if (has("gui_win32") || has("gui_gtk")) && !exists("b:browsefilter")
  let b:browsefilter = "Lua Source Files (*.lua)\t*.lua\n"
  if has("win32")
    let b:browsefilter ..= "All Files (*.*)\t*\n"
  else
    let b:browsefilter ..= "All Files (*)\t*\n"
  endif
  let b:undo_ftplugin ..= " | unlet! b:browsefilter"
endif

" fold Lua functions
silent! function FoldLuaLevel(lnum)
  lua foldlevel_luacode()
endfunction

if has("folding") && get(g:, "lua_folding", 0)
  setlocal foldexpr=FoldLuaLevel(v:lnum)
  setlocal foldmethod=expr
  let b:undo_ftplugin ..= "|setl foldexpr< foldmethod<"
endif

let &cpo = s:cpo_save
unlet s:cpo_save

" The rest of the file needs to be :sourced only once per session.
if exists('s:loaded_lua') || !get(g:, "lua_folding", 0) | finish | endif
let s:loaded_lua = 1

" From https://github.com/ElPiloto/Lua-Omni-Vim-Completion/blob/7044c7010d4e6f59da60704a4a8e2118af7e973b/ftplugin/lua_omni.lua#L715C1-L738C4

lua << EOF
--- Iterator which scans Vim buffer and returns on each call a supposed fold level, line number and line itself. Parsing is simplified but should be good enough for most of the time.
-- @param buf a Vim buffer to scan, nil for current buffer
-- @param fromline a line number from which scanning starts, nil for 1
-- @param toline a line number at which scanning stops, nil for the last buffer's line
-- @return fold level, line number, line's content
function fold_iter(buf, fromline, toline)
  assert(fromline == nil or type(fromline) == "number", "fromline must be a number if specified!")
  buf = buf or vim.buffer()
  toline = toline or #buf
  assert(type(toline) == "number", "toline must be a number if specified!")

  local lineidx = fromline and (fromline - 1) or 0
  -- to remember consecutive folds
  local foldlist = {}
  -- closure blocks opening/closing statements
  local patterns = {{"do", "end"},
                    {"repeat", "until%s+.+"},
                    {"if%s+.+%s+then", "end"},
                    {"for%s+.+%s+do", "end"},
                    {"function.+", "end"},
                    {"return%s+function.+", "end"},
                    {"local%s+function%s+.+", "end"},
                   }

  return function()
    lineidx = lineidx + 1
    if lineidx <= toline then
      -- search for one of blocks statements
      for i, t in ipairs(patterns) do
        -- add whole line anchors
        local tagopen = '^%s*' .. t[1] .. '%s*$'
        local tagclose = '^%s*' .. t[2] .. '%s*$'
        -- try to find opening statement
        if string.find(buf[lineidx], tagopen) then
          -- just remember it
          table.insert(foldlist, t)
        elseif string.find(buf[lineidx], tagclose) then     -- check for closing statement
          -- Proceed only if there is unclosed block in foldlist and its
          -- closing statement matches.
          if #foldlist > 0 and string.find(buf[lineidx], foldlist[#foldlist][2]) then
            table.remove(foldlist)
            -- Add 1 to foldlist length (synonymous to fold level) to include
            -- closing statement in the fold too.
            return #foldlist + 1, lineidx, buf[lineidx]
          else
            -- An incorrect situation where opening/closing statements didn't
            -- match (probably due to malformed formating or erroneous code).
            -- Just "reset" foldlist.
            foldlist = {}
          end
        end
      end
      -- #foldlist is fold level
      return #foldlist, lineidx, buf[lineidx]
    end
  end
end

--- A Lua part to be called from Vim script FoldLuaLevel function used by foldexpr option. It returns fold level for given line number.
function foldlevel_luacode()
  -- make nested folds by default though a configuration variable can disable it
  local innerfolds = vim.eval("get(b:, 'lua_inner_folds', get(g:, 'lua_inner_folds', 1))") 
  -- __p("innerfolds " .. tostring(innerfolds))
  -- Iterate over line fold levels to find that one for which Vim is asking.
  -- TODO It's repetitively inefficient - perhaps some kind of caching would
  -- be beneficial?
  local lnum = vim.eval("v:lnum")
  for lvl, lineidx in fold_iter() do
    if lineidx == lnum then
      vim.command("return " .. (innerfolds and lvl or (lvl > 1 and 1 or lvl)))
      break
    end
  end
end
EOF

" vim: nowrap sw=2 sts=2 ts=8 noet:
