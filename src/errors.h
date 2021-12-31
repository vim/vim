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
EXTERN char e_cannot_change_readonly_variable[]
	INIT(= N_("E46: Cannot change read-only variable"));
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
EXTERN char e_nested_str[]
	INIT(= N_("E61: Nested %s*"));
EXTERN char e_nested_str_chr[]
	INIT(= N_("E62: Nested %s%c"));
EXTERN char e_invalid_use_of_underscore[]
	INIT(= N_("E63: invalid use of \\_"));
EXTERN char e_str_chr_follows_nothing[]
	INIT(= N_("E64: %s%c follows nothing"));
EXTERN char e_illegal_back_reference[]
	INIT(= N_("E65: Illegal back reference"));
#ifdef FEAT_SYN_HL
EXTERN char e_z_not_allowed_here[]
	INIT(= N_("E66: \\z( not allowed here"));
EXTERN char e_z1_z9_not_allowed_here[]
	INIT(= N_("E67: \\z1 - \\z9 not allowed here"));
#endif
EXTERN char e_missing_sb_after_str[]
	INIT(= N_("E69: Missing ] after %s%%["));
EXTERN char e_empty_str_brackets[]
	INIT(= N_("E70: Empty %s%%[]"));
EXTERN char e_invalid_character_after_str[]
	INIT(= N_("E71: Invalid character after %s%%"));
EXTERN char e_close_error_on_swap_file[]
	INIT(= N_("E72: Close error on swap file"));
EXTERN char e_tag_stack_empty[]
	INIT(= N_("E73: tag stack empty"));
EXTERN char e_command_too_complex[]
	INIT(= N_("E74: Command too complex"));
EXTERN char e_name_too_long[]
	INIT(= N_("E75: Name too long"));
EXTERN char e_too_many_brackets[]
	INIT(= N_("E76: Too many ["));
EXTERN char e_too_many_file_names[]
	INIT(= N_("E77: Too many file names"));
EXTERN char e_unknown_mark[]
	INIT(= N_("E78: Unknown mark"));
EXTERN char e_cannot_expand_wildcards[]
	INIT(= N_("E79: Cannot expand wildcards"));
EXTERN char e_error_while_writing[]
	INIT(= N_("E80: Error while writing"));
#ifdef FEAT_EVAL
EXTERN char e_using_sid_not_in_script_context[]
	INIT(= N_("E81: Using <SID> not in a script context"));
#endif
EXTERN char e_cannot_allocate_any_buffer_exiting[]
	INIT(= N_("E82: Cannot allocate any buffer, exiting..."));
EXTERN char e_cannot_allocate_buffer_using_other_one[]
	INIT(= N_("E83: Cannot allocate buffer, using other one..."));
EXTERN char e_no_modified_buffer_found[]
	INIT(= N_("E84: No modified buffer found"));
EXTERN char e_there_is_no_listed_buffer[]
	INIT(= N_("E85: There is no listed buffer"));
EXTERN char e_buffer_nr_does_not_exist[]
	INIT(= N_("E86: Buffer %ld does not exist"));
EXTERN char e_cannot_go_beyond_last_buffer[]
	INIT(= N_("E87: Cannot go beyond last buffer"));
EXTERN char e_cannot_go_before_first_buffer[]
	INIT(= N_("E88: Cannot go before first buffer"));
EXTERN char e_no_write_since_last_change_for_buffer_nr_add_bang_to_override[]
	INIT(= N_("E89: No write since last change for buffer %d (add ! to override)"));
EXTERN char e_cannot_unload_last_buffer[]
	INIT(= N_("E90: Cannot unload last buffer"));
EXTERN char e_shell_option_is_empty[]
	INIT(= N_("E91: 'shell' option is empty"));
EXTERN char e_buffer_nr_not_found[]
	INIT(= N_("E92: Buffer %d not found"));
EXTERN char e_more_than_one_match_for_str[]
	INIT(= N_("E93: More than one match for %s"));
EXTERN char e_no_matching_buffer_for_str[]
	INIT(= N_("E94: No matching buffer for %s"));
EXTERN char e_buffer_with_this_name_already_exists[]
	INIT(= N_("E95: Buffer with this name already exists"));
#if defined(FEAT_DIFF)
EXTERN char e_cannot_diff_more_than_nr_buffers[]
	INIT(= N_("E96: Cannot diff more than %d buffers"));
EXTERN char e_cannot_create_diffs[]
	INIT(= N_("E97: Cannot create diffs"));
EXTERN char e_cannot_read_diff_output[]
	INIT(= N_("E98: Cannot read diff output"));
EXTERN char e_current_buffer_is_not_in_diff_mode[]
	INIT(= N_("E99: Current buffer is not in diff mode"));
EXTERN char e_no_other_buffer_in_diff_mode[]
	INIT(= N_("E100: No other buffer in diff mode"));
EXTERN char e_more_than_two_buffers_in_diff_mode_dont_know_which_one_to_use[]
	INIT(= N_("E101: More than two buffers in diff mode, don't know which one to use"));
EXTERN char e_cant_find_buffer_str[]
	INIT(= N_("E102: Can't find buffer \"%s\""));
EXTERN char e_buffer_str_is_not_in_diff_mode[]
	INIT(= N_("E103: Buffer \"%s\" is not in diff mode"));
#endif
EXTERN char e_escape_not_allowed_in_digraph[]
	INIT(= N_("E104: Escape not allowed in digraph"));
EXTERN char e_using_loadkeymap_not_in_sourced_file[]
	INIT(= N_("E105: Using :loadkeymap not in a sourced file"));
// E106 unused
EXTERN char e_missing_parenthesis_str[]
	INIT(= N_("E107: Missing parentheses: %s"));
#ifdef FEAT_EVAL
EXTERN char e_no_such_variable_str[]
	INIT(= N_("E108: No such variable: \"%s\""));
EXTERN char e_missing_colon_after_questionmark[]
	INIT(= N_("E109: Missing ':' after '?'"));
EXTERN char e_missing_closing_paren[]
	INIT(= N_("E110: Missing ')'"));
EXTERN char e_missing_closing_square_brace[]
	INIT(= N_("E111: Missing ']'"));
#endif
EXTERN char e_option_name_missing_str[]
	INIT(= N_("E112: Option name missing: %s"));
EXTERN char e_unknown_option_str[]
	INIT(= N_("E113: Unknown option: %s"));
EXTERN char e_missing_double_quote_str[]
	INIT(= N_("E114: Missing double quote: %s"));
EXTERN char e_missing_single_quote_str[]
	INIT(= N_("E115: Missing single quote: %s"));
#ifdef FEAT_EVAL
EXTERN char e_invalid_arguments_for_function_str[]
	INIT(= N_("E116: Invalid arguments for function %s"));
EXTERN char e_unknown_function_str[]
	INIT(= N_("E117: Unknown function: %s"));
EXTERN char e_too_many_arguments_for_function_str[]
	INIT(= N_("E118: Too many arguments for function: %s"));
EXTERN char e_not_enough_arguments_for_function_str[]
	INIT(= N_("E119: Not enough arguments for function: %s"));
EXTERN char e_using_sid_not_in_script_context_str[]
	INIT(= N_("E120: Using <SID> not in a script context: %s"));
EXTERN char e_undefined_variable_str[]
	INIT(= N_("E121: Undefined variable: %s"));
EXTERN char e_undefined_variable_char_str[]
	INIT(= N_("E121: Undefined variable: %c:%s"));
EXTERN char e_function_str_already_exists_add_bang_to_replace[]
	INIT(= N_("E122: Function %s already exists, add ! to replace it"));
EXTERN char e_undefined_function_str[]
	INIT(= N_("E123: Undefined function: %s"));
EXTERN char e_missing_paren_str[]
	INIT(= N_("E124: Missing '(': %s"));
EXTERN char e_illegal_argument_str[]
	INIT(= N_("E125: Illegal argument: %s"));
EXTERN char e_missing_endfunction[]
	INIT(= N_("E126: Missing :endfunction"));
EXTERN char e_cannot_redefine_function_str_it_is_in_use[]
	INIT(= N_("E127: Cannot redefine function %s: It is in use"));
EXTERN char e_function_name_must_start_with_capital_or_s_str[]
	INIT(= N_("E128: Function name must start with a capital or \"s:\": %s"));
EXTERN char e_function_name_required[]
	INIT(= N_("E129: Function name required"));
// E130 unused
EXTERN char e_cannot_delete_function_str_it_is_in_use[]
	INIT(= N_("E131: Cannot delete function %s: It is in use"));
EXTERN char e_function_call_depth_is_higher_than_macfuncdepth[]
	INIT(= N_("E132: Function call depth is higher than 'maxfuncdepth'"));
EXTERN char e_return_not_inside_function[]
	INIT(= N_("E133: :return not inside a function"));
#endif
EXTERN char e_cannot_move_range_of_lines_into_itself[]
	INIT(= N_("E134: Cannot move a range of lines into itself"));
EXTERN char e_filter_autocommands_must_not_change_current_buffer[]
	INIT(= N_("E135: *Filter* Autocommands must not change current buffer"));
#if defined(FEAT_VIMINFO)
EXTERN char e_viminfo_too_many_errors_skipping_rest_of_file[]
	INIT(= N_("E136: viminfo: Too many errors, skipping rest of file"));
EXTERN char e_viminfo_file_is_not_writable_str[]
	INIT(= N_("E137: Viminfo file is not writable: %s"));
EXTERN char e_cant_write_viminfo_file_str[]
	INIT(= N_("E138: Can't write viminfo file %s!"));
#endif
EXTERN char e_file_is_loaded_in_another_buffer[]
	INIT(= N_("E139: File is loaded in another buffer"));
EXTERN char e_use_bang_to_write_partial_buffer[]
	INIT(= N_("E140: Use ! to write partial buffer"));
EXTERN char e_no_file_name_for_buffer_nr[]
	INIT(= N_("E141: No file name for buffer %ld"));
EXTERN char e_file_not_written_writing_is_disabled_by_write_option[]
	INIT(= N_("E142: File not written: Writing is disabled by 'write' option"));
EXTERN char e_autocommands_unexpectedly_deleted_new_buffer_str[]
	INIT(= N_("E143: Autocommands unexpectedly deleted new buffer %s"));
EXTERN char e_non_numeric_argument_to_z[]
	INIT(= N_("E144: non-numeric argument to :z"));
EXTERN char e_shell_commands_and_some_functionality_not_allowed_in_rvim[]
	INIT(= N_("E145: Shell commands and some functionality not allowed in rvim"));
EXTERN char e_regular_expressions_cant_be_delimited_by_letters[]
	INIT(= N_("E146: Regular expressions can't be delimited by letters"));
EXTERN char e_cannot_do_global_recursive_with_range[]
	INIT(= N_("E147: Cannot do :global recursive with a range"));
EXTERN char e_regular_expression_missing_from_global[]
	INIT(= N_("E148: Regular expression missing from :global"));
EXTERN char e_sorry_no_help_for_str[]
	INIT(= N_("E149: Sorry, no help for %s"));
EXTERN char e_not_a_directory_str[]
	INIT(= N_("E150: Not a directory: %s"));
EXTERN char e_no_match_str[]
	INIT(= N_("E151: No match: %s"));
EXTERN char e_cannot_open_str_for_writing_1[]
	INIT(= N_("E152: Cannot open %s for writing"));
EXTERN char e_unable_to_open_str_for_reading[]
	INIT(= N_("E153: Unable to open %s for reading"));
EXTERN char e_duplicate_tag_str_in_file_str_str[]
	INIT(= N_("E154: Duplicate tag \"%s\" in file %s/%s"));
EXTERN char e_unknown_sign_str[]
	INIT(= N_("E155: Unknown sign: %s"));
EXTERN char e_missing_sign_name[]
	INIT(= N_("E156: Missing sign name"));
EXTERN char e_invalid_sign_id_nr[]
	INIT(= N_("E157: Invalid sign ID: %d"));
EXTERN char e_invalid_buffer_name_str[]
	INIT(= N_("E158: Invalid buffer name: %s"));
EXTERN char e_missing_sign_number[]
	INIT(= N_("E159: Missing sign number"));
EXTERN char e_unknown_sign_command_str[]
	INIT(= N_("E160: Unknown sign command: %s"));
#ifdef FEAT_EVAL
EXTERN char e_breakpoint_not_found_str[]
	INIT(= N_("E161: Breakpoint not found: %s"));
#endif
EXTERN char e_no_write_since_last_change_for_buffer_str[]
	INIT(= N_("E162: No write since last change for buffer \"%s\""));
EXTERN char e_there_is_only_one_file_to_edit[]
	INIT(= N_("E163: There is only one file to edit"));
EXTERN char e_cannot_go_before_first_file[]
	INIT(= N_("E164: Cannot go before first file"));
EXTERN char e_cannot_go_beyond_last_file[]
	INIT(= N_("E165: Cannot go beyond last file"));
EXTERN char e_cant_open_linked_file_for_writing[]
	INIT(= N_("E166: Can't open linked file for writing"));
EXTERN char e_scriptencoding_used_outside_of_sourced_file[]
	INIT(= N_("E167: :scriptencoding used outside of a sourced file"));
EXTERN char e_finish_used_outside_of_sourced_file[]
	INIT(= N_("E168: :finish used outside of a sourced file"));
EXTERN char e_command_too_recursive[]
	INIT(= N_("E169: Command too recursive"));
EXTERN char e_missing_endwhile[]
	INIT(= N_("E170: Missing :endwhile"));
EXTERN char e_missing_endfor[]
	INIT(= N_("E170: Missing :endfor"));
EXTERN char e_missing_endif[]
	INIT(= N_("E171: Missing :endif"));
EXTERN char e_missing_marker[]
	INIT(= N_("E172: Missing marker"));
EXTERN char e_nr_more_file_to_edit[]
	INIT(= N_("E173: %d more file to edit"));
EXTERN char e_nr_more_files_to_edit[]
	INIT(= N_("E173: %d more files to edit"));
EXTERN char e_command_already_exists_add_bang_to_replace_it_str[]
	INIT(= N_("E174: Command already exists: add ! to replace it: %s"));
EXTERN char e_no_attribute_specified[]
	INIT(= N_("E175: No attribute specified"));
EXTERN char e_invalid_number_of_arguments[]
	INIT(= N_("E176: Invalid number of arguments"));
EXTERN char e_count_cannot_be_specified_twice[]
	INIT(= N_("E177: Count cannot be specified twice"));
EXTERN char e_invalid_default_value_for_count[]
	INIT(= N_("E178: Invalid default value for count"));
EXTERN char e_argument_required_for_str[]
	INIT(= N_("E179: argument required for %s"));
EXTERN char e_invalid_complete_value_str[]
	INIT(= N_("E180: Invalid complete value: %s"));
EXTERN char e_invalid_attribute_str[]
	INIT(= N_("E181: Invalid attribute: %s"));
EXTERN char e_invalid_command_name[]
	INIT(= N_("E182: Invalid command name"));
EXTERN char e_user_defined_commands_must_start_with_an_uppercase_letter[]
	INIT(= N_("E183: User defined commands must start with an uppercase letter"));
EXTERN char e_no_such_user_defined_command_str[]
	INIT(= N_("E184: No such user-defined command: %s"));
EXTERN char e_cannot_find_color_scheme_str[]
	INIT(= N_("E185: Cannot find color scheme '%s'"));
EXTERN char e_no_previous_directory[]
	INIT(= N_("E186: No previous directory"));
EXTERN char e_directory_unknown[]
	INIT(= N_("E187: Directory unknown"));
EXTERN char e_obtaining_window_position_not_implemented_for_this_platform[]
	INIT(= N_("E188: Obtaining window position not implemented for this platform"));
EXTERN char e_str_exists_add_bang_to_override[]
	INIT(= N_("E189: \"%s\" exists (add ! to override)"));
EXTERN char e_cannot_open_str_for_writing_2[]
	INIT(= N_("E190: Cannot open \"%s\" for writing"));
EXTERN char e_argument_must_be_letter_or_forward_backward_quote[]
	INIT(= N_("E191: Argument must be a letter or forward/backward quote"));
EXTERN char e_recursive_use_of_normal_too_deep[]
	INIT(= N_("E192: Recursive use of :normal too deep"));
EXTERN char e_str_not_inside_function[]
	INIT(= N_("E193: %s not inside a function"));
EXTERN char e_no_alternate_file_name_to_substitute_for_hash[]
	INIT(= N_("E194: No alternate file name to substitute for '#'"));
EXTERN char e_cannot_open_viminfo_file_for_reading[]
	INIT(= N_("E195: Cannot open viminfo file for reading"));
#ifndef FEAT_DIGRAPHS
EXTERN char e_no_digraphs_version[]
	INIT(= N_("E196: No digraphs in this version"));
#endif
EXTERN char e_cannot_set_language_to_str[]
	INIT(= N_("E197: Cannot set language to \"%s\""));
// E198 unused
EXTERN char e_active_window_or_buffer_deleted[]
	INIT(= N_("E199: Active window or buffer deleted"));
EXTERN char e_readpre_autocommands_made_file_unreadable[]
	INIT(= N_("E200: *ReadPre autocommands made the file unreadable"));
EXTERN char e_readpre_autocommands_must_not_change_current_buffer[]
	INIT(= N_("E201: *ReadPre autocommands must not change current buffer"));
EXTERN char e_conversion_mad_file_unreadable[]
	INIT(= N_("E202: Conversion made file unreadable!"));
EXTERN char e_autocommands_deleted_or_unloaded_buffer_to_be_written[]
	INIT(= N_("E203: Autocommands deleted or unloaded buffer to be written"));
EXTERN char e_autocommands_changed_number_of_lines_in_unexpected_way[]
	INIT(= N_("E204: Autocommand changed number of lines in unexpected way"));
EXTERN char e_patchmode_cant_save_original_file[]
	INIT(= N_("E205: Patchmode: can't save original file"));
EXTERN char e_patchmode_cant_touch_empty_original_file[]
	INIT(= N_("E206: patchmode: can't touch empty original file"));
EXTERN char e_cant_delete_backup_file[]
	INIT(= N_("E207: Can't delete backup file"));
EXTERN char e_error_writing_to_str[]
	INIT(= N_("E208: Error writing to \"%s\""));
EXTERN char e_error_closing_str[]
	INIT(= N_("E209: Error closing \"%s\""));
EXTERN char e_error_reading_str[]
	INIT(= N_("E210: Error reading \"%s\""));
EXTERN char e_file_str_no_longer_available[]
	INIT(= N_("E211: File \"%s\" no longer available"));
