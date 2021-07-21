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

EXTERN char e_backslash_should_be_followed_by[]
	INIT(= N_("E10: \\ should be followed by /, ? or &"));
#ifdef FEAT_CMDWIN
EXTERN char e_invalid_in_cmdline_window[]
	INIT(= N_("E11: Invalid in command-line window; <CR> executes, CTRL-C quits"));
#endif
EXTERN char e_command_not_allowed_from_vimrc_in_current_dir_or_tag_search[]
	INIT(= N_("E12: Command not allowed from exrc/vimrc in current dir or tag search"));
EXTERN char e_file_exists[]
	INIT(= N_("E13: File exists (add ! to override)"));
#ifdef FEAT_EVAL
EXTERN char e_invalid_expression_str[]
	INIT(= N_("E15: Invalid expression: \"%s\""));
#endif
EXTERN char e_invalid_range[]
	INIT(= N_("E16: Invalid range"));
#if defined(UNIX) || defined(FEAT_SYN_HL) || defined(FEAT_SPELL)
EXTERN char e_src_is_directory[]
	INIT(= N_("E17: \"%s\" is a directory"));
#endif
#ifdef FEAT_EVAL
EXTERN char e_unexpected_characters_in_let[]
	INIT(= N_("E18: Unexpected characters in :let"));
EXTERN char e_unexpected_characters_in_assignment[]
	INIT(= N_("E18: Unexpected characters in assignment"));
#endif
EXTERN char e_mark_has_invalid_line_number[]
	INIT(= N_("E19: Mark has invalid line number"));
EXTERN char e_mark_not_set[]
	INIT(= N_("E20: Mark not set"));
EXTERN char e_cannot_make_changes_modifiable_is_off[]
	INIT(= N_("E21: Cannot make changes, 'modifiable' is off"));
EXTERN char e_scripts_nested_too_deep[]
	INIT(= N_("E22: Scripts nested too deep"));
EXTERN char e_no_alternate_file[]
	INIT(= N_("E23: No alternate file"));
EXTERN char e_no_such_abbreviation[]
	INIT(= N_("E24: No such abbreviation"));
#if !defined(FEAT_GUI) || defined(VIMDLL)
EXTERN char e_gui_cannot_be_used_not_enabled_at_compile_time[]
	INIT(= N_("E25: GUI cannot be used: Not enabled at compile time"));
#endif
#ifndef FEAT_RIGHTLEFT
EXTERN char e_hebrew_cannot_be_used_not_enabled_at_compile_time[]
	INIT(= N_("E26: Hebrew cannot be used: Not enabled at compile time\n"));
#endif
EXTERN char e_farsi_support_has_been_removed[]
	INIT(= N_("E27: Farsi support has been removed\n"));
#if defined(FEAT_SEARCH_EXTRA) || defined(FEAT_SYN_HL)
EXTERN char e_no_such_highlight_group_name_str[]
	INIT(= N_("E28: No such highlight group name: %s"));
#endif
EXTERN char e_no_inserted_text_yet[]
	INIT(= N_("E29: No inserted text yet"));
EXTERN char e_no_previous_command_line[]
	INIT(= N_("E30: No previous command line"));
EXTERN char e_no_such_mapping[]
	INIT(= N_("E31: No such mapping"));
EXTERN char e_no_file_name[]
	INIT(= N_("E32: No file name"));
EXTERN char e_no_previous_substitute_regular_expression[]
	INIT(= N_("E33: No previous substitute regular expression"));
EXTERN char e_no_previous_command[]
	INIT(= N_("E34: No previous command"));
EXTERN char e_no_previous_regular_expression[]
	INIT(= N_("E35: No previous regular expression"));
EXTERN char e_not_enough_room[]
	INIT(= N_("E36: Not enough room"));
EXTERN char e_no_write_since_last_change[]
	INIT(= N_("E37: No write since last change"));
EXTERN char e_no_write_since_last_change_add_bang_to_override[]
	INIT(= N_("E37: No write since last change (add ! to override)"));
EXTERN char e_null_argument[]
	INIT(= N_("E38: Null argument"));
#if defined(FEAT_DIGRAPHS) || defined(FEAT_TIMERS) || defined(FEAT_EVAL)
EXTERN char e_number_expected[]
	INIT(= N_("E39: Number expected"));
#endif
#ifdef FEAT_QUICKFIX
EXTERN char e_cant_open_errorfile_str[]
	INIT(= N_("E40: Can't open errorfile %s"));
#endif
EXTERN char e_out_of_memory[]
	INIT(= N_("E41: Out of memory!"));
#ifdef FEAT_QUICKFIX
EXTERN char e_no_errors[]
	INIT(= N_("E42: No Errors"));
#endif
EXTERN char e_damaged_match_string[]
	INIT(= N_("E43: Damaged match string"));
EXTERN char e_corrupted_regexp_program[]
	INIT(= N_("E44: Corrupted regexp program"));
EXTERN char e_readonly_option_is_set_add_bang_to_override[]
	INIT(= N_("E45: 'readonly' option is set (add ! to override)"));
#ifdef FEAT_EVAL
EXTERN char e_cannot_change_readonly_variable_str[]
	INIT(= N_("E46: Cannot change read-only variable \"%s\""));
#endif
#ifdef FEAT_QUICKFIX
EXTERN char e_error_while_reading_errorfile[]
	INIT(= N_("E47: Error while reading errorfile"));
#endif
#ifdef HAVE_SANDBOX
EXTERN char e_not_allowed_in_sandbox[]
	INIT(= N_("E48: Not allowed in sandbox"));
#endif
EXTERN char e_invalid_scroll_size[]
	INIT(= N_("E49: Invalid scroll size"));
EXTERN char e_too_many_z[]
	INIT(= N_("E50: Too many \\z("));
EXTERN char e_too_many_str_open[]
	INIT(= N_("E51: Too many %s("));
EXTERN char e_unmatched_z[]
	INIT(= N_("E52: Unmatched \\z("));
EXTERN char e_unmatched_str_percent_open[]
	INIT(= N_("E53: Unmatched %s%%("));
EXTERN char e_unmatched_str_open[]
	INIT(= N_("E54: Unmatched %s("));
EXTERN char e_unmatched_str_close[]
	INIT(= N_("E55: Unmatched %s)"));
EXTERN char e_invalid_character_after_str_at[]
	INIT(= N_("E59: invalid character after %s@"));
EXTERN char e_too_many_complex_str_curly[]
	INIT(= N_("E60: Too many complex %s{...}s"));

#ifdef FEAT_EVAL
EXTERN char e_undefined_variable_str[]
	INIT(= N_("E121: Undefined variable: %s"));
EXTERN char e_undefined_variable_char_str[]
	INIT(= N_("E121: Undefined variable: %c:%s"));
#endif
#ifndef FEAT_DIGRAPHS
EXTERN char e_no_digraphs_version[]
	INIT(= N_("E196: No digraphs in this version"));
#endif
EXTERN char e_ambiguous_use_of_user_defined_command[]
	INIT(= N_("E464: Ambiguous use of user-defined command"));
EXTERN char e_invalid_command[]
	INIT(= N_("E476: Invalid command"));
#ifdef FEAT_EVAL
EXTERN char e_invalid_command_str[]
	INIT(= N_("E476: Invalid command: %s"));
EXTERN char e_list_value_has_more_items_than_targets[]
	INIT(= N_("E710: List value has more items than targets"));
EXTERN char e_list_value_does_not_have_enough_items[]
	INIT(= N_("E711: List value does not have enough items"));
EXTERN char e_cannot_slice_dictionary[]
	INIT(= N_("E719: Cannot slice a Dictionary"));
EXTERN char e_assert_fails_second_arg[]
	INIT(= N_("E856: \"assert_fails()\" second argument must be a string or a list with one or two strings"));
EXTERN char e_using_invalid_value_as_string_str[]
	INIT(= N_("E908: using an invalid value as a String: %s"));
EXTERN char e_cannot_index_special_variable[]
	INIT(= N_("E909: Cannot index a special variable"));
#endif
EXTERN char e_command_not_supported_in_vim9_script_missing_var_str[]
	INIT(= N_("E1100: Command not supported in Vim9 script (missing :var?): %s"));
#ifdef FEAT_EVAL
EXTERN char e_variable_not_found_str[]
	INIT(= N_("E1001: Variable not found: %s"));
EXTERN char e_syntax_error_at_str[]
	INIT(= N_("E1002: Syntax error at %s"));
EXTERN char e_missing_return_value[]
	INIT(= N_("E1003: Missing return value"));
EXTERN char e_white_space_required_before_and_after_str_at_str[]
	INIT(= N_("E1004: White space required before and after '%s' at \"%s\""));
EXTERN char e_too_many_argument_types[]
	INIT(= N_("E1005: Too many argument types"));
EXTERN char e_str_is_used_as_argument[]
	INIT(= N_("E1006: %s is used as an argument"));
EXTERN char e_mandatory_argument_after_optional_argument[]
	INIT(= N_("E1007: Mandatory argument after optional argument"));
EXTERN char e_missing_type[]
	INIT(= N_("E1008: Missing <type>"));
EXTERN char e_missing_gt_after_type[]
	INIT(= N_("E1009: Missing > after type"));
EXTERN char e_type_not_recognized_str[]
	INIT(= N_("E1010: Type not recognized: %s"));
EXTERN char e_name_too_long_str[]
	INIT(= N_("E1011: Name too long: %s"));
EXTERN char e_type_mismatch_expected_str_but_got_str[]
	INIT(= N_("E1012: Type mismatch; expected %s but got %s"));
EXTERN char e_argument_nr_type_mismatch_expected_str_but_got_str[]
	INIT(= N_("E1013: Argument %d: type mismatch, expected %s but got %s"));
EXTERN char e_invalid_key_str[]
	INIT(= N_("E1014: Invalid key: %s"));
EXTERN char e_name_expected_str[]
	INIT(= N_("E1015: Name expected: %s"));
EXTERN char e_cannot_declare_a_scope_variable[]
	INIT(= N_("E1016: Cannot declare a %s variable: %s"));
EXTERN char e_cannot_declare_an_environment_variable[]
	INIT(= N_("E1016: Cannot declare an environment variable: %s"));
EXTERN char e_variable_already_declared[]
	INIT(= N_("E1017: Variable already declared: %s"));
EXTERN char e_cannot_assign_to_constant[]
	INIT(= N_("E1018: Cannot assign to a constant: %s"));
EXTERN char e_can_only_concatenate_to_string[]
	INIT(= N_("E1019: Can only concatenate to string"));
EXTERN char e_cannot_use_operator_on_new_variable[]
	INIT(= N_("E1020: Cannot use an operator on a new variable: %s"));
EXTERN char e_const_requires_a_value[]
	INIT(= N_("E1021: Const requires a value"));
EXTERN char e_type_or_initialization_required[]
	INIT(= N_("E1022: Type or initialization required"));
EXTERN char e_using_number_as_bool_nr[]
	INIT(= N_("E1023: Using a Number as a Bool: %lld"));
EXTERN char e_using_number_as_string[]
	INIT(= N_("E1024: Using a Number as a String"));
EXTERN char e_using_rcurly_outside_if_block_scope[]
	INIT(= N_("E1025: Using } outside of a block scope"));
EXTERN char e_missing_rcurly[]
	INIT(= N_("E1026: Missing }"));
EXTERN char e_missing_return_statement[]
	INIT(= N_("E1027: Missing return statement"));
EXTERN char e_compiling_def_function_failed[]
	INIT(= N_("E1028: Compiling :def function failed"));
EXTERN char e_expected_str_but_got_str[]
	INIT(= N_("E1029: Expected %s but got %s"));
EXTERN char e_using_string_as_number_str[]
	INIT(= N_("E1030: Using a String as a Number: \"%s\""));
EXTERN char e_cannot_use_void_value[]
	INIT(= N_("E1031: Cannot use void value"));
EXTERN char e_missing_catch_or_finally[]
	INIT(= N_("E1032: Missing :catch or :finally"));
EXTERN char e_catch_unreachable_after_catch_all[]
	INIT(= N_("E1033: Catch unreachable after catch-all"));
EXTERN char e_cannot_use_reserved_name[]
	INIT(= N_("E1034: Cannot use reserved name %s"));
EXTERN char e_percent_requires_number_arguments[]
	INIT(= N_("E1035: % requires number arguments"));
EXTERN char e_char_requires_number_or_float_arguments[]
	INIT(= N_("E1036: %c requires number or float arguments"));
EXTERN char e_cannot_use_str_with_str[]
	INIT(= N_("E1037: Cannot use \"%s\" with %s"));
EXTERN char e_vim9script_can_only_be_used_in_script[]
	INIT(= N_("E1038: \"vim9script\" can only be used in a script"));
EXTERN char e_vim9script_must_be_first_command_in_script[]
	INIT(= N_("E1039: \"vim9script\" must be the first command in a script"));
#endif
EXTERN char e_cannot_use_scriptversion_after_vim9script[]
	INIT(= N_("E1040: Cannot use :scriptversion after :vim9script"));
#ifdef FEAT_EVAL
EXTERN char e_redefining_script_item_str[]
	INIT(= N_("E1041: Redefining script item %s"));
EXTERN char e_export_can_only_be_used_in_vim9script[]
	INIT(= N_("E1042: Export can only be used in vim9script"));
EXTERN char e_invalid_command_after_export[]
	INIT(= N_("E1043: Invalid command after :export"));
EXTERN char e_export_with_invalid_argument[]
	INIT(= N_("E1044: Export with invalid argument"));
EXTERN char e_missing_as_after_star[]
	INIT(= N_("E1045: Missing \"as\" after *"));
EXTERN char e_missing_comma_in_import[]
	INIT(= N_("E1046: Missing comma in import"));
EXTERN char e_syntax_error_in_import[]
	INIT(= N_("E1047: Syntax error in import"));
EXTERN char e_item_not_found_in_script_str[]
	INIT(= N_("E1048: Item not found in script: %s"));
EXTERN char e_item_not_exported_in_script_str[]
	INIT(= N_("E1049: Item not exported in script: %s"));
EXTERN char e_colon_required_before_range_str[]
	INIT(= N_("E1050: Colon required before a range: %s"));
EXTERN char e_wrong_argument_type_for_plus[]
	INIT(= N_("E1051: Wrong argument type for +"));
EXTERN char e_cannot_declare_an_option[]
	INIT(= N_("E1052: Cannot declare an option: %s"));
EXTERN char e_could_not_import_str[]
	INIT(= N_("E1053: Could not import \"%s\""));
EXTERN char e_variable_already_declared_in_script_str[]
	INIT(= N_("E1054: Variable already declared in the script: %s"));
EXTERN char e_missing_name_after_dots[]
	INIT(= N_("E1055: Missing name after ..."));
EXTERN char e_expected_type_str[]
	INIT(= N_("E1056: Expected a type: %s"));
EXTERN char e_missing_enddef[]
	INIT(= N_("E1057: Missing :enddef"));
EXTERN char e_function_nesting_too_deep[]
	INIT(= N_("E1058: Function nesting too deep"));
EXTERN char e_no_white_space_allowed_before_colon_str[]
	INIT(= N_("E1059: No white space allowed before colon: %s"));
EXTERN char e_expected_dot_after_name_str[]
	INIT(= N_("E1060: Expected dot after name: %s"));
EXTERN char e_cannot_find_function_str[]
	INIT(= N_("E1061: Cannot find function %s"));
EXTERN char e_cannot_index_number[]
	INIT(= N_("E1062: Cannot index a Number"));
EXTERN char e_type_mismatch_for_v_variable[]
	INIT(= N_("E1063: Type mismatch for v: variable"));
// E1064 unused
// E1065 unused
EXTERN char e_cannot_declare_a_register_str[]
	INIT(= N_("E1066: Cannot declare a register: %s"));
EXTERN char e_separator_mismatch_str[]
	INIT(= N_("E1067: Separator mismatch: %s"));
EXTERN char e_no_white_space_allowed_before_str_str[]
	INIT(= N_("E1068: No white space allowed before '%s': %s"));
EXTERN char e_white_space_required_after_str_str[]
	INIT(= N_("E1069: White space required after '%s': %s"));
EXTERN char e_missing_from[]
	INIT(= N_("E1070: Missing \"from\""));
EXTERN char e_invalid_string_after_from[]
	INIT(= N_("E1071: Invalid string after \"from\""));
EXTERN char e_cannot_compare_str_with_str[]
	INIT(= N_("E1072: Cannot compare %s with %s"));
EXTERN char e_name_already_defined_str[]
	INIT(= N_("E1073: Name already defined: %s"));
EXTERN char e_no_white_space_allowed_after_dot[]
	INIT(= N_("E1074: No white space allowed after dot"));
EXTERN char e_namespace_not_supported_str[]
	INIT(= N_("E1075: Namespace not supported: %s"));
EXTERN char e_this_vim_is_not_compiled_with_float_support[]
	INIT(= N_("E1076: This Vim is not compiled with float support"));
EXTERN char e_missing_argument_type_for_str[]
	INIT(= N_("E1077: Missing argument type for %s"));
// E1078 unused
// E1079 unused
// E1080 unused
EXTERN char e_cannot_unlet_str[]
	INIT(= N_("E1081: Cannot unlet %s"));
EXTERN char e_cannot_use_namespaced_variable[]
	INIT(= N_("E1082: Cannot use a namespaced variable: %s"));
EXTERN char e_missing_backtick[]
	INIT(= N_("E1083: Missing backtick"));
EXTERN char e_cannot_delete_vim9_script_function_str[]
	INIT(= N_("E1084: Cannot delete Vim9 script function %s"));
EXTERN char e_not_callable_type_str[]
	INIT(= N_("E1085: Not a callable type: %s"));
EXTERN char e_cannot_use_function_inside_def[]
	INIT(= N_("E1086: Cannot use :function inside :def"));
EXTERN char e_cannot_use_index_when_declaring_variable[]
	INIT(= N_("E1087: Cannot use an index when declaring a variable"));
// E1088 unused
EXTERN char e_unknown_variable_str[]
	INIT(= N_("E1089: Unknown variable: %s"));
EXTERN char e_cannot_assign_to_argument[]
	INIT(= N_("E1090: Cannot assign to argument %s"));
EXTERN char e_function_is_not_compiled_str[]
	INIT(= N_("E1091: Function is not compiled: %s"));
EXTERN char e_cannot_use_list_for_declaration[]
	INIT(= N_("E1092: Cannot use a list for a declaration"));
EXTERN char e_expected_nr_items_but_got_nr[]
	INIT(= N_("E1093: Expected %d items but got %d"));
EXTERN char e_import_can_only_be_used_in_script[]
	INIT(= N_("E1094: Import can only be used in a script"));
EXTERN char e_unreachable_code_after_return[]
	INIT(= N_("E1095: Unreachable code after :return"));
EXTERN char e_returning_value_in_function_without_return_type[]
	INIT(= N_("E1096: Returning a value in a function without a return type"));
EXTERN char e_line_incomplete[]
	INIT(= N_("E1097: Line incomplete"));
// E1098 unused
EXTERN char e_unknown_error_while_executing_str[]
	INIT(= N_("E1099: Unknown error while executing %s"));
EXTERN char e_cannot_declare_script_variable_in_function[]
	INIT(= N_("E1101: Cannot declare a script variable in a function: %s"));
EXTERN char e_lambda_function_not_found_str[]
	INIT(= N_("E1102: Lambda function not found: %s"));
EXTERN char e_dictionary_not_set[]
	INIT(= N_("E1103: Dictionary not set"));
EXTERN char e_missing_gt[]
	INIT(= N_("E1104: Missing >"));
EXTERN char e_cannot_convert_str_to_string[]
	INIT(= N_("E1105: Cannot convert %s to string"));
EXTERN char e_one_argument_too_many[]
	INIT(= N_("E1106: One argument too many"));
EXTERN char e_nr_arguments_too_many[]
	INIT(= N_("E1106: %d arguments too many"));
EXTERN char e_string_list_dict_or_blob_required[]
	INIT(= N_("E1107: String, List, Dict or Blob required"));
EXTERN char e_item_not_found_str[]
	INIT(= N_("E1108: Item not found: %s"));
EXTERN char e_list_item_nr_is_not_list[]
	INIT(= N_("E1109: List item %d is not a List"));
EXTERN char e_list_item_nr_does_not_contain_3_numbers[]
	INIT(= N_("E1110: List item %d does not contain 3 numbers"));
EXTERN char e_list_item_nr_range_invalid[]
	INIT(= N_("E1111: List item %d range invalid"));
EXTERN char e_list_item_nr_cell_width_invalid[]
	INIT(= N_("E1112: List item %d cell width invalid"));
EXTERN char e_overlapping_ranges_for_nr[]
	INIT(= N_("E1113: Overlapping ranges for 0x%lx"));
EXTERN char e_only_values_of_0x100_and_higher_supported[]
	INIT(= N_("E1114: Only values of 0x100 and higher supported"));
EXTERN char e_assert_fails_fourth_argument[]
	INIT(= N_("E1115: \"assert_fails()\" fourth argument must be a number"));
EXTERN char e_assert_fails_fifth_argument[]
	INIT(= N_("E1116: \"assert_fails()\" fifth argument must be a string"));
EXTERN char e_cannot_use_bang_with_nested_def[]
	INIT(= N_("E1117: Cannot use ! with nested :def"));
EXTERN char e_cannot_change_list[]
	INIT(= N_("E1118: Cannot change list"));
EXTERN char e_cannot_change_list_item[]
	INIT(= N_("E1119: Cannot change list item"));
EXTERN char e_cannot_change_dict[]
	INIT(= N_("E1120: Cannot change dict"));
EXTERN char e_cannot_change_dict_item[]
	INIT(= N_("E1121: Cannot change dict item"));
EXTERN char e_variable_is_locked_str[]
	INIT(= N_("E1122: Variable is locked: %s"));
EXTERN char e_missing_comma_before_argument_str[]
	INIT(= N_("E1123: Missing comma before argument: %s"));
EXTERN char e_str_cannot_be_used_in_legacy_vim_script[]
	INIT(= N_("E1124: \"%s\" cannot be used in legacy Vim script"));
EXTERN char e_final_requires_a_value[]
	INIT(= N_("E1125: Final requires a value"));
EXTERN char e_cannot_use_let_in_vim9_script[]
	INIT(= N_("E1126: Cannot use :let in Vim9 script"));
EXTERN char e_missing_name_after_dot[]
	INIT(= N_("E1127: Missing name after dot"));
EXTERN char e_endblock_without_block[]
	INIT(= N_("E1128: } without {"));
EXTERN char e_throw_with_empty_string[]
	INIT(= N_("E1129: Throw with empty string"));
EXTERN char e_cannot_add_to_null_list[]
	INIT(= N_("E1130: Cannot add to null list"));
EXTERN char e_cannot_add_to_null_blob[]
	INIT(= N_("E1131: Cannot add to null blob"));
EXTERN char e_missing_function_argument[]
	INIT(= N_("E1132: Missing function argument"));
EXTERN char e_cannot_extend_null_dict[]
	INIT(= N_("E1133: Cannot extend a null dict"));
EXTERN char e_cannot_extend_null_list[]
	INIT(= N_("E1134: Cannot extend a null list"));
EXTERN char e_using_string_as_bool_str[]
	INIT(= N_("E1135: Using a String as a Bool: \"%s\""));
#endif
EXTERN char e_cmd_mapping_must_end_with_cr[]
	INIT(=N_("E1135: <Cmd> mapping must end with <CR>"));
EXTERN char e_cmd_mapping_must_end_with_cr_before_second_cmd[]
	INIT(=N_("E1136: <Cmd> mapping must end with <CR> before second <Cmd>"));
EXTERN char e_cmd_maping_must_not_include_str_key[]
	INIT(= N_("E1137: <Cmd> mapping must not include %s key"));
EXTERN char e_using_bool_as_number[]
	INIT(= N_("E1138: Using a Bool as a Number"));
EXTERN char e_missing_matching_bracket_after_dict_key[]
	INIT(= N_("E1139: Missing matching bracket after dict key"));
EXTERN char e_for_argument_must_be_sequence_of_lists[]
	INIT(= N_("E1140: :for argument must be a sequence of lists"));
EXTERN char e_indexable_type_required[]
	INIT(= N_("E1141: Indexable type required"));
EXTERN char e_non_empty_string_required[]
	INIT(= N_("E1142: Non-empty string required"));
EXTERN char e_empty_expression_str[]
	INIT(= N_("E1143: Empty expression: \"%s\""));
EXTERN char e_command_str_not_followed_by_white_space_str[]
	INIT(= N_("E1144: Command \"%s\" is not followed by white space: %s"));
EXTERN char e_missing_heredoc_end_marker_str[]
	INIT(= N_("E1145: Missing heredoc end marker: %s"));
EXTERN char e_command_not_recognized_str[]
	INIT(= N_("E1146: Command not recognized: %s"));
EXTERN char e_list_not_set[]
	INIT(= N_("E1147: List not set"));
EXTERN char e_cannot_index_str[]
	INIT(= N_("E1148: Cannot index a %s"));
EXTERN char e_script_variable_invalid_after_reload_in_function_str[]
	INIT(= N_("E1149: Script variable is invalid after reload in function %s"));
EXTERN char e_script_variable_type_changed[]
	INIT(= N_("E1150: Script variable type changed"));
EXTERN char e_mismatched_endfunction[]
	INIT(= N_("E1151: Mismatched endfunction"));
EXTERN char e_mismatched_enddef[]
	INIT(= N_("E1152: Mismatched enddef"));
EXTERN char e_invalid_operation_for_str[]
	INIT(= N_("E1153: Invalid operation for %s"));
EXTERN char e_divide_by_zero[]
	INIT(= N_("E1154: Divide by zero"));
EXTERN char e_cannot_define_autocommands_for_all_events[]
	INIT(= N_("E1155: Cannot define autocommands for ALL events"));
EXTERN char e_cannot_change_arglist_recursively[]
	INIT(= N_("E1156: Cannot change the argument list recursively"));
EXTERN char e_missing_return_type[]
	INIT(= N_("E1157: Missing return type"));
EXTERN char e_cannot_use_flatten_in_vim9_script[]
	INIT(= N_("E1158: Cannot use flatten() in Vim9 script"));
EXTERN char e_cannot_split_window_when_closing_buffer[]
	INIT(= N_("E1159: Cannot split a window when closing the buffer"));
EXTERN char e_cannot_use_default_for_variable_arguments[]
	INIT(= N_("E1160: Cannot use a default for variable arguments"));
EXTERN char e_cannot_json_encode_str[]
	INIT(= N_("E1161: Cannot json encode a %s"));
EXTERN char e_register_name_must_be_one_char_str[]
	INIT(= N_("E1162: Register name must be one character: %s"));
EXTERN char e_variable_nr_type_mismatch_expected_str_but_got_str[]
	INIT(= N_("E1163: Variable %d: type mismatch, expected %s but got %s"));
EXTERN char e_vim9cmd_must_be_followed_by_command[]
	INIT(= N_("E1164: vim9cmd must be followed by a command"));
EXTERN char e_cannot_use_range_with_assignment_str[]
	INIT(= N_("E1165: Cannot use a range with an assignment: %s"));
EXTERN char e_cannot_use_range_with_dictionary[]
	INIT(= N_("E1166: Cannot use a range with a dictionary"));
EXTERN char e_argument_name_shadows_existing_variable_str[]
	INIT(= N_("E1167: Argument name shadows existing variable: %s"));
EXTERN char e_argument_already_declared_in_script_str[]
	INIT(= N_("E1168: Argument already declared in the script: %s"));
EXTERN char e_import_as_name_not_supported_here[]
	INIT(= N_("E1169: 'import * as {name}' not supported here"));
EXTERN char e_cannot_use_hash_curly_to_start_comment[]
	INIT(= N_("E1170: Cannot use #{ to start a comment"));
EXTERN char e_missing_end_block[]
	INIT(= N_("E1171: Missing } after inline function"));
EXTERN char e_cannot_use_default_values_in_lambda[]
	INIT(= N_("E1172: Cannot use default values in a lambda"));
EXTERN char e_text_found_after_enddef_str[]
	INIT(= N_("E1173: Text found after enddef: %s"));
EXTERN char e_string_required_for_argument_nr[]
	INIT(= N_("E1174: String required for argument %d"));
EXTERN char e_non_empty_string_required_for_argument_nr[]
	INIT(= N_("E1175: Non-empty string required for argument %d"));
EXTERN char e_misplaced_command_modifier[]
	INIT(= N_("E1176: Misplaced command modifier"));
EXTERN char e_for_loop_on_str_not_supported[]
	INIT(= N_("E1177: For loop on %s not supported"));
EXTERN char e_cannot_lock_unlock_local_variable[]
	INIT(= N_("E1178: Cannot lock or unlock a local variable"));
EXTERN char e_failed_to_extract_pwd_from_str_check_your_shell_config[]
	INIT(= N_("E1179: Failed to extract PWD from %s, check your shell's config related to OSC 7"));
EXTERN char e_variable_arguments_type_must_be_list_str[]
	INIT(= N_("E1180: Variable arguments type must be a list: %s"));
EXTERN char e_cannot_use_underscore_here[]
	INIT(= N_("E1181: Cannot use an underscore here"));
EXTERN char e_blob_required[]
	INIT(= N_("E1182: Blob required"));
EXTERN char e_cannot_use_range_with_assignment_operator_str[]
	INIT(= N_("E1183: Cannot use a range with an assignment operator: %s"));
EXTERN char e_blob_not_set[]
	INIT(= N_("E1184: Blob not set"));
EXTERN char e_cannot_nest_redir[]
	INIT(= N_("E1185: Cannot nest :redir"));
EXTERN char e_missing_redir_end[]
	INIT(= N_("E1185: Missing :redir END"));
EXTERN char e_expression_does_not_result_in_value_str[]
	INIT(= N_("E1186: Expression does not result in a value: %s"));
EXTERN char e_failed_to_source_defaults[]
	INIT(= N_("E1187: Failed to source defaults.vim"));
EXTERN char e_cannot_open_terminal_from_command_line_window[]
	INIT(= N_("E1188: Cannot open a terminal from the command line window"));
EXTERN char e_cannot_use_legacy_with_command_str[]
	INIT(= N_("E1189: Cannot use :legacy with this command: %s"));
EXTERN char e_one_argument_too_few[]
	INIT(= N_("E1190: One argument too few"));
EXTERN char e_nr_arguments_too_few[]
	INIT(= N_("E1190: %d arguments too few"));
EXTERN char e_call_to_function_that_failed_to_compile_str[]
	INIT(= N_("E1191: Call to function that failed to compile: %s"));
EXTERN char e_empty_function_name[]
	INIT(= N_("E1192: Empty function name"));
// libsodium
EXTERN char e_libsodium_not_built_in[]
	INIT(= N_("E1193: cryptmethod xchacha20 not built into this Vim"));
EXTERN char e_libsodium_cannot_encrypt_header[]
	INIT(= N_("E1194: Cannot encrypt header, not enough space"));
EXTERN char e_libsodium_cannot_encrypt_buffer[]
	INIT(= N_("E1195: Cannot encrypt buffer, not enough space"));
EXTERN char e_libsodium_cannot_decrypt_header[]
	INIT(= N_("E1196: Cannot decrypt header, not enough space"));
EXTERN char e_libsodium_cannot_allocate_buffer[]
	INIT(= N_("E1197: Cannot allocate_buffer for encryption"));
EXTERN char e_libsodium_decryption_failed_header_incomplete[]
	INIT(= N_("E1198: Decryption failed: Header incomplete!"));
EXTERN char e_libsodium_cannot_decrypt_buffer[]
	INIT(= N_("E1199: Cannot decrypt buffer, not enough space"));
EXTERN char e_libsodium_decryption_failed[]
	INIT(= N_("E1200: Decryption failed!"));
EXTERN char e_libsodium_decryption_failed_premature[]
	INIT(= N_("E1201: Decryption failed: pre-mature end of file!"));
EXTERN char e_no_white_space_allowed_after_str_str[]
	INIT(= N_("E1202: No white space allowed after '%s': %s"));
EXTERN char e_dot_can_only_be_used_on_dictionary_str[]
	INIT(= N_("E1203: Dot can only be used on a dictionary: %s"));
EXTERN char e_regexp_number_after_dot_pos_search[]
	INIT(= N_("E1204: No Number allowed after .: '\\%%%c'"));
EXTERN char e_no_white_space_allowed_between_option_and[]
	INIT(= N_("E1205: No white space allowed between option and"));
EXTERN char e_dict_required_for_argument_nr[]
	INIT(= N_("E1206: Dictionary required for argument %d"));
EXTERN char e_expression_without_effect_str[]
	INIT(= N_("E1207: Expression without an effect: %s"));
EXTERN char e_complete_used_without_nargs[]
	INIT(= N_("E1208: -complete used without -nargs"));
EXTERN char e_invalid_value_for_line_number_str[]
	INIT(= N_("E1209: Invalid value for a line number: \"%s\""));
EXTERN char e_number_required_for_argument_nr[]
	INIT(= N_("E1210: Number required for argument %d"));
EXTERN char e_list_required_for_argument_nr[]
	INIT(= N_("E1211: List required for argument %d"));
EXTERN char e_bool_required_for_argument_nr[]
	INIT(= N_("E1212: Bool required for argument %d"));
EXTERN char e_redefining_imported_item_str[]
	INIT(= N_("E1213: Redefining imported item \"%s\""));
#if defined(FEAT_DIGRAPHS) && defined(FEAT_EVAL)
EXTERN char e_digraph_must_be_just_two_characters_str[]
	INIT(= N_("E1214: Digraph must be just two characters: %s"));
EXTERN char e_digraph_argument_must_be_one_character_str[]
	INIT(= N_("E1215: Digraph must be one character: %s"));
EXTERN char e_setdigraphlist_argument_must_be_list_of_lists_with_two_items[]
	INIT(= N_("E1216: setdigraphlist() argument must be a list of lists with two items"));
#endif
EXTERN char e_blob_required_for_argument_nr[]
	INIT(= N_("E1217: Blob required for argument %d"));
EXTERN char e_chan_or_job_required_for_argument_nr[]
	INIT(= N_("E1218: Channel or Job required for argument %d"));
EXTERN char e_job_required_for_argument_nr[]
	INIT(= N_("E1219: Job required for argument %d"));
