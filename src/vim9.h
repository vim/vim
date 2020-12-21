/* vi:set ts=8 sts=4 sw=4 noet:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */

/*
 * vim9.h: types and globals used for Vim9 script.
 */

typedef enum {
    ISN_EXEC,	    // execute Ex command line isn_arg.string
    ISN_EXECCONCAT, // execute Ex command from isn_arg.number items on stack
    ISN_ECHO,	    // echo isn_arg.echo.echo_count items on top of stack
    ISN_EXECUTE,    // execute Ex commands isn_arg.number items on top of stack
    ISN_ECHOMSG,    // echo Ex commands isn_arg.number items on top of stack
    ISN_ECHOERR,    // echo Ex commands isn_arg.number items on top of stack
    ISN_RANGE,	    // compute range from isn_arg.string, push to stack

    // get and set variables
    ISN_LOAD,	    // push local variable isn_arg.number
    ISN_LOADV,	    // push v: variable isn_arg.number
    ISN_LOADG,	    // push g: variable isn_arg.string
    ISN_LOADAUTO,   // push g: autoload variable isn_arg.string
    ISN_LOADB,	    // push b: variable isn_arg.string
    ISN_LOADW,	    // push w: variable isn_arg.string
    ISN_LOADT,	    // push t: variable isn_arg.string
    ISN_LOADGDICT,  // push g: dict
    ISN_LOADBDICT,  // push b: dict
    ISN_LOADWDICT,  // push w: dict
    ISN_LOADTDICT,  // push t: dict
    ISN_LOADS,	    // push s: variable isn_arg.loadstore
    ISN_LOADOUTER,  // push variable from outer scope isn_arg.number
    ISN_LOADSCRIPT, // push script-local variable isn_arg.script.
    ISN_LOADOPT,    // push option isn_arg.string
    ISN_LOADENV,    // push environment variable isn_arg.string
    ISN_LOADREG,    // push register isn_arg.number

    ISN_STORE,	    // pop into local variable isn_arg.number
    ISN_STOREV,	    // pop into v: variable isn_arg.number
    ISN_STOREG,	    // pop into global variable isn_arg.string
    ISN_STOREAUTO,  // pop into global autoload variable isn_arg.string
    ISN_STOREB,	    // pop into buffer-local variable isn_arg.string
    ISN_STOREW,	    // pop into window-local variable isn_arg.string
    ISN_STORET,	    // pop into tab-local variable isn_arg.string
    ISN_STORES,	    // pop into script variable isn_arg.loadstore
    ISN_STOREOUTER,  // pop variable into outer scope isn_arg.number
    ISN_STORESCRIPT, // pop into script variable isn_arg.script
    ISN_STOREOPT,    // pop into option isn_arg.string
    ISN_STOREENV,    // pop into environment variable isn_arg.string
    ISN_STOREREG,    // pop into register isn_arg.number
    // ISN_STOREOTHER, // pop into other script variable isn_arg.other.

    ISN_STORENR,    // store number into local variable isn_arg.storenr.stnr_idx
    ISN_STOREINDEX,	// store into list or dictionary, type isn_arg.vartype,
			// value/index/variable on stack

    ISN_UNLET,		// unlet variable isn_arg.unlet.ul_name
    ISN_UNLETENV,	// unlet environment variable isn_arg.unlet.ul_name

    ISN_LOCKCONST,	// lock constant value

    // constants
    ISN_PUSHNR,		// push number isn_arg.number
    ISN_PUSHBOOL,	// push bool value isn_arg.number
    ISN_PUSHSPEC,	// push special value isn_arg.number
    ISN_PUSHF,		// push float isn_arg.fnumber
    ISN_PUSHS,		// push string isn_arg.string
    ISN_PUSHBLOB,	// push blob isn_arg.blob
    ISN_PUSHFUNC,	// push func isn_arg.string
    ISN_PUSHCHANNEL,	// push channel isn_arg.channel
    ISN_PUSHJOB,	// push channel isn_arg.job
    ISN_NEWLIST,	// push list from stack items, size is isn_arg.number
    ISN_NEWDICT,	// push dict from stack items, size is isn_arg.number

    // function call
    ISN_BCALL,	    // call builtin function isn_arg.bfunc
    ISN_DCALL,	    // call def function isn_arg.dfunc
    ISN_UCALL,	    // call user function or funcref/partial isn_arg.ufunc
    ISN_PCALL,	    // call partial, use isn_arg.pfunc
    ISN_PCALL_END,  // cleanup after ISN_PCALL with cpf_top set
    ISN_RETURN,	    // return, result is on top of stack
    ISN_FUNCREF,    // push a function ref to dfunc isn_arg.funcref
    ISN_NEWFUNC,    // create a global function from a lambda function
    ISN_DEF,	    // list functions

    // expression operations
    ISN_JUMP,	    // jump if condition is matched isn_arg.jump

    // loop
    ISN_FOR,	    // get next item from a list, uses isn_arg.forloop

    ISN_TRY,	    // add entry to ec_trystack, uses isn_arg.try
    ISN_THROW,	    // pop value of stack, store in v:exception
    ISN_PUSHEXC,    // push v:exception
    ISN_CATCH,	    // drop v:exception
    ISN_ENDTRY,	    // take entry off from ec_trystack

    // more expression operations
    ISN_ADDLIST,    // add two lists
    ISN_ADDBLOB,    // add two blobs

    // operation with two arguments; isn_arg.op.op_type is exptype_T
    ISN_OPNR,
    ISN_OPFLOAT,
    ISN_OPANY,

    // comparative operations; isn_arg.op.op_type is exptype_T, op_ic used
    ISN_COMPAREBOOL,
    ISN_COMPARESPECIAL,
    ISN_COMPARENR,
    ISN_COMPAREFLOAT,
    ISN_COMPARESTRING,
    ISN_COMPAREBLOB,
    ISN_COMPARELIST,
    ISN_COMPAREDICT,
    ISN_COMPAREFUNC,
    ISN_COMPAREANY,

    // expression operations
    ISN_CONCAT,
    ISN_STRINDEX,   // [expr] string index
    ISN_STRSLICE,   // [expr:expr] string slice
    ISN_LISTAPPEND, // append to a list, like add()
    ISN_LISTINDEX,  // [expr] list index
    ISN_LISTSLICE,  // [expr:expr] list slice
    ISN_ANYINDEX,   // [expr] runtime index
    ISN_ANYSLICE,   // [expr:expr] runtime slice
    ISN_SLICE,	    // drop isn_arg.number items from start of list
    ISN_BLOBAPPEND, // append to a blob, like add()
    ISN_GETITEM,    // push list item, isn_arg.number is the index
    ISN_MEMBER,	    // dict[member]
    ISN_STRINGMEMBER, // dict.member using isn_arg.string
    ISN_2BOOL,	    // falsy/truthy to bool, invert if isn_arg.number != 0
    ISN_COND2BOOL,  // convert value to bool
    ISN_2STRING,    // convert value to string at isn_arg.number on stack
    ISN_2STRING_ANY, // like ISN_2STRING but check type
    ISN_NEGATENR,   // apply "-" to number

    ISN_CHECKNR,    // check value can be used as a number
    ISN_CHECKTYPE,  // check value type is isn_arg.type.tc_type
    ISN_CHECKLEN,   // check list length is isn_arg.checklen.cl_min_len

    ISN_PUT,	    // ":put", uses isn_arg.put

    ISN_CMDMOD,	    // set cmdmod
    ISN_CMDMOD_REV, // undo ISN_CMDMOD

    ISN_UNPACK,	    // unpack list into items, uses isn_arg.unpack
    ISN_SHUFFLE,    // move item on stack up or down
    ISN_DROP	    // pop stack and discard value
} isntype_T;