EXTERN char e_cant_open_file_for_writing[]
	INIT(= N_("E212: Can't open file for writing"));
EXTERN char e_cannot_convert_add_bang_to_write_without_conversion[]
	INIT(= N_("E213: Cannot convert (add ! to write without conversion)"));
EXTERN char e_cant_find_temp_file_for_writing[]
	INIT(= N_("E214: Can't find temp file for writing"));
EXTERN char e_illegal_character_after_star_str[]
	INIT(= N_("E215: Illegal character after *: %s"));
EXTERN char e_no_such_event_str[]
	INIT(= N_("E216: No such event: %s"));
EXTERN char e_no_such_group_or_event_str[]
	INIT(= N_("E216: No such group or event: %s"));
EXTERN char e_cant_execute_autocommands_for_all_events[]
	INIT(= N_("E217: Can't execute autocommands for ALL events"));
EXTERN char e_autocommand_nesting_too_deep[]
	INIT(= N_("E218: autocommand nesting too deep"));
EXTERN char e_missing_open_curly[]
	INIT(= N_("E219: Missing {."));
EXTERN char e_missing_close_curly[]
	INIT(= N_("E220: Missing }."));
EXTERN char e_marker_cannot_start_with_lower_case_letter[]
	INIT(= N_("E221: Marker cannot start with lower case letter"));
EXTERN char e_add_to_internal_buffer_that_was_already_read_from[]
	INIT(= N_("E222: Add to internal buffer that was already read from"));
EXTERN char e_recursive_mapping[]
	INIT(= N_("E223: recursive mapping"));
EXTERN char e_global_abbreviation_already_exists_for_str[]
	INIT(= N_("E224: global abbreviation already exists for %s"));
EXTERN char e_global_mapping_already_exists_for_str[]
	INIT(= N_("E225: global mapping already exists for %s"));
EXTERN char e_abbreviation_already_exists_for_str[]
	INIT(= N_("E226: abbreviation already exists for %s"));
EXTERN char e_mapping_already_exists_for_str[]
	INIT(= N_("E227: mapping already exists for %s"));
EXTERN char e_makemap_illegal_mode[]
	INIT(= N_("E228: makemap: Illegal mode"));
EXTERN char e_cannot_start_the_GUI[]
	INIT(= N_("E229: Cannot start the GUI"));


EXTERN char e_window_layout_changed_unexpectedly[]
	INIT(= N_("E249: window layout changed unexpectedly"));
#if defined(FEAT_GUI) || defined(FEAT_TERMGUICOLORS)
EXTERN char e_cannot_allocate_color_str[]
	INIT(= N_("E254: Cannot allocate color %s"));
#endif

EXTERN char e_internal_error_lalloc_zero[]
	INIT(= N_("E341: Internal error: lalloc(0, )"));
EXTERN char e_out_of_memory_allocating_nr_bytes[]
	INIT(= N_("E342: Out of memory!  (allocating %lu bytes)"));
EXTERN char e_no_such_group_str[]
	INIT(= N_("E367: No such group: \"%s\""));
EXTERN char e_cannot_write_buftype_option_is_set[]
	INIT(= N_("E382: Cannot write, 'buftype' option is set"));

EXTERN char e_ambiguous_use_of_user_defined_command[]
	INIT(= N_("E464: Ambiguous use of user-defined command"));
EXTERN char e_invalid_command[]
	INIT(= N_("E476: Invalid command"));
#ifdef FEAT_EVAL
EXTERN char e_invalid_command_str[]
	INIT(= N_("E476: Invalid command: %s"));

	// E502
EXTERN char e_is_a_directory[]
	INIT(= N_("is a directory"));
	// E503
EXTERN char e_is_not_file_or_writable_device[]
	INIT(= N_("is not a file or writable device"));

	// E504
EXTERN char e_is_read_only_cannot_override_W_in_cpoptions[]
	INIT(= N_("is read-only (cannot override: \"W\" in 'cpoptions')"));
	// E505
EXTERN char e_is_read_only_add_bang_to_override[]
	INIT(= N_("is read-only (add ! to override)"));
EXTERN char e_canot_write_to_backup_file_add_bang_to_override[]
	INIT(= N_("E506: Can't write to backup file (add ! to override)"));
EXTERN char e_close_error_for_backup_file_add_bang_to_write_anyway[]
	INIT(= N_("E507: Close error for backup file (add ! to write anyway)"));
EXTERN char e_cant_read_file_for_backup_add_bang_to_write_anyway[]
	INIT(= N_("E508: Can't read file for backup (add ! to write anyway)"));
EXTERN char e_cannot_create_backup_file_add_bang_to_write_anyway[]
	INIT(= N_("E509: Cannot create backup file (add ! to override)"));
