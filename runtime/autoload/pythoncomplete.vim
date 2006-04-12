"pythoncomplete.vim - Omni Completion for python
" Maintainer: Aaron Griffin
" Version: 0.3
" Last Updated: 23 January 2006
"
"   v0.3 Changes:
"       added top level def parsing
"       for safety, call returns are not evaluated
"       handful of parsing changes
"       trailing ( and . characters
"       argument completion on open parens
"       stop parsing at current line - ++performance, local var resolution
"
"   TODO
"       RExec subclass
"       Code cleanup + make class
"       use internal dict, not globals()

if !has('python')
    echo "Error: Required vim compiled with +python"
    finish
endif

function! pythoncomplete#Complete(findstart, base)
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
        return g:pythoncomplete_completions
    endif
endfunction

function! s:DefPython()
python << PYTHONEOF
import vim, sys, types
import __builtin__
import tokenize, keyword, cStringIO

LOCALDEFS = \
	['LOCALDEFS', 'clean_up','eval_source_code', \
	 'get_completions', '__builtin__', '__builtins__', \
	 'dbg', '__name__', 'vim', 'sys', 'parse_to_end', \
     'parse_statement', 'tokenize', 'keyword', 'cStringIO', \
     'debug_level', 'safe_eval', '_ctor', 'get_arguments', \
     'strip_calls', 'types', 'parse_block']

def dbg(level,msg):
    debug_level = 1
    try:
        debug_level = vim.eval("g:pythoncomplete_debug_level")
    except:
        pass
    if level <= debug_level: print(msg)

def strip_calls(stmt):
    parsed=''
    level = 0
    for c in stmt:
        if c in ['[','(']:
            level += 1
        elif c in [')',']']:
            level -= 1
        elif level == 0:
            parsed += c
    ##dbg(10,"stripped: %s" % parsed)
    return parsed

def get_completions(base):
    stmt = vim.eval('expand("<cWORD>")')
    #dbg(1,"statement: %s - %s" % (stmt, base))
    stmt = stmt+base
    eval_source_code()

    try:
        ridx = stmt.rfind('.')
        if stmt[-1] == '(':
            match = ""
            stmt = strip_calls(stmt[:len(stmt)-1])
            all = get_arguments(eval(stmt))
        elif ridx == -1:
            match = stmt
            all = globals() + __builtin__.__dict__
        else:
            match = stmt[ridx+1:]
            stmt = strip_calls(stmt[:ridx])
            all = eval(stmt).__dict__

        #dbg(15,"completions for: %s, match=%s" % (stmt,match))
        completions = []
        if type(all) == types.DictType:
            for m in all:
                if m.find('_') != 0 and m.find(match) == 0 and \
			       m not in LOCALDEFS:
                    #dbg(25,"matched... %s, %s" % (m, m.find(match)))
                    typestr = str(all[m])
                    if "function" in typestr: m += '('
                    elif "method" in typestr: m += '('
                    elif "module" in typestr: m += '.'
                    elif "class" in typestr: m += '('
                    completions.append(m)
            completions.sort()
        else:
            completions.append(all)
        #dbg(10,"all completions: %s" % completions)
        vim.command("let g:pythoncomplete_completions = %s" % completions)
    except:
        vim.command("let g:pythoncomplete_completions = []")
        #dbg(1,"exception: %s" % sys.exc_info()[1])
    clean_up()

def get_arguments(func_obj):
    def _ctor(obj):
        try:
            return class_ob.__init__.im_func
        except AttributeError:
            for base in class_ob.__bases__:
                rc = _find_constructor(base)
                if rc is not None: return rc
        return None

    arg_offset = 1
    if type(func_obj) == types.ClassType: func_obj = _ctor(func_obj)
    elif type(func_obj) == types.MethodType: func_obj = func_obj.im_func
    else: arg_offset = 0
    
    #dbg(20,"%s, offset=%s" % (str(func_obj), arg_offset))

    arg_text = ''
    if type(func_obj) in [types.FunctionType, types.LambdaType]:
        try:
            cd = func_obj.func_code
            real_args = cd.co_varnames[arg_offset:cd.co_argcount]
            defaults = func_obj.func_defaults or []
            defaults = list(map(lambda name: "=%s" % name, defaults))
            defaults = [""] * (len(real_args)-len(defaults)) + defaults
            items = map(lambda a,d: a+d, real_args, defaults)
            if func_obj.func_code.co_flags & 0x4:
                items.append("...")
            if func_obj.func_code.co_flags & 0x8:
                items.append("***")
            arg_text = ", ".join(items) + ')'

        except:
            #dbg(1,"exception: %s" % sys.exc_info()[1])
            pass
    if len(arg_text) == 0:
        # The doc string sometimes contains the function signature
        #  this works for alot of C modules that are part of the
        #  standard library
        doc = getattr(func_obj, '__doc__', '')
        if doc:
            doc = doc.lstrip()
            pos = doc.find('\n')
            if pos > 0:
                sigline = doc[:pos]
                lidx = sigline.find('(')
                ridx = sigline.find(')')
                retidx = sigline.find('->')
                ret = sigline[retidx+2:].strip()
                if lidx > 0 and ridx > 0:
                    arg_text = sigline[lidx+1:ridx] + ')'
                    if len(ret) > 0: arg_text += ' #returns %s' % ret
    #dbg(15,"argument completion: %s" % arg_text)
    return arg_text

