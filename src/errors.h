/* vi:set ts=8 sts=4 sw=4 noet:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 */

/*
 * Definition of error messages, sorted on error number.
 */

#ifdef FEAT_EVAL
EXTERN char e_white_space_required_before_and_after[]
	INIT(= N_("E1004: white space required before and after '%s'"));
EXTERN char e_cannot_declare_a_scope_variable[]
	INIT(= N_("E1016: Cannot declare a %s variable: %s"));
EXTERN char e_cannot_declare_an_environment_variable[]
	INIT(= N_("E1016: Cannot declare an environment variable: %s"));
EXTERN char e_const_requires_a_value[]
	INIT(= N_("E1021: const requires a value"));
EXTERN char e_type_or_initialization_required[]
	INIT(= N_("E1022: type or initialization required"));
EXTERN char e_colon_required_before_a_range[]
	INIT(= N_("E1050: Colon required before a range"));
EXTERN char e_no_white_space_allowed_before[]
	INIT(= N_("E1068: No white space allowed before '%s'"));
EXTERN char e_white_space_required_after[]
	INIT(= N_("E1069: white space required after '%s'"));
EXTERN char e_name_already_defined[]
	INIT(= N_("E1073: name already defined: %s"));
EXTERN char e_list_dict_or_blob_required[]
	INIT(= N_("E1090: List, Dict or Blob required"));
EXTERN char e_dictionary_not_set[]
	INIT(= N_("E1103: Dictionary not set"));
#endif
