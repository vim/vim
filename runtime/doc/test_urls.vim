vim9script

# Test URLs in help documents.
#
# Opens a new window with all found URLs followed by return code from curl
# (anything other than 0 means unreachable)
#
# Vim9 script version (based on legacy Vim script by Christian Brabandt)

var command1: string
var command2: string

def Test_check_URLs(): void
  var outdev: string = has("win32") ? 'nul' : '/dev/null'
  # Restorer:  For Windows users.  If 'curl' or 'wget' is installed on the
  # system but not in %PATH%, add the full path to them to %PATH% environment
  # variable.
  if executable('curl')
    # Note: does not follow redirects!
    command1 = $"curl --silent --max-time 5 --fail --output {outdev} --head "
  elseif executable('wget')
    # Note: only allow a couple of redirects
    command1 = "wget --quiet -S --spider --max-redirect=2 --timeout=5" ..
      $" --tries=2 -O {outdev} "
  elseif has("win32")
    if executable('powershell')
      command1 = "powershell -nologo -noprofile " ..
        '"{[Net.ServicePointManager]::SecurityProtocol = ' ..
        "'Tls12, Tls11, Tls, Ssl3'};try{(Invoke-WebRequest " ..
        "-MaximumRedirection 2 -TimeoutSec 5 -Uri "
      command2 = ').StatusCode}catch{exit [int]$Error[0].Exception.Status}"'
    endif
  else
    echoerr 'Only works when "curl" or "wget", or "powershell" is available'
    return
  endif

  # do the testing
  set report=999
  set nomore shm+=s

  var pat: string = '\v(https?|ftp)://\w[^ \t*`"<>]*'
  execute 'helpgrep' pat
  helpclose

  var urls: list<string> = map(getqflist(), 'v:val.text')
  # do not use submatch(1)!
  urls = urls->map((_, val) => matchstr(val, pat))
  # remove examples like user@host (invalid urls)
  urls = urls->filter((_, val) => val !~ "@")
  # Remove example URLs which are invalid
  urls = urls->filter((_, val) => val !~
    '\<\(\(my\|some\)\?host\|machine\|hostname\|file\)\>')
  new
  put =urls
  # remove some more invalid items
  # empty lines
  :vglobal/./delete _
  # remove # anchors
  :%substitute/#.*$//e
  # remove trailing dot, comma, quotes, etc. (only for HTTP links)
  :global/^h/substitute;[.),'"`/>][:.,]\?$;;
  silent! global/ftp://,$/delete _
  silent! global/=$/delete _
  var a: list<string> = getline(1, '$')
  a = a->sort()->uniq()
  :%delete _
  setline(1, a)

  :%substitute_\v^(\a.+$)\n\1/$_\1_e

  :%substitute/.*/\=TestURL(submatch(0))/

  # syntax highlight the failures
  syntax match Error '\v.* ([0-9]*[1-9]|[0-9]{2,})$'
enddef

def TestURL(url: string): string
  # relies on the return code to determine whether a page is valid
  echomsg printf("Testing URL: %d/%d %s", line('.'), line('$'), url)
  system($"{command1}{shellescape(url)}{command2}")
  return printf("%s %d", url, v:shell_error)
enddef

const SHELL_SETTINGS: list<string> = [&shell, &shellcmdflag, &shellxquote,
  &shellxescape, &shellquote, &shellredir]
if has('win32')
  &shell = 'cmd.exe'
  &shellcmdflag = '/s /c'
  &shellxquote = '('
  &shellxescape = '"&|<>()@^'
  &shellquote = ''
  &shellredir = '>%s 2>&1'
endif

try
  Test_check_URLs()
finally
  [&shell, &shellcmdflag, &shellxquote, &shellxescape, &shellquote,
        \ &shellredir] = SHELL_SETTINGS
endtry

# vim: sw=2 sts=2 tw=78 et