def parse_to_end(gen):
    stmt=''
    level = 0
    for type, str, begin, end, line in gen:
        if line == vim.eval('getline(\'.\')'): break
        elif str == '\\': continue
        elif str == ';':
            break
        elif type == tokenize.NEWLINE and level == 0:
            break
        elif str in ['[','(']:
            level += 1
        elif str in [')',']']:
            level -= 1
        elif level == 0:
            stmt += str
        #dbg(10,"current statement: %s" % stmt)
    return stmt

def parse_block(gen):
    lines = []
    level = 0
    for type, str, begin, end, line in gen:
        if line.replace('\n','') == vim.eval('getline(\'.\')'): break
        elif type == tokenize.INDENT:
            level += 1
        elif type == tokenize.DEDENT:
            level -= 1
            if level == 0: break;
        else:
            stmt = parse_statement(gen,str)
            if len(stmt) > 0: lines.append(stmt)
    return lines

def parse_statement(gen,curstr=''):
    var = curstr
    type, str, begin, end, line = gen.next()
    if str == '=':
        type, str, begin, end, line = gen.next()
        if type == tokenize.NEWLINE:
            return ''
        elif type == tokenize.STRING or str == 'str':  
            return '%s = str' % var
        elif str == '[' or str == 'list':
            return '%s= list' % var
        elif str == '{' or str == 'dict':
            return '%s = dict' % var
        elif type == tokenize.NUMBER:
            return '%s = 0' % var
        elif str == 'Set': 
            return '%s = Set' % var
        elif str == 'open' or str == 'file':
            return '%s = file' % var
        else:
            inst = str + parse_to_end(gen)
            if len(inst) > 0:
                #dbg(5,"found [%s = %s]" % (var, inst))
                return '%s = %s' % (var, inst)
    return ''

def eval_source_code():
    LINE=vim.eval('getline(\'.\')')
    s = cStringIO.StringIO('\n'.join(vim.current.buffer[:]) + '\n')
    g = tokenize.generate_tokens(s.readline)

    stmts = []
    lineNo = 0
    try:
        for type, str, begin, end, line in g:
            if line.replace('\n','') == vim.eval('getline(\'.\')'): break
            elif begin[0] == lineNo: continue
            #junk
            elif type == tokenize.INDENT or \
                 type == tokenize.DEDENT or \
                 type == tokenize.ERRORTOKEN or \
                 type == tokenize.ENDMARKER or \
                 type == tokenize.NEWLINE or \
                 type == tokenize.COMMENT:
                continue
            #import statement
            elif str == 'import':
                import_stmt=parse_to_end(g)
                if len(import_stmt) > 0:
                    #dbg(5,"found [import %s]" % import_stmt)
                    stmts.append("import %s" % import_stmt)
            #import from statement
            elif str == 'from':
                type, str, begin, end, line = g.next()
                mod = str

                type, str, begin, end, line = g.next()
                if str != "import": break
                from_stmt=parse_to_end(g)
                if len(from_stmt) > 0:
                    #dbg(5,"found [from %s import %s]" % (mod, from_stmt))
                    stmts.append("from %s import %s" % (mod, from_stmt))
            #def statement
            elif str == 'def':
                funcstr = ''
                for type, str, begin, end, line in g:
                    if line.replace('\n','') == vim.eval('getline(\'.\')'): break
                    elif str == ':':
                        stmts += parse_block(g)
                        break
                    funcstr += str
                if len(funcstr) > 0:
                    #dbg(5,"found [def %s]" % funcstr)
                    stmts.append("def %s:\n   pass" % funcstr)
            #class declaration
            elif str == 'class':
                type, str, begin, end, line = g.next()
                classname = str
                #dbg(5,"found [class %s]" % classname)

                level = 0
                members = []
                for type, str, begin, end, line in g:
                    if line.replace('\n','') == vim.eval('getline(\'.\')'): break
                    elif type == tokenize.INDENT:
                        level += 1
                    elif type == tokenize.DEDENT:
                        level -= 1
                        if level == 0: break;
                    elif str == 'def':
                        memberstr = ''
                        for type, str, begin, end, line in g:
                            if line.replace('\n','') == vim.eval('getline(\'.\')'): break
                            elif str == ':':
                                stmts += parse_block(g)
                                break
                            memberstr += str
                        #dbg(5,"   member [%s]" % memberstr)
                        members.append(memberstr)
                classstr = 'class %s:' % classname
                for m in members:
                    classstr += ("\n   def %s:\n      pass" % m)
                stmts.append("%s\n" % classstr)
            elif keyword.iskeyword(str) or str in globals():
                #dbg(5,"keyword = %s" % str)
                lineNo = begin[0]
            else:
                assign = parse_statement(g,str)
                if len(assign) > 0: stmts.append(assign)
                
        for s in stmts:
            try:
                #dbg(15,"evaluating: %s\n" % s)
                exec(s) in globals()
            except:
                #dbg(1,"exception: %s" % sys.exc_info()[1])
                pass
    except:
        #dbg(1,"exception: %s" % sys.exc_info()[1])
        pass

def clean_up():
    for o in globals().keys():
        if o not in LOCALDEFS:
            try:
                exec('del %s' % o) in globals()
            except: pass

sys.path.extend(['.','..'])
PYTHONEOF
endfunction

let g:pythoncomplete_debug_level = 0
call s:DefPython()
" vim: set et ts=4:
