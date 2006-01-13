"pycomplete.vim - Omni Completion for python
" Maintainer: Aaron Griffin
" Version: 0.2
" Last Updated: 5 January 2006
"
"   TODO
"   * local variables *inside* class members

if !has('python')
    echo "Error: Required vim compiled with +python"
    finish
endif

function! pycomplete#Complete(findstart, base)
    "findstart = 1 when we need to get the text length
    if a:findstart
        let line = getline('.')
        let idx = col('.')
        while idx > 0
            let idx -= 1
            let c = line[idx-1]
            if c =~ '\w'
				continue
			elseif ! c =~ '\.'
                idx = -1
				break
            else
                break
            endif
        endwhile

        return idx
    "findstart = 0 when we need to return the list of completions
    else
        execute "python get_completions('" . a:base . "')"
        return g:pycomplete_completions
    endif
endfunction

function! s:DefPython()
python << PYTHONEOF
import vim
import sys
import __builtin__

LOCALDEFS = \
	['LOCALDEFS', 'clean_up','eval_source_code', \
	 'get_completions', '__builtin__', '__builtins__', \
	 'dbg', '__name__', 'vim', 'sys']
#comment/uncomment one line at a time to enable/disable debugging
def dbg(msg):
    pass
#    print(msg)

#it seems that by this point, vim has already stripped the base
#  matched in the findstart=1 section, so we will create the
#  statement from scratch
def get_completions(base):
    stmt = vim.eval('expand("<cWORD>")')+base
    dbg("parsed statement => %s" % stmt)
    eval_source_code()
    try:
        dbg("eval: %s" % stmt)
        if len(stmt.split('.')) == 1:
            all = globals().keys() + dir(__builtin__)
            match = stmt
        else:
            rindex= stmt.rfind('.')
            all = dir(eval(stmt[:rindex]))
            match = stmt[rindex+1:]

        completions = []
        dbg("match == %s" % match)
        for m in all:
            #TODO: remove private (_foo) functions?
            if m.find('__') != 0 and \
               m.find(match) == 0 and \
			   m not in LOCALDEFS:
                dbg("matched... %s, %s" % (m, m.find(match)))
                completions.append(m)
        dbg("all completions: %s" % completions)
        vim.command("let g:pycomplete_completions = %s" % completions)
    except:
        dbg("exception: %s" % sys.exc_info()[1])
        vim.command("let g:pycomplete_completions = []")
    clean_up()

#yes, this is a quasi-functional python lexer
def eval_source_code():
    import tokenize
    import keyword
    import StringIO
    s = StringIO.StringIO('\n'.join(vim.current.buffer[:]) + '\n')
    g = tokenize.generate_tokens(s.readline)

    stmts = []
    lineNo = 0
    try:
        for type, str, begin, end, line in g:
            if begin[0] == lineNo:
                continue
            #junk
            elif type == tokenize.INDENT or \
                 type == tokenize.DEDENT or \
                 type == tokenize.ERRORTOKEN or \
                 type == tokenize.ENDMARKER or \
                 type == tokenize.NEWLINE:
                continue
            #import statement
            elif str == 'import':
                for type, str, begin, end, line in g:
                    if str == ';' or type == tokenize.NEWLINE: break
                    dbg("found [import %s]" % str)
                    stmts.append("import %s" % str)
            #import from statement
            elif str == 'from':
                type, str, begin, end, line = g.next()
                mod = str

                type, str, begin, end, line = g.next()
                if str != "import": break
                mem = ''
                for type, str, begin, end, line in g:
                    if str == ';' or type == tokenize.NEWLINE: break
                    mem += (str + ',')
                if len(mem) > 0:
                    dbg("found [from %s import %s]" % (mod, mem[:-1]))
                    stmts.append("from %s import %s" % (mod, mem[:-1]))
            #class declaration
            elif str == 'class':
                type, str, begin, end, line = g.next()
                classname = str
                dbg("found [class %s]" % classname)

                level = 0
                members = []
                #we don't care about the meat of the members,
                # only the signatures, so we'll replace the bodies
                # with 'pass' for evaluation
                for type, str, begin, end, line in g:
                    if type == tokenize.INDENT:
                        level += 1
                    elif type == tokenize.DEDENT:
                        level -= 1
                        if level == 0: break;
                    elif str == 'def':
                        #TODO: if name begins with '_', keep private
                        memberstr = ''
                        for type, str, begin, end, line in g:
                            if str == ':': break
                            memberstr += str
                        dbg("   member [%s]" % memberstr)
                        members.append(memberstr)
                    #TODO parse self.blah = something lines
                    #elif str == "self" && next && str == "." ...blah...
                classstr = 'class %s:' % classname
                for m in members:
                    classstr += ("\n   def %s:\n      pass" % m)
                stmts.append("%s\n" % classstr)
            elif keyword.iskeyword(str) or str in globals():
                dbg("keyword = %s" % str)
                lineNo = begin[0]
            else:
                if line.find("=") == -1: continue
                var = str
                type, str, begin, end, line = g.next()
                dbg('next = %s' % str)
                if str != '=': continue

                type, str, begin, end, line = g.next()
                if type == tokenize.NEWLINE:
                    continue
                elif type == tokenize.STRING or str == 'str':  
                    stmts.append('%s = str' % var)
                elif str == '[' or str == 'list':
                    stmts.append('%s= list' % var)
                elif str == '{' or str == 'dict':
                    stmts.append('%s = dict' % var)
                elif type == tokenize.NUMBER:
                    continue
                elif str == 'Set': 
                    stmts.append('%s = Set' % var)
                elif str == 'open' or str == 'file':
                    stmts.append('%s = file' % var)
                else:
                    inst = str
                    for type, str, begin, end, line in g:
                        if type == tokenize.NEWLINE:
                            break
                        inst += str
                    if len(inst) > 0:
                        dbg("found [%s = %s]" % (var, inst))
                        stmts.append('%s = %s' % (var, inst))
                lineNo = begin[0]
        for s in stmts:
            try:
                dbg("evaluating: %s\n" % s)
                exec(s) in globals()
            except:
                pass
    except:
        dbg("exception: %s" % sys.exc_info()[1])

def clean_up():
    for o in globals().keys():
        if o not in LOCALDEFS:
            try:
                exec('del %s' % o) in globals()
            except: pass

sys.path.extend(['.','..'])
PYTHONEOF
endfunction

call s:DefPython()
" vim: set et ts=4:
