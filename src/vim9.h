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
    ISN_ECHO,	    // echo isn_arg.number items on top of stack

    // get and set variables
    ISN_LOAD,	    // push local variable isn_arg.number
    ISN_LOADV,	    // push v: variable isn_arg.number
    ISN_LOADSCRIPT, // push script-local variable isn_arg.script.
    ISN_LOADS,	    // push s: variable isn_arg.string
    ISN_LOADG,	    // push g: variable isn_arg.string
    ISN_LOADOPT,    // push option isn_arg.string
    ISN_LOADENV,    // push environment variable isn_arg.string
    ISN_LOADREG,    // push register isn_arg.number

    ISN_STORE,	    // pop into local variable isn_arg.number
    ISN_STOREG,	    // pop into global variable isn_arg.string
    ISN_STORESCRIPT, // pop into scirpt variable isn_arg.script
    ISN_STOREOPT,   // pop into option isn_arg.string
    // ISN_STOREOTHER, // pop into other script variable isn_arg.other.

    ISN_STORENR,    // store number into local variable isn_arg.storenr.str_idx

    // constants
    ISN_PUSHNR,	    // push number isn_arg.number
    ISN_PUSHBOOL,   // push bool value isn_arg.number
    ISN_PUSHSPEC,   // push special value isn_arg.number
    ISN_PUSHF,	    // push float isn_arg.fnumber
    ISN_PUSHS,	    // push string isn_arg.string
    ISN_PUSHBLOB,   // push blob isn_arg.blob
    ISN_NEWLIST,    // push list from stack items, size is isn_arg.number
    ISN_NEWDICT,    // push dict from stack items, size is isn_arg.number

    // function call
    ISN_BCALL,	    // call builtin function isn_arg.bfunc
    ISN_DCALL,	    // call def function isn_arg.dfunc
    ISN_UCALL,	    // call user function or funcref/partial isn_arg.ufunc
    ISN_PCALL,	    // call partial, use isn_arg.pfunc
    ISN_RETURN,	    // return, result is on top of stack
    ISN_FUNCREF,    // push a function ref to dfunc isn_arg.number

    // expression operations
    ISN_JUMP,	    // jump if condition is matched isn_arg.jump

    // loop
    ISN_FOR,	    // get next item from a list, uses isn_arg.forloop

    ISN_TRY,	    // add entry to ec_trystack, uses isn_arg.try
    ISN_THROW,	    // pop value of stack, store in v:exception
    ISN_PUSHEXC,    // push v:exception
    ISN_CATCH,	    // drop v:exception
    ISN_ENDTRY,	    // take entry off from ec_trystack

    // moreexpression operations
    ISN_ADDLIST,
    ISN_ADDBLOB,

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
    ISN_COMPAREPARTIAL,
    ISN_COMPAREANY,

    // expression operations
    ISN_CONCAT,
    ISN_INDEX,	    // [expr] list index
    ISN_MEMBER,	    // dict.member using isn_arg.string
    ISN_2BOOL,	    // convert value to bool, invert if isn_arg.number != 0
    ISN_2STRING,    // convert value to string at isn_arg.number on stack
    ISN_NEGATENR,   // apply "-" to number

    ISN_CHECKNR,    // check value can be used as a number
    ISN_CHECKTYPE,  // check value type is isn_arg.type.tc_type

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
    JUMP_IF_TRUE,		// pop and jump if true
    JUMP_IF_FALSE,		// pop and jump if false
    JUMP_AND_KEEP_IF_TRUE,	// jump if top of stack is true, drop if not
    JUMP_AND_KEEP_IF_FALSE,	// jump if top of stack is false, drop if not
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
    vartype_T	ct_type;
    int		ct_off;	    // offset in stack, -1 is bottom
} checktype_T;

// arguments to ISN_STORENR
typedef struct {
    int		str_idx;
    varnumber_T	str_val;
} storenr_T;

// arguments to ISN_STOREOPT
typedef struct {
    char_u	*so_name;
    int		so_flags;
} storeopt_T;

// arguments to ISN_LOADS
typedef struct {
    char_u	*ls_name;	// variable name
    int		ls_sid;		// script ID
} loads_T;

// arguments to ISN_LOADSCRIPT
typedef struct {
    int		script_sid;	// script ID
    int		script_idx;	// index in sn_var_vals
} script_T;

/*
 * Instruction
 */
typedef struct {
    isntype_T	isn_type;
    int		isn_lnum;
    union {
	char_u		    *string;
	varnumber_T	    number;
	blob_T		    *blob;
#ifdef FEAT_FLOAT
	float_T		    fnumber;
#endif
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
	loads_T		    loads;
	script_T	    script;
    } isn_arg;
} isn_T;

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
};

// Number of entries used by stack frame for a function call.
#define STACK_FRAME_SIZE 3


#ifdef DEFINE_VIM9_GLOBALS
// Functions defined with :def are stored in this growarray.
// They are never removed, so that they can be found by index.
// Deleted functions have the df_deleted flag set.
garray_T def_functions = {0, 0, sizeof(dfunc_T), 50, NULL};
#else
extern garray_T def_functions;
#endif