EXTERN char e_cant_make_backup_file_add_bang_to_write_anyway[]
	INIT(= N_("E510: Can't make backup file (add ! to write anyway)"));
EXTERN char e_close_failed[]
	INIT(= N_("E512: Close failed"));
EXTERN char e_write_error_conversion_failed_make_fenc_empty_to_override[]
	INIT(= N_("E513: write error, conversion failed (make 'fenc' empty to override)"));
EXTERN char e_write_error_conversion_failed_in_line_nr_make_fenc_empty_to_override[]
	INIT(= N_("E513: write error, conversion failed in line %ld (make 'fenc' empty to override)"));
EXTERN char e_write_error_file_system_full[]
	INIT(= N_("E514: write error (file system full?)"));
EXTERN char e_no_buffers_were_unloaded[]
	INIT(= N_("E515: No buffers were unloaded"));
EXTERN char e_no_buffers_were_deleted[]
	INIT(= N_("E516: No buffers were deleted"));
EXTERN char e_no_buffers_were_wiped_out[]
	INIT(= N_("E517: No buffers were wiped out"));

EXTERN char e_no_argument_to_delete[]
	INIT(= N_("E610: No argument to delete"));
#ifdef FEAT_NETBEANS_INTG
	// E656
EXTERN char e_netbeans_disallows_writes_of_unmodified_buffers[]
	INIT(= N_("NetBeans disallows writes of unmodified buffers"));
	// E657
EXTERN char e_partial_writes_disallowed_for_netbeans_buffers[]
	INIT(= N_("Partial writes disallowed for NetBeans buffers"));
#endif
EXTERN char e_no_matching_autocommands_for_acwrite_buffer[]
	INIT(= N_("E676: No matching autocommands for acwrite buffer"));
EXTERN char e_buffer_nr_invalid_buffer_number[]
	INIT(= N_("E680: <buffer=%d>: invalid buffer number"));
EXTERN char e_cannot_index_a_funcref[]
	INIT(= N_("E695: Cannot index a Funcref"));

EXTERN char e_list_value_has_more_items_than_targets[]
	INIT(= N_("E710: List value has more items than targets"));
EXTERN char e_list_value_does_not_have_enough_items[]
	INIT(= N_("E711: List value does not have enough items"));
EXTERN char e_cannot_slice_dictionary[]
	INIT(= N_("E719: Cannot slice a Dictionary"));
EXTERN char e_value_is_locked[]
	INIT(= N_("E741: Value is locked"));
EXTERN char e_value_is_locked_str[]
	INIT(= N_("E741: Value is locked: %s"));
EXTERN char e_cannot_change_value[]
	INIT(= N_("E742: Cannot change value"));
EXTERN char e_cannot_change_value_of_str[]
	INIT(= N_("E742: Cannot change value of %s"));
EXTERN char e_cannot_set_variable_in_sandbox[]
	INIT(= N_("E794: Cannot set variable in the sandbox"));
EXTERN char e_cannot_set_variable_in_sandbox_str[]
	INIT(= N_("E794: Cannot set variable in the sandbox: \"%s\""));
EXTERN char e_cannot_delete_variable[]
	INIT(= N_("E795: Cannot delete variable"));
	// E796
EXTERN char e_writing_to_device_disabled_with_opendevice_option[]
	INIT(= N_("writing to device disabled with 'opendevice' option"));
EXTERN char e_cannot_delete_variable_str[]
	INIT(= N_("E795: Cannot delete variable %s"));
#endif

EXTERN char e_blowfish_big_little_endian_use_wrong[]
	INIT(= N_("E817: Blowfish big/little endian use wrong"));
EXTERN char e_sha256_test_failed[]
	INIT(= N_("E818: sha256 test failed"));
EXTERN char e_blowfish_test_failed[]
	INIT(= N_("E819: Blowfish test failed"));
EXTERN char e_sizeof_uint32_isnot_four[]
	INIT(= N_("E820: sizeof(uint32_t) != 4"));
EXTERN char e_bf_key_init_called_with_empty_password[]
	INIT(= N_("E831: bf_key_init() called with empty password"));
EXTERN char e_conflicts_with_value_of_listchars[]
	INIT(= N_("E834: Conflicts with value of 'listchars'"));
EXTERN char e_conflicts_with_value_of_fillchars[]
	INIT(= N_("E835: Conflicts with value of 'fillchars'"));
EXTERN char e_autocommands_caused_command_to_abort[]
	INIT(= N_("E855: Autocommands caused command to abort"));
#ifdef FEAT_EVAL
EXTERN char e_assert_fails_second_arg[]
	INIT(= N_("E856: \"assert_fails()\" second argument must be a string or a list with one or two strings"));

EXTERN char e_using_invalid_value_as_string_str[]
	INIT(= N_("E908: using an invalid value as a String: %s"));
EXTERN char e_cannot_index_special_variable[]
	INIT(= N_("E909: Cannot index a special variable"));
#endif
EXTERN char e_buffer_cannot_be_registered[]
	INIT(= N_("E931: Buffer cannot be registered"));
EXTERN char e_cannot_delete_current_group[]
	INIT(= N_("E936: Cannot delete the current group"));
EXTERN char e_attempt_to_delete_buffer_that_is_in_use_str[]
	INIT(= N_("E937: Attempt to delete a buffer that is in use: %s"));
#ifdef FEAT_TERMINAL
EXTERN char e_job_still_running[]
	INIT(= N_("E948: Job still running"));
EXTERN char e_job_still_running_add_bang_to_end_the_job[]
	INIT(= N_("E948: Job still running (add ! to end the job)"));
EXTERN char e_file_changed_while_writing[]
	INIT(= N_("E949: File changed while writing"));
#endif
EXTERN char_u e_invalid_column_number_nr[]
	INIT(= N_("E964: Invalid column number: %ld"));
EXTERN char_u e_invalid_line_number_nr[]
	INIT(= N_("E966: Invalid line number: %ld"));
EXTERN char e_blob_value_does_not_have_right_number_of_bytes[]
	INIT(= N_("E972: Blob value does not have the right number of bytes"));

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
EXTERN char e_type_mismatch_expected_str_but_got_str_in_str[]
	INIT(= N_("E1012: Type mismatch; expected %s but got %s in %s"));
EXTERN char e_argument_nr_type_mismatch_expected_str_but_got_str[]
	INIT(= N_("E1013: Argument %d: type mismatch, expected %s but got %s"));
EXTERN char e_argument_nr_type_mismatch_expected_str_but_got_str_in_str[]
	INIT(= N_("E1013: Argument %d: type mismatch, expected %s but got %s in %s"));
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
#endif
EXTERN char e_missing_rcurly[]
	INIT(= N_("E1026: Missing }"));
#ifdef FEAT_EVAL
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
EXTERN char e_syntax_error_in_import_str[]
	INIT(= N_("E1047: Syntax error in import: %s"));
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
#endif
EXTERN char e_yank_register_changed_while_using_it[]
	INIT(= N_("E1064: Yank register changed while using it"));
#ifdef FEAT_EVAL
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
EXTERN char e_function_reference_invalid[]
	INIT(= N_("E1086: Function reference invalid"));
EXTERN char e_cannot_use_index_when_declaring_variable[]
	INIT(= N_("E1087: Cannot use an index when declaring a variable"));
// E1088 unused
EXTERN char e_unknown_variable_str[]
	INIT(= N_("E1089: Unknown variable: %s"));
EXTERN char e_cannot_assign_to_argument[]
	INIT(= N_("E1090: Cannot assign to argument %s"));
EXTERN char e_function_is_not_compiled_str[]
	INIT(= N_("E1091: Function is not compiled: %s"));
// E1092 unused
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
EXTERN char e_string_list_or_blob_required[]
	INIT(= N_("E1098: String, List or Blob required"));
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
EXTERN char e_cmd_mapping_must_end_with_cr_before_second_cmd[]
	INIT(= N_("E1136: <Cmd> mapping must end with <CR> before second <Cmd>"));
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
EXTERN char e_variable_nr_type_mismatch_expected_str_but_got_str_in_str[]
	INIT(= N_("E1163: Variable %d: type mismatch, expected %s but got %s in %s"));
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
EXTERN char e_text_found_after_str_str[]
	INIT(= N_("E1173: Text found after %s: %s"));
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
EXTERN char e_complete_used_without_allowing_arguments[]
	INIT(= N_("E1208: -complete used without allowing arguments"));
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
EXTERN char e_digraph_setlist_argument_must_be_list_of_lists_with_two_items[]
	INIT(= N_("E1216: digraph_setlist() argument must be a list of lists with two items"));
#endif
EXTERN char e_chan_or_job_required_for_argument_nr[]
	INIT(= N_("E1217: Channel or Job required for argument %d"));
EXTERN char e_job_required_for_argument_nr[]
	INIT(= N_("E1218: Job required for argument %d"));
EXTERN char e_float_or_number_required_for_argument_nr[]
	INIT(= N_("E1219: Float or Number required for argument %d"));
EXTERN char e_string_or_number_required_for_argument_nr[]
	INIT(= N_("E1220: String or Number required for argument %d"));
EXTERN char e_string_or_blob_required_for_argument_nr[]
	INIT(= N_("E1221: String or Blob required for argument %d"));
EXTERN char e_string_or_list_required_for_argument_nr[]
	INIT(= N_("E1222: String or List required for argument %d"));