// arguments to ISN_BCALL
typedef struct {
    int	    cbf_idx;	    // index in "global_functions"
    int	    cbf_argcount;   // number of arguments on top of stack
} cbfunc_T;

// arguments to ISN_DCALL
typedef struct {
    int	    cdf_idx;	    // index in "def_functions" for ISN_DCALL
    int	    cdf_argcount;   // number of arguments on top of stack
} cdfunc_T;

// arguments to ISN_PCALL
typedef struct {
    int	    cpf_top;	    // when TRUE partial is above the arguments
    int	    cpf_argcount;   // number of arguments on top of stack
} cpfunc_T;

// arguments to ISN_UCALL and ISN_XCALL
typedef struct {
    char_u  *cuf_name;
    int	    cuf_argcount;   // number of arguments on top of stack
} cufunc_T;

typedef enum {
    JUMP_ALWAYS,
    JUMP_IF_FALSE,		// pop and jump if false
    JUMP_AND_KEEP_IF_TRUE,	// jump if top of stack is truthy, drop if not
    JUMP_AND_KEEP_IF_FALSE,	// jump if top of stack is falsy, drop if not
    JUMP_IF_COND_TRUE,		// jump if top of stack is true, drop if not
    JUMP_IF_COND_FALSE,		// jump if top of stack is false, drop if not
} jumpwhen_T;

// arguments to ISN_JUMP
typedef struct {
    jumpwhen_T	jump_when;
    int		jump_where;	    // position to jump to
} jump_T;

// arguments to ISN_FOR
typedef struct {
    int	    for_idx;	    // loop variable index
    int	    for_end;	    // position to jump to after done
} forloop_T;

// arguments to ISN_TRY
typedef struct {
    int	    try_catch;	    // position to jump to on throw
    int	    try_finally;    // position to jump to for return
} try_T;

// arguments to ISN_ECHO
typedef struct {
    int	    echo_with_white;    // :echo instead of :echon
    int	    echo_count;		// number of expressions
} echo_T;

// arguments to ISN_OPNR, ISN_OPFLOAT, etc.
typedef struct {
    exptype_T	op_type;
    int		op_ic;	    // TRUE with '#', FALSE with '?', else MAYBE
} opexpr_T;

// arguments to ISN_CHECKTYPE
typedef struct {
    type_T	*ct_type;
    int		ct_off;	    // offset in stack, -1 is bottom
} checktype_T;

// arguments to ISN_STORENR
typedef struct {
    int		stnr_idx;
    varnumber_T	stnr_val;
} storenr_T;

// arguments to ISN_STOREOPT
typedef struct {
    char_u	*so_name;
    int		so_flags;
} storeopt_T;

// arguments to ISN_LOADS and ISN_STORES
typedef struct {
    char_u	*ls_name;	// variable name (with s: for ISN_STORES)
    int		ls_sid;		// script ID
} loadstore_T;

// arguments to ISN_LOADSCRIPT and ISN_STORESCRIPT
typedef struct {
    int		script_sid;	// script ID
    int		script_idx;	// index in sn_var_vals
} script_T;

// arguments to ISN_UNLET
typedef struct {
    char_u	*ul_name;	// variable name with g:, w:, etc.
    int		ul_forceit;	// forceit flag
} unlet_T;

// arguments to ISN_FUNCREF
typedef struct {
    int		fr_func;	// function index
} funcref_T;

// arguments to ISN_NEWFUNC
typedef struct {
    char_u	*nf_lambda;	// name of the lambda already defined
    char_u	*nf_global;	// name of the global function to be created
} newfunc_T;

// arguments to ISN_CHECKLEN
typedef struct {
    int		cl_min_len;	// minimum length
    int		cl_more_OK;	// longer is allowed
} checklen_T;

// arguments to ISN_SHUFFLE
typedef struct {
    int		shfl_item;	// item to move (relative to top of stack)
    int		shfl_up;	// places to move upwards
} shuffle_T;

// arguments to ISN_PUT
typedef struct {
    int		put_regname;	// register, can be NUL
    linenr_T	put_lnum;	// line number to put below
} put_T;

// arguments to ISN_CMDMOD
typedef struct {
    cmdmod_T	*cf_cmdmod;	// allocated
} cmod_T;

// arguments to ISN_UNPACK
typedef struct {
    int		unp_count;	// number of items to produce
    int		unp_semicolon;	// last item gets list of remainder
} unpack_T;

/*
 * Instruction
 */
struct isn_S {
    isntype_T	isn_type;
    int		isn_lnum;
    union {
	char_u		    *string;
	varnumber_T	    number;
	blob_T		    *blob;
	vartype_T	    vartype;
#ifdef FEAT_FLOAT
	float_T		    fnumber;
#endif
	channel_T	    *channel;
	job_T		    *job;
	partial_T	    *partial;
	jump_T		    jump;
	forloop_T	    forloop;
	try_T		    try;
	cbfunc_T	    bfunc;
	cdfunc_T	    dfunc;
	cpfunc_T	    pfunc;
	cufunc_T	    ufunc;
	echo_T		    echo;
	opexpr_T	    op;
	checktype_T	    type;
	storenr_T	    storenr;
	storeopt_T	    storeopt;
	loadstore_T	    loadstore;
	script_T	    script;
	unlet_T		    unlet;
	funcref_T	    funcref;
	newfunc_T	    newfunc;
	checklen_T	    checklen;
	shuffle_T	    shuffle;
	put_T		    put;
	cmod_T		    cmdmod;
	unpack_T	    unpack;
    } isn_arg;
};

/*
 * Info about a function defined with :def.  Used in "def_functions".
 */
struct dfunc_S {
    ufunc_T	*df_ufunc;	    // struct containing most stuff
    int		df_idx;		    // index in def_functions
    int		df_deleted;	    // if TRUE function was deleted

    garray_T	df_def_args_isn;    // default argument instructions
    isn_T	*df_instr;	    // function body to be executed
    int		df_instr_count;

    int		df_varcount;	    // number of local variables
    int		df_has_closure;	    // one if a closure was created
};

// Number of entries used by stack frame for a function call.
// - ec_dfunc_idx:   function index
// - ec_iidx:        instruction index
// - ec_outer_stack: stack used for closures  TODO: can we avoid this?
// - ec_outer_frame: stack frame for closures
// - ec_frame_idx:   previous frame index
#define STACK_FRAME_SIZE 5


#ifdef DEFINE_VIM9_GLOBALS
// Functions defined with :def are stored in this growarray.
// They are never removed, so that they can be found by index.
// Deleted functions have the df_deleted flag set.
garray_T def_functions = {0, 0, sizeof(dfunc_T), 50, NULL};
#else
extern garray_T def_functions;
#endif

// Used for "lnum" when a range is to be taken from the stack.
#define LNUM_VARIABLE_RANGE -999

// Used for "lnum" when a range is to be taken from the stack and "!" is used.
#define LNUM_VARIABLE_RANGE_ABOVE -888
