/* vi:set ts=8 sts=4 sw=4 noet:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */

/*
 * vim9class.c: Vim9 script class support
 */

#define USING_FLOAT_STUFF
#include "vim.h"

#if defined(FEAT_EVAL) || defined(PROTO)

// When not generating protos this is included in proto.h
#ifdef PROTO
# include "vim9.h"
#endif

/*
 * Handle ":class" and ":abstract class" up to ":endclass".
 */
    void
ex_class(exarg_T *eap)
{
    int is_abstract = eap->cmdidx == CMD_abstract;

    char_u *arg = eap->arg;
    if (is_abstract)
    {
	if (STRNCMP(arg, "class", 5) != 0 || !VIM_ISWHITE(arg[5]))
	{
	    semsg(_(e_invalid_argument_str), arg);
	    return;
	}
	arg = skipwhite(arg + 5);
    }

    if (!ASCII_ISUPPER(*arg))
    {
	semsg(_(e_class_name_must_start_with_uppercase_letter_str), arg);
	return;
    }

    // TODO:
    // generics: <Tkey, Tentry>
    //    extends SomeClass
    //    implements SomeInterface
    //    specifies SomeInterface


    // TODO: handle until "endclass" is found:
    // object and class members (public, read access, private):
    //	  public this.varname
    //	  public static varname
    //	  this.varname
    //	  static varname
    //	  this._varname
    //	  static _varname
    //
    // constructors:
    //	  def new()
    //	  enddef
    //	  def newOther()
    //	  enddef
    //
    // methods (object, class, generics):
    //	  def someMethod()
    //	  enddef
    //	  static def someMethod()
    //	  enddef
    //	  def <Tval> someMethod()
    //	  enddef
    //	  static def <Tval> someMethod()
    //	  enddef
}

/*
 * Handle ":interface" up to ":endinterface".
 */
    void
ex_interface(exarg_T *eap UNUSED)
{
    // TODO
}

/*
 * Handle ":enum" up to ":endenum".
 */
    void
ex_enum(exarg_T *eap UNUSED)
{
    // TODO
}

/*
 * Handle ":type".
 */
    void
ex_type(exarg_T *eap UNUSED)
{
    // TODO
}


#endif // FEAT_EVAL