EXTERN char e_string_or_dict_required_for_argument_nr[]
	INIT(= N_("E1223: String or Dictionary required for argument %d"));
EXTERN char e_string_number_or_list_required_for_argument_nr[]
	INIT(= N_("E1224: String, Number or List required for argument %d"));
EXTERN char e_string_list_or_dict_required_for_argument_nr[]
	INIT(= N_("E1225: String, List or Dictionary required for argument %d"));
EXTERN char e_list_or_blob_required_for_argument_nr[]
	INIT(= N_("E1226: List or Blob required for argument %d"));
EXTERN char e_list_or_dict_required_for_argument_nr[]
	INIT(= N_("E1227: List or Dictionary required for argument %d"));
EXTERN char e_list_dict_or_blob_required_for_argument_nr[]
	INIT(= N_("E1228: List, Dictionary or Blob required for argument %d"));
EXTERN char e_expected_dictionary_for_using_key_str_but_got_str[]
	INIT(= N_("E1229: Expected dictionary for using key \"%s\", but got %s"));
EXTERN char e_encryption_sodium_mlock_failed[]
	INIT(= N_("E1230: Encryption: sodium_mlock() failed"));
EXTERN char e_cannot_use_bar_to_separate_commands_here_str[]
	INIT(= N_("E1231: Cannot use a bar to separate commands here: %s"));
EXTERN char e_argument_of_exists_compiled_must_be_literal_string[]
	INIT(= N_("E1232: Argument of exists_compiled() must be a literal string"));
EXTERN char e_exists_compiled_can_only_be_used_in_def_function[]
	INIT(= N_("E1233: exists_compiled() can only be used in a :def function"));
EXTERN char e_legacy_must_be_followed_by_command[]
	INIT(= N_("E1234: legacy must be followed by a command"));
EXTERN char e_function_reference_is_not_set[]
	INIT(= N_("E1235: Function reference is not set"));
EXTERN char e_cannot_use_str_itself_it_is_imported_with_star[]
	INIT(= N_("E1236: Cannot use %s itself, it is imported with '*'"));
EXTERN char e_no_such_user_defined_command_in_current_buffer_str[]
	INIT(= N_("E1237: No such user-defined command in current buffer: %s"));
EXTERN char e_blob_required_for_argument_nr[]
	INIT(= N_("E1238: Blob required for argument %d"));
EXTERN char e_invalid_value_for_blob_nr[]
	INIT(= N_("E1239: Invalid value for blob: %d"));
EXTERN char e_resulting_text_too_long[]
	INIT(= N_("E1240: Resulting text too long"));
EXTERN char e_separator_not_supported_str[]
	INIT(= N_("E1241: Separator not supported: %s"));
EXTERN char e_no_white_space_allowed_before_separator_str[]
	INIT(= N_("E1242: No white space allowed before separator: %s"));
EXTERN char e_ascii_code_not_in_range[]
	INIT(= N_("E1243: ASCII code not in 32-127 range"));
EXTERN char e_bad_color_string_str[]
	INIT(= N_("E1244: Bad color string: %s"));
EXTERN char e_cannot_expand_sfile_in_vim9_function[]
	INIT(= N_("E1245: Cannot expand <sfile> in a Vim9 function"));
EXTERN char e_cannot_find_variable_to_unlock_str[]
	INIT(= N_("E1246: Cannot find variable to (un)lock: %s"));
EXTERN char e_line_number_out_of_range[]
	INIT(= N_("E1247: Line number out of range"));
EXTERN char e_closure_called_from_invalid_context[]
	INIT(= N_("E1248: Closure called from invalid context"));
EXTERN char e_highlight_group_name_too_long[]
	INIT(= N_("E1249: Highlight group name too long"));
EXTERN char e_argument_of_str_must_be_list_string_dictionary_or_blob[]
	INIT(= N_("E1250: Argument of %s must be a List, String, Dictionary or Blob"));
EXTERN char e_list_dict_blob_or_string_required_for_argument_nr[]
	INIT(= N_("E1251: List, Dictionary, Blob or String required for argument %d"));
EXTERN char e_string_list_or_blob_required_for_argument_nr[]
	INIT(= N_("E1252: String, List or Blob required for argument %d"));
EXTERN char e_string_expected_for_argument_nr[]
	INIT(= N_("E1253: String expected for argument %d"));
EXTERN char e_cannot_use_script_variable_in_for_loop[]
	INIT(= N_("E1254: Cannot use script variable in for loop"));
EXTERN char e_cmd_mapping_must_end_with_cr[]
	INIT(= N_("E1255: <Cmd> mapping must end with <CR>"));
EXTERN char e_string_or_function_required_for_argument_nr[]
	INIT(= N_("E1256: String or function required for argument %d"));
