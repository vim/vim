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

EXTERN char e_interrupted[]
	INIT(= N_("Interrupted"));

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
EXTERN char e_no_match_str_1[]
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
#ifdef FEAT_GUI
EXTERN char e_cannot_start_the_GUI[]
	INIT(= N_("E229: Cannot start the GUI"));
EXTERN char e_cannot_read_from_str[]
	INIT(= N_("E230: Cannot read from \"%s\""));
EXTERN char e_guifontwide_invalid[]
	INIT(= N_("E231: 'guifontwide' invalid"));
EXTERN char e_cannot_create_ballooneval_with_both_message_and_callback[]
	INIT(= N_("E232: Cannot create BalloonEval with both message and callback"));
# if defined(FEAT_GUI_GTK) || defined(FEAT_GUI_X11)
EXTERN char e_cannot_open_display[]
	INIT(= N_("E233: cannot open display"));
# endif
# if defined(FEAT_XFONTSET)
EXTERN char e_unknown_fontset_str[]
	INIT(= N_("E234: Unknown fontset: %s"));
# endif
# if defined(FEAT_GUI_X11) || defined(FEAT_GUI_GTK) \
	|| defined(FEAT_GUI_PHOTON) || defined(FEAT_GUI_MSWIN) || defined(FEAT_GUI_HAIKU)
EXTERN char e_unknown_font_str[]
	INIT(= N_("E235: Unknown font: %s"));
# endif
# if defined(FEAT_GUI_X11) && !defined(FEAT_GUI_GTK)
EXTERN char e_font_str_is_not_fixed_width[]
	INIT(= N_("E236: Font \"%s\" is not fixed-width"));
# endif
#endif
#ifdef MSWIN
EXTERN char e_printer_selection_failed[]
	INIT(= N_("E237: Printer selection failed"));
EXTERN char e_print_error_str[]
	INIT(= N_("E238: Print error: %s"));
#endif
EXTERN char e_invalid_sign_text_str[]
	INIT(= N_("E239: Invalid sign text: %s"));
#if defined(FEAT_CLIENTSERVER) && defined(FEAT_X11)
EXTERN char e_no_connection_to_x_server[]
	INIT(= N_("E240: No connection to the X server"));
#endif
#ifdef FEAT_CLIENTSERVER
EXTERN char e_unable_to_send_to_str[]
	INIT(= N_("E241: Unable to send to %s"));
#endif
EXTERN char e_cant_split_window_while_closing_another[]
	INIT(= N_("E242: Can't split a window while closing another"));
#if defined(FEAT_GUI_MSWIN) && !defined(FEAT_OLE)
EXTERN char e_argument_not_supported_str_use_ole_version[]
	INIT(= N_("E243: Argument not supported: \"-%s\"; Use the OLE version."));
#endif
#ifdef MSWIN
EXTERN char e_illegal_str_name_str_in_font_name_str[]
	INIT(= N_("E244: Illegal %s name \"%s\" in font name \"%s\""));
EXTERN char e_illegal_char_nr_in_font_name_str[]
	INIT(= N_("E245: Illegal char '%c' in font name \"%s\""));
#endif
EXTERN char e_filechangedshell_autocommand_deleted_buffer[]
	INIT(= N_("E246: FileChangedShell autocommand deleted buffer"));
#ifdef FEAT_CLIENTSERVER
EXTERN char e_no_registered_server_named_str[]
	INIT(= N_("E247: no registered server named \"%s\""));
EXTERN char e_failed_to_send_command_to_destination_program[]
	INIT(= N_("E248: Failed to send command to the destination program"));
#endif
EXTERN char e_window_layout_changed_unexpectedly[]
	INIT(= N_("E249: window layout changed unexpectedly"));
#ifdef FEAT_XFONTSET
EXTERN char e_fonts_for_the_following_charsets_are_missing_in_fontset[]
	INIT(= N_("E250: Fonts for the following charsets are missing in fontset %s:"));
#endif
#ifdef FEAT_CLIENTSERVER
EXTERN char e_vim_instance_registry_property_is_badly_formed_deleted[]
	INIT(= N_("E251: VIM instance registry property is badly formed.  Deleted!"));
#endif
#ifdef FEAT_GUI_X11
EXTERN char e_fontsent_name_str_font_str_is_not_fixed_width[]
	INIT(= N_("E252: Fontset name: %s - Font '%s' is not fixed-width"));
EXTERN char e_fontset_name_str[]
	INIT(= N_("E253: Fontset name: %s"));
#endif
#if defined(FEAT_GUI) || defined(FEAT_TERMGUICOLORS)
EXTERN char e_cannot_allocate_color_str[]
	INIT(= N_("E254: Cannot allocate color %s"));
#endif
#if defined(FEAT_SIGN_ICONS) && !defined(FEAT_GUI_GTK)
EXTERN char e_couldnt_read_in_sign_data[]
	INIT(= N_("E255: Couldn't read in sign data"));
#endif
// E256 unused
EXTERN char e_cstag_tag_not_founc[]
	INIT(= N_("E257: cstag: tag not found"));
#ifdef FEAT_CLIENTSERVER
EXTERN char e_unable_to_send_to_client[]
	INIT(= N_("E258: Unable to send to client"));
#endif
#ifdef FEAT_CSCOPE
EXTERN char e_no_matches_found_for_cscope_query_str_of_str[]
	INIT(= N_("E259: no matches found for cscope query %s of %s"));
#endif
#ifdef FEAT_EVAL
EXTERN char e_missing_name_after_method[]
	INIT(= N_("E260: Missing name after ->"));
#endif
#ifdef FEAT_CSCOPE
EXTERN char e_cscope_connection_str_not_founc[]
	INIT(= N_("E261: cscope connection %s not found"));
EXTERN char e_error_reading_cscope_connection_nr[]
	INIT(= N_("E262: error reading cscope connection %d"));
#endif
#if defined(DYNAMIC_PYTHON) || defined(DYNAMIC_PYTHON3)
EXTERN char e_sorry_this_command_is_disabled_python_library_could_not_be_found[]
	INIT(= N_("E263: Sorry, this command is disabled, the Python library could not be loaded."));
#endif
#if defined(FEAT_PYTHON) || defined(FEAT_PYTHON3)
EXTERN char e_python_error_initialising_io_object[]
	INIT(= N_("E264: Python: Error initialising I/O objects"));
#endif
#ifdef FEAT_RUBY
EXTERN char e_dollar_must_be_an_instance_of_string[]
	INIT(= N_("E265: $_ must be an instance of String"));
#endif
#ifdef DYNAMIC_RUBY
EXTERN char e_sorry_this_command_is_disabled_the_ruby_library_could_not_be_loaded[]
	INIT(= N_("E266: Sorry, this command is disabled, the Ruby library could not be loaded."));
#endif
#ifdef FEAT_RUBY
EXTERN char e_unexpected_return[]
	INIT(= N_("E267: unexpected return"));
EXTERN char e_unexpected_next[]
	INIT(= N_("E268: unexpected next"));
EXTERN char e_unexpected_break[]
	INIT(= N_("E269: unexpected break"));
EXTERN char e_unexpected_redo[]
	INIT(= N_("E270: unexpected redo"));
EXTERN char e_retry_outside_of_rescue_clause[]
	INIT(= N_("E271: retry outside of rescue clause"));
EXTERN char e_unhandled_exception[]
	INIT(= N_("E272: unhandled exception"));
EXTERN char e_unknown_longjmp_status_nr[]
	INIT(= N_("E273: unknown longjmp status %d"));
#endif
#ifdef FEAT_EVAL
EXTERN char e_no_white_space_allowed_before_parenthesis[]
	INIT(= N_("E274: No white space allowed before parenthesis"));
#endif
#ifdef FEAT_PROP_POPUP
EXTERN char e_cannot_add_text_property_to_unloaded_buffer[]
	INIT(= N_("E275: Cannot add text property to unloaded buffer"));
#endif
#ifdef FEAT_EVAL
EXTERN char e_cannot_use_function_as_method_str[]
	INIT(= N_("E276: Cannot use function as a method: %s"));
#endif
#ifdef FEAT_CLIENTSERVER
EXTERN char e_unable_to_read_server_reply[]
	INIT(= N_("E277: Unable to read a server reply"));
#endif
#ifdef FEAT_TERMINAL
EXTERN char e_sorry_plusplusshell_not_supported_on_this_system[]
	INIT(= N_("E279: Sorry, ++shell is not supported on this system"));
#endif
#ifdef FEAT_TCL
EXTERN char e_tcl_fatal_error_reflist_corrupt_please_report_this[]
	INIT(= N_("E280: TCL FATAL ERROR: reflist corrupt!? Please report this to vim-dev@vim.org"));
#endif
// E281 unused
EXTERN char e_cannot_read_from_str_2[]
	INIT(= N_("E282: Cannot read from \"%s\""));
EXTERN char e_no_marks_matching_str[]
	INIT(= N_("E283: No marks matching \"%s\""));
#ifdef FEAT_XIM
EXTERN char e_cannot_set_ic_values[]
	INIT(= N_("E284: Cannot set IC values"));
# if defined(FEAT_GUI_X11)
EXTERN char e_failed_to_create_input_context[]
	INIT(= N_("E285: Failed to create input context"));
# endif
EXTERN char e_failed_to_open_input_method[]
	INIT(= N_("E286: Failed to open input method"));
EXTERN char e_warning_could_not_set_destroy_callback_to_im[]
	INIT(= N_("E287: Warning: Could not set destroy callback to IM"));
EXTERN char e_input_method_doesnt_support_any_style[]
	INIT(= N_("E288: input method doesn't support any style"));
EXTERN char e_input_method_doesnt_support_my_preedit_type[]
	INIT(= N_("E289: input method doesn't support my preedit type"));
#endif
#ifdef FEAT_SEARCH_EXTRA
EXTERN char e_list_or_number_required[]
	INIT(= N_("E290: List or number required"));
#endif
// E291 unused
EXTERN char e_invalid_count_for_del_bytes_nr[]
	INIT(= N_("E292: Invalid count for del_bytes(): %ld"));
EXTERN char e_block_was_not_locked[]
	INIT(= N_("E293: block was not locked"));
EXTERN char e_seek_error_in_swap_file_read[]
	INIT(= N_("E294: Seek error in swap file read"));
EXTERN char e_read_error_in_swap_file[]
	INIT(= N_("E295: Read error in swap file"));
EXTERN char e_seek_error_in_swap_file_write[]
	INIT(= N_("E296: Seek error in swap file write"));
EXTERN char e_write_error_in_swap_file[]
	INIT(= N_("E297: Write error in swap file"));
EXTERN char e_didnt_get_block_nr_zero[]
	INIT(= N_("E298: Didn't get block nr 0?"));
EXTERN char e_didnt_get_block_nr_one[]
	INIT(= N_("E298: Didn't get block nr 1?"));
EXTERN char e_didnt_get_block_nr_two[]
	INIT(= N_("E298: Didn't get block nr 2?"));
// E299 unused
EXTERN char e_swap_file_already_exists_symlink_attack[]
	INIT(= N_("E300: Swap file already exists (symlink attack?)"));
EXTERN char e_oops_lost_the_swap_file[]
	INIT(= N_("E301: Oops, lost the swap file!!!"));
EXTERN char e_could_not_rename_swap_file[]
	INIT(= N_("E302: Could not rename swap file"));
EXTERN char e_unable_to_open_swap_file_for_str_recovery_impossible[]
	INIT(= N_("E303: Unable to open swap file for \"%s\", recovery impossible"));
EXTERN char e_ml_upd_block0_didnt_get_block_zero[]
	INIT(= N_("E304: ml_upd_block0(): Didn't get block 0??"));
EXTERN char e_no_swap_file_found_for_str[]
	INIT(= N_("E305: No swap file found for %s"));
EXTERN char e_cannot_open_str[]
	INIT(= N_("E306: Cannot open %s"));
EXTERN char e_str_does_not_look_like_vim_swap_file[]
	INIT(= N_("E307: %s does not look like a Vim swap file"));
EXTERN char e_warning_original_file_may_have_been_changed[]
	INIT(= N_("E308: Warning: Original file may have been changed"));
EXTERN char e_unable_to_read_block_one_from_str[]
	INIT(= N_("E309: Unable to read block 1 from %s"));
EXTERN char e_block_one_id_wrong_str_not_swp_file[]
	INIT(= N_("E310: Block 1 ID wrong (%s not a .swp file?)"));
EXTERN char e_recovery_interrupted[]
	INIT(= N_("E311: Recovery Interrupted"));
EXTERN char e_errors_detected_while_recovering_look_for_lines_starting_with_questions[]
	INIT(= N_("E312: Errors detected while recovering; look for lines starting with ???"));
EXTERN char e_cannot_preserve_there_is_no_swap_file[]
	INIT(= N_("E313: Cannot preserve, there is no swap file"));
EXTERN char e_preserve_failed[]
	INIT(= N_("E314: Preserve failed"));
EXTERN char e_ml_get_invalid_lnum_nr[]
	INIT(= N_("E315: ml_get: invalid lnum: %ld"));
EXTERN char e_ml_get_cannot_find_line_nr_in_buffer_nr_str[]
	INIT(= N_("E316: ml_get: cannot find line %ld in buffer %d %s"));
EXTERN char e_pointer_block_id_wrong[]
	INIT(= N_("E317: pointer block id wrong"));
EXTERN char e_pointer_block_id_wrong_two[]
	INIT(= N_("E317: pointer block id wrong 2"));
EXTERN char e_pointer_block_id_wrong_three[]
	INIT(= N_("E317: pointer block id wrong 3"));
EXTERN char e_pointer_block_id_wrong_four[]
	INIT(= N_("E317: pointer block id wrong 4"));
EXTERN char e_updated_too_many_blocks[]
	INIT(= N_("E318: Updated too many blocks?"));
EXTERN char e_sorry_command_is_not_available_in_this_version[]
	INIT(= N_("E319: Sorry, the command is not available in this version"));
EXTERN char e_cannot_find_line_nr[]
	INIT(= N_("E320: Cannot find line %ld"));
EXTERN char e_could_not_reload_str[]
	INIT(= N_("E321: Could not reload \"%s\""));
EXTERN char e_line_number_out_of_range_nr_past_the_end[]
	INIT(= N_("E322: line number out of range: %ld past the end"));
EXTERN char e_line_count_wrong_in_block_nr[]
	INIT(= N_("E323: line count wrong in block %ld"));
#ifdef FEAT_POSTSCRIPT
EXTERN char e_cant_open_postscript_output_file[]
	INIT(= N_("E324: Can't open PostScript output file"));
#endif
EXTERN char e_attention[]
	INIT(= N_("E325: ATTENTION"));
EXTERN char e_too_many_swap_files_found[]
	INIT(= N_("E326: Too many swap files found"));
EXTERN char_u e_part_of_menu_item_path_is_not_sub_menu[]
	INIT(= N_("E327: Part of menu-item path is not sub-menu"));
#ifdef FEAT_MENU
EXTERN char e_menu_only_exists_in_another_mode[]
	INIT(= N_("E328: Menu only exists in another mode"));
#endif
EXTERN char_u e_no_menu_str[]
	INIT(= N_("E329: No menu \"%s\""));
EXTERN char e_menu_path_must_not_loead_to_sub_menu[]
	INIT(= N_("E330: Menu path must not lead to a sub-menu"));
EXTERN char e_must_not_add_menu_items_directly_to_menu_bar[]
	INIT(= N_("E331: Must not add menu items directly to menu bar"));
EXTERN char e_separator_cannot_be_part_of_menu_path[]
	INIT(= N_("E332: Separator cannot be part of a menu path"));
EXTERN char e_menu_path_must_lead_to_menu_item[]
	INIT(= N_("E333: Menu path must lead to a menu item"));
EXTERN char e_menu_not_found_str[]
	INIT(= N_("E334: Menu not found: %s"));
EXTERN char e_menu_not_defined_for_str_mode[]
	INIT(= N_("E335: Menu not defined for %s mode"));
EXTERN char e_menu_path_must_lead_to_sub_menu[]
	INIT(= N_("E336: Menu path must lead to a sub-menu"));
EXTERN char e_menu_not_found_check_menu_names[]
	INIT(= N_("E337: Menu not found - check menu names"));
EXTERN char e_sorry_no_file_browser_in_console_mode[]
	INIT(= N_("E338: Sorry, no file browser in console mode"));
EXTERN char e_pattern_too_long[]
	INIT(= N_("E339: Pattern too long"));
// E340 unused
EXTERN char e_internal_error_lalloc_zero[]
	INIT(= N_("E341: Internal error: lalloc(0, )"));
EXTERN char e_out_of_memory_allocating_nr_bytes[]
	INIT(= N_("E342: Out of memory!  (allocating %lu bytes)"));
EXTERN char e_invalid_path_number_must_be_at_end_of_path_or_be_followed_by_str[]
	INIT(= N_("E343: Invalid path: '**[number]' must be at the end of the path or be followed by '%s'."));
EXTERN char e_cant_find_directory_str_in_cdpath[]
	INIT(= N_("E344: Can't find directory \"%s\" in cdpath"));
EXTERN char e_cant_find_file_str_in_path[]
	INIT(= N_("E345: Can't find file \"%s\" in path"));
EXTERN char e_no_more_directory_str_found_in_cdpath[]
	INIT(= N_("E346: No more directory \"%s\" found in cdpath"));
EXTERN char e_no_more_file_str_found_in_path[]
	INIT(= N_("E347: No more file \"%s\" found in path"));
EXTERN char e_no_string_under_cursor[]
	INIT(= N_("E348: No string under cursor"));
EXTERN char e_no_identifier_under_cursor[]
	INIT(= N_("E349: No identifier under cursor"));
EXTERN char e_cannot_create_fold_with_current_foldmethod[]
	INIT(= N_("E350: Cannot create fold with current 'foldmethod'"));
EXTERN char e_cannot_delete_fold_with_current_foldmethod[]
	INIT(= N_("E351: Cannot delete fold with current 'foldmethod'"));
EXTERN char e_cannot_erase_folds_with_current_foldmethod[]
	INIT(= N_("E352: Cannot erase folds with current 'foldmethod'"));
EXTERN char e_nothing_in_register_str[]
	INIT(= N_("E353: Nothing in register %s"));
EXTERN char e_invalid_register_name_str[]
	INIT(= N_("E354: Invalid register name: '%s'"));
EXTERN char e_unknown_option_str_2[]
	INIT(= N_("E355: Unknown option: %s"));
EXTERN char e_get_varp_error[]
	INIT(= N_("E356: get_varp ERROR"));
EXTERN char e_langmap_matching_character_missing_for_str[]
	INIT(= N_("E357: 'langmap': Matching character missing for %s"));
EXTERN char e_langmap_extra_characters_after_semicolon_str[]
	INIT(= N_("E358: 'langmap': Extra characters after semicolon: %s"));
#if defined(AMIGA) || defined(MACOS_X) || defined(MSWIN)  \
	|| defined(UNIX) || defined(VMS)
EXTERN char e_screen_mode_setting_not_supported[]
	INIT(= N_("E359: Screen mode setting not supported"));
#endif
EXTERN char e_cannot_execute_shell_with_f_option[]
	INIT(= N_("E360: Cannot execute shell with -f option"));
// E361 unused
EXTERN char e_using_boolean_valud_as_float[]
	INIT(= N_("E362: Using a boolean value as a Float"));
EXTERN char e_pattern_uses_more_memory_than_maxmempattern[]
	INIT(= N_("E363: pattern uses more memory than 'maxmempattern'"));
#ifdef FEAT_LIBCALL
EXTERN char e_library_call_failed_for_str[]
	INIT(= N_("E364: Library call failed for \"%s()\""));
#endif
#ifdef FEAT_POSTSCRIPT
EXTERN char e_failed_to_print_postscript_file[]
	INIT(= N_("E365: Failed to print PostScript file"));
#endif
EXTERN char e_not_allowed_to_enter_popup_window[]
	INIT(= N_("E366: Not allowed to enter a popup window"));
EXTERN char e_no_such_group_str[]
	INIT(= N_("E367: No such group: \"%s\""));
#ifdef FEAT_LIBCALL
EXTERN char e_got_sig_str_in_libcall[]
	INIT(= N_("E368: got SIG%s in libcall()"));
#endif
EXTERN char e_invalid_item_in_str_brackets[]
	INIT(= N_("E369: invalid item in %s%%[]"));
#ifdef USING_LOAD_LIBRARY
EXTERN char e_could_not_load_library_str_str[]
	INIT(= N_("E370: Could not load library %s: %s"));
#endif
#ifdef FEAT_GUI_MSWIN
EXTERN char e_command_not_found[]
	INIT(= N_("E371: Command not found"));
#endif
EXTERN char e_too_many_chr_in_format_string[]
	INIT(= N_("E372: Too many %%%c in format string"));
EXTERN char e_unexpected_chr_in_format_str[]
	INIT(= N_("E373: Unexpected %%%c in format string"));
EXTERN char e_missing_rsb_in_format_string[]
	INIT(= N_("E374: Missing ] in format string"));
EXTERN char e_unsupported_chr_in_format_string[]
	INIT(= N_("E375: Unsupported %%%c in format string"));
EXTERN char e_invalid_chr_in_format_string_prefix[]
	INIT(= N_("E376: Invalid %%%c in format string prefix"));
EXTERN char e_invalid_chr_in_format_string[]
	INIT(= N_("E377: Invalid %%%c in format string"));
EXTERN char e_errorformat_contains_no_pattern[]
	INIT(= N_("E378: 'errorformat' contains no pattern"));
EXTERN char e_missing_or_empty_directory_name[]
	INIT(= N_("E379: Missing or empty directory name"));
EXTERN char e_at_bottom_of_quickfix_stack[]
	INIT(= N_("E380: At bottom of quickfix stack"));
EXTERN char e_at_top_of_quickfix_stack[]
	INIT(= N_("E381: At top of quickfix stack"));
EXTERN char e_cannot_write_buftype_option_is_set[]
	INIT(= N_("E382: Cannot write, 'buftype' option is set"));
EXTERN char e_invalid_search_string_str[]
	INIT(= N_("E383: Invalid search string: %s"));
EXTERN char e_search_hit_top_without_match_for_str[]
	INIT(= N_("E384: search hit TOP without match for: %s"));
EXTERN char e_search_hit_bottom_without_match_for_str[]
	INIT(= N_("E385: search hit BOTTOM without match for: %s"));
EXTERN char e_expected_question_or_slash_after_semicolon[]
	INIT(= N_("E386: Expected '?' or '/'  after ';'"));
EXTERN char e_match_is_on_current_line[]
	INIT(= N_("E387: Match is on current line"));
EXTERN char e_couldnt_find_definition[]
	INIT(= N_("E388: Couldn't find definition"));
EXTERN char e_couldnt_find_pattern[]
	INIT(= N_("E389: Couldn't find pattern"));
EXTERN char e_illegal_argument_str_2[]
	INIT(= N_("E390: Illegal argument: %s"));
EXTERN char e_no_such_syntax_cluster_1[]
	INIT(= N_("E391: No such syntax cluster: %s"));
EXTERN char e_no_such_syntax_cluster_2[]
	INIT(= N_("E392: No such syntax cluster: %s"));
EXTERN char e_groupthere_not_accepted_here[]
	INIT(= N_("E393: group[t]here not accepted here"));
EXTERN char e_didnt_find_region_item_for_str[]
	INIT(= N_("E394: Didn't find region item for %s"));
EXTERN char e_contains_argument_not_accepted_here[]
	INIT(= N_("E395: contains argument not accepted here"));
// E396 unused
EXTERN char e_filename_required[]
	INIT(= N_("E397: Filename required"));
EXTERN char e_missing_equal_str[]
	INIT(= N_("E398: Missing '=': %s"));
EXTERN char e_not_enough_arguments_syntax_region_str[]
	INIT(= N_("E399: Not enough arguments: syntax region %s"));
EXTERN char e_no_cluster_specified[]
	INIT(= N_("E400: No cluster specified"));
EXTERN char e_pattern_delimiter_not_found_str[]
	INIT(= N_("E401: Pattern delimiter not found: %s"));
EXTERN char e_garbage_after_pattern_str[]
	INIT(= N_("E402: Garbage after pattern: %s"));
EXTERN char e_syntax_sync_line_continuations_pattern_specified_twice[]
	INIT(= N_("E403: syntax sync: line continuations pattern specified twice"));
EXTERN char e_illegal_arguments_str[]
	INIT(= N_("E404: Illegal arguments: %s"));
EXTERN char e_missing_equal_sign_str[]
	INIT(= N_("E405: Missing equal sign: %s"));
EXTERN char e_empty_argument_str[]
	INIT(= N_("E406: Empty argument: %s"));
EXTERN char e_str_not_allowed_here[]
	INIT(= N_("E407: %s not allowed here"));
EXTERN char e_str_must_be_first_in_contains_list[]
	INIT(= N_("E408: %s must be first in contains list"));
EXTERN char e_unknown_group_name_str[]
	INIT(= N_("E409: Unknown group name: %s"));
EXTERN char e_invalid_syntax_subcommand_str[]
	INIT(= N_("E410: Invalid :syntax subcommand: %s"));
EXTERN char e_highlight_group_name_not_found_str[]
	INIT(= N_("E411: highlight group not found: %s"));
EXTERN char e_not_enough_arguments_highlight_link_str[]
	INIT(= N_("E412: Not enough arguments: \":highlight link %s\""));
EXTERN char e_too_many_arguments_highlight_link_str[]
	INIT(= N_("E413: Too many arguments: \":highlight link %s\""));
EXTERN char e_group_has_settings_highlight_link_ignored[]
	INIT(= N_("E414: group has settings, highlight link ignored"));
EXTERN char e_unexpected_equal_sign_str[]
	INIT(= N_("E415: unexpected equal sign: %s"));
EXTERN char e_missing_equal_sign_str_2[]
	INIT(= N_("E416: missing equal sign: %s"));
EXTERN char e_missing_argument_str[]
	INIT(= N_("E417: missing argument: %s"));
EXTERN char e_illegal_value_str[]
	INIT(= N_("E418: Illegal value: %s"));
EXTERN char e_fg_color_unknown[]
	INIT(= N_("E419: FG color unknown"));
EXTERN char e_bg_color_unknown[]
	INIT(= N_("E420: BG color unknown"));
EXTERN char e_color_name_or_number_not_recognized[]
	INIT(= N_("E421: Color name or number not recognized: %s"));
EXTERN char e_terminal_code_too_long_str[]
	INIT(= N_("E422: terminal code too long: %s"));
EXTERN char e_illegal_argument_str_3[]
	INIT(= N_("E423: Illegal argument: %s"));
EXTERN char e_too_many_different_highlighting_attributes_in_use[]
	INIT(= N_("E424: Too many different highlighting attributes in use"));
EXTERN char e_cannot_go_before_first_matching_tag[]
	INIT(= N_("E425: Cannot go before first matching tag"));
EXTERN char e_tag_not_found_str[]
	INIT(= N_("E426: tag not found: %s"));
EXTERN char e_there_is_only_one_matching_tag[]
	INIT(= N_("E427: There is only one matching tag"));
EXTERN char e_cannot_go_beyond_last_matching_tag[]
	INIT(= N_("E428: Cannot go beyond last matching tag"));
EXTERN char e_file_str_does_not_exist[]
	INIT(= N_("E429: File \"%s\" does not exist"));
EXTERN char e_tag_file_path_truncated_for_str[]
	INIT(= N_("E430: Tag file path truncated for %s\n"));
EXTERN char e_format_error_in_tags_file_str[]
	INIT(= N_("E431: Format error in tags file \"%s\""));
EXTERN char e_tags_file_not_sorted_str[]
	INIT(= N_("E432: Tags file not sorted: %s"));
EXTERN char e_no_tags_file[]
	INIT(= N_("E433: No tags file"));
EXTERN char e_canot_find_tag_pattern[]
	INIT(= N_("E434: Can't find tag pattern"));
EXTERN char e_couldnt_find_tag_just_guessing[]
	INIT(= N_("E435: Couldn't find tag, just guessing!"));
EXTERN char e_no_str_entry_in_termcap[]
	INIT(= N_("E436: No \"%s\" entry in termcap"));
EXTERN char e_terminal_capability_cm_required[]
	INIT(= N_("E437: terminal capability \"cm\" required"));
EXTERN char e_u_undo_line_numbers_wrong[]
	INIT(= N_("E438: u_undo: line numbers wrong"));
EXTERN char e_undo_list_corrupt[]
	INIT(= N_("E439: undo list corrupt"));
EXTERN char e_undo_line_missing[]
	INIT(= N_("E440: undo line missing"));
EXTERN char e_there_is_no_preview_window[]
	INIT(= N_("E441: There is no preview window"));
EXTERN char e_cant_split_topleft_and_botright_at_the_same_time[]
	INIT(= N_("E442: Can't split topleft and botright at the same time"));
EXTERN char e_cannot_rotate_when_another_window_is_split[]
	INIT(= N_("E443: Cannot rotate when another window is split"));
EXTERN char e_cannot_close_last_window[]
	INIT(= N_("E444: Cannot close last window"));
EXTERN char e_other_window_contains_changes[]
	INIT(= N_("E445: Other window contains changes"));
EXTERN char e_no_file_name_under_cursor[]
	INIT(= N_("E446: No file name under cursor"));
EXTERN char e_cant_find_file_str_in_path_2[]
	INIT(= N_("E447: Can't find file \"%s\" in path"));
#ifdef USING_LOAD_LIBRARY
EXTERN char e_could_not_load_library_function_str[]
	INIT(= N_("E448: Could not load library function %s"));
#endif
#ifdef FEAT_CLIENTSERVER
EXTERN char e_invalid_expression_received[]
	INIT(= N_("E449: Invalid expression received"));
#endif
EXTERN char e_buffer_number_text_or_list_required[]
	INIT(= N_("E450: buffer number, text or a list required"));
EXTERN char e_expected_right_curly_str[]
	INIT(= N_("E451: Expected }: %s"));
#ifdef FEAT_EVAL
EXTERN char e_double_semicolon_in_list_of_variables[]
	INIT(= N_("E452: Double ; in list of variables"));
#endif
EXTERN char e_ul_color_unknown[]
	INIT(= N_("E453: UL color unknown"));
EXTERN char e_function_list_was_modified[]
	INIT(= N_("E454: function list was modified"));
#ifdef FEAT_POSTSCRIPT
EXTERN char e_error_writing_to_postscript_output_file[]
	INIT(= N_("E455: Error writing to PostScript output file"));
EXTERN char e_cant_open_file_str_2[]
	INIT(= N_("E456: Can't open file \"%s\""));
EXTERN char e_cant_find_postscript_resource_file_str_ps[]
	INIT(= N_("E456: Can't find PostScript resource file \"%s.ps\""));
EXTERN char e_cant_read_postscript_resource_file_str[]
	INIT(= N_("E457: Can't read PostScript resource file \"%s\""));
#endif
EXTERN char e_cannot_allocate_colormap_entry_some_colors_may_be_incorrect[]
	INIT(= N_("E458: Cannot allocate colormap entry, some colors may be incorrect"));
#if defined(UNIX) || defined(FEAT_SESSION)
EXTERN char e_cannot_go_back_to_previous_directory[]
	INIT(= N_("E459: Cannot go back to previous directory"));
#endif
EXTERN char e_entries_missing_in_mapset_dict_argument[]
	INIT(= N_("E460: entries missing in mapset() dict argument"));
#ifdef FEAT_EVAL
EXTERN char e_illegal_variable_name_str[]
	INIT(= N_("E461: Illegal variable name: %s"));
#endif
EXTERN char e_could_not_prepare_for_reloading_str[]
	INIT(= N_("E462: Could not prepare for reloading \"%s\""));
#ifdef FEAT_NETBEANS_INTG
EXTERN char e_region_is_guarded_cannot_modify[]
	INIT(= N_("E463: Region is guarded, cannot modify"));
#endif
EXTERN char e_ambiguous_use_of_user_defined_command[]
	INIT(= N_("E464: Ambiguous use of user-defined command"));
EXTERN char e_winsize_requires_two_number_arguments[]
	INIT(= N_("E465: :winsize requires two number arguments"));
EXTERN char e_winpos_requires_two_number_arguments[]
	INIT(= N_("E466: :winpos requires two number arguments"));
EXTERN char e_custom_completion_requires_function_argument[]
	INIT(= N_("E467: Custom completion requires a function argument"));
EXTERN char e_completion_argument_only_allowed_for_custom_completion[]
	INIT(= N_("E468: Completion argument only allowed for custom completion"));
EXTERN char e_invalid_cscopequickfix_flag_chr_for_chr[]
	INIT(= N_("E469: invalid cscopequickfix flag %c for %c"));
EXTERN char e_command_aborted[]
	INIT(= N_("E470: Command aborted"));
EXTERN char e_argument_required[]
	INIT(= N_("E471: Argument required"));
EXTERN char e_command_failed[]
	INIT(= N_("E472: Command failed"));
EXTERN char e_internal_error_in_regexp[]
	INIT(= N_("E473: Internal error in regexp"));
EXTERN char e_invalid_argument[]
	INIT(= N_("E474: Invalid argument"));
EXTERN char e_invalid_argument_str[]
	INIT(= N_("E475: Invalid argument: %s"));
EXTERN char e_invalid_value_for_argument_str[]
	INIT(= N_("E475: Invalid value for argument %s"));
EXTERN char e_invalid_value_for_argument_str_str[]
	INIT(= N_("E475: Invalid value for argument %s: %s"));
EXTERN char e_invalid_command[]
	INIT(= N_("E476: Invalid command"));
#ifdef FEAT_EVAL
EXTERN char e_invalid_command_str[]
	INIT(= N_("E476: Invalid command: %s"));
#endif
EXTERN char e_no_bang_allowed[]
	INIT(= N_("E477: No ! allowed"));
EXTERN char e_dont_panic[]
	INIT(= N_("E478: Don't panic!"));
EXTERN char e_no_match[]
	INIT(= N_("E479: No match"));
EXTERN char e_no_match_str_2[]
	INIT(= N_("E480: No match: %s"));
EXTERN char e_no_range_allowed[]
	INIT(= N_("E481: No range allowed"));
EXTERN char e_cant_create_file_str[]
	INIT(= N_("E482: Can't create file %s"));
EXTERN char e_cant_get_temp_file_name[]
	INIT(= N_("E483: Can't get temp file name"));
EXTERN char e_cant_open_file_str[]
	INIT(= N_("E484: Can't open file %s"));
EXTERN char e_cant_read_file_str[]
	INIT(= N_("E485: Can't read file %s"));
EXTERN char e_pattern_not_found[]
	INIT(= N_("E486: Pattern not found"));
EXTERN char e_pattern_not_found_str[]
	INIT(= N_("E486: Pattern not found: %s"));
EXTERN char e_argument_must_be_positive[]
	INIT(= N_("E487: Argument must be positive"));
EXTERN char e_trailing_characters[]
	INIT(= N_("E488: Trailing characters"));
EXTERN char e_trailing_characters_str[]
	INIT(= N_("E488: Trailing characters: %s"));
EXTERN char e_no_call_stack_to_substitute_for_stack[]
	INIT(= N_("E489: no call stack to substitute for \"<stack>\""));
#ifdef FEAT_FOLDING
EXTERN char e_no_fold_found[]
	INIT(= N_("E490: No fold found"));
#endif
#ifdef FEAT_EVAL
EXTERN char e_json_decode_error_at_str[]
	INIT(= N_("E491: json decode error at '%s'"));
#endif
EXTERN char e_not_an_editor_command[]
	INIT(= N_("E492: Not an editor command"));
EXTERN char e_backwards_range_given[]
	INIT(= N_("E493: Backwards range given"));
EXTERN char e_use_w_or_w_gt_gt[]
	INIT(= N_("E494: Use w or w>>"));
EXTERN char e_no_autocommand_file_name_to_substitute_for_afile[]
	INIT(= N_("E495: no autocommand file name to substitute for \"<afile>\""));
EXTERN char e_no_autocommand_buffer_name_to_substitute_for_abuf[]
	INIT(= N_("E496: no autocommand buffer number to substitute for \"<abuf>\""));
EXTERN char e_no_autocommand_match_name_to_substitute_for_amatch[]
	INIT(= N_("E497: no autocommand match name to substitute for \"<amatch>\""));
EXTERN char e_no_source_file_name_to_substitute_for_sfile[]
	INIT(= N_("E498: no :source file name to substitute for \"<sfile>\""));
EXTERN char e_empty_file_name_for_percent_or_hash_only_works_with_ph[]
	INIT(= N_("E499: Empty file name for '%' or '#', only works with \":p:h\""));
EXTERN char e_evaluates_to_an_empty_string[]
	INIT(= N_("E500: Evaluates to an empty string"));
EXTERN char e_at_end_of_file[]
	INIT(= N_("E501: At end-of-file"));
	// E502
EXTERN char e_is_a_directory[]
	INIT(= N_("is a directory"));
	// E503
EXTERN char e_is_not_file_or_writable_device[]
	INIT(= N_("is not a file or writable device"));
EXTERN char e_str_is_not_file_or_writable_device[]
	INIT(= N_("E503: \"%s\" is not a file or writable device"));
	// E504
EXTERN char e_is_read_only_cannot_override_W_in_cpoptions[]
	INIT(= N_("is read-only (cannot override: \"W\" in 'cpoptions')"));
	// E505
EXTERN char e_is_read_only_add_bang_to_override[]
	INIT(= N_("is read-only (add ! to override)"));
EXTERN char e_str_is_read_only_add_bang_to_override[]
	INIT(= N_("E505: \"%s\" is read-only (add ! to override)"));
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
#ifdef FEAT_NETBEANS_INTG
EXTERN char e_netbeans_already_connected[]
	INIT(= N_("E511: netbeans already connected"));
#endif
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
EXTERN char e_unknown_option[]
	INIT(= N_("E518: Unknown option"));
EXTERN char e_option_not_supported[]
	INIT(= N_("E519: Option not supported"));
EXTERN char e_not_allowed_in_modeline[]
	INIT(= N_("E520: Not allowed in a modeline"));
EXTERN char e_number_required_after_equal[]
	INIT(= N_("E521: Number required after ="));
EXTERN char e_number_required_after_str_equal_str[]
	INIT(= N_("E521: Number required: &%s = '%s'"));
EXTERN char e_not_found_in_termcap[]
	INIT(= N_("E522: Not found in termcap"));
EXTERN char e_not_allowed_here[]
	INIT(= N_("E523: Not allowed here"));
EXTERN char e_missing_colon[]
	INIT(= N_("E524: Missing colon"));
EXTERN char e_zero_length_string[]
	INIT(= N_("E525: Zero length string"));
EXTERN char e_missing_number_after_angle_str_angle[]
	INIT(= N_("E526: Missing number after <%s>"));
EXTERN char e_missing_comma[]
	INIT(= N_("E527: Missing comma"));
EXTERN char e_must_specify_a_value[]
	INIT(= N_("E528: Must specify a ' value"));
EXTERN char e_cannot_set_term_to_empty_string[]
	INIT(= N_("E529: Cannot set 'term' to empty string"));
EXTERN char e_cannot_change_term_in_GUI[]
	INIT(= N_("E530: Cannot change 'term' in the GUI"));
EXTERN char e_use_gui_to_start_GUI[]
	INIT(= N_("E531: Use \":gui\" to start the GUI"));
#ifdef FEAT_NETBEANS_INTG
EXTERN char e_highlighting_color_name_too_long_in_defineAnnoType[]
	INIT(= N_("E532: highlighting color name too long in defineAnnoType"));
#endif
EXTERN char e_cant_select_wide_font[]
	INIT(= N_("E533: can't select wide font"));
EXTERN char e_invalid_wide_font[]
	INIT(= N_("E534: Invalid wide font"));
EXTERN char e_illegal_character_after_chr[]
	INIT(= N_("E535: Illegal character after <%c>"));
EXTERN char e_comma_required[]
	INIT(= N_("E536: comma required"));
EXTERN char e_commentstring_must_be_empty_or_contain_str[]
	INIT(= N_("E537: 'commentstring' must be empty or contain %s"));
// E538 unused (perhaps 538.nl ?)
EXTERN char e_illegal_character_str[]
	INIT(= N_("E539: Illegal character <%s>"));
EXTERN char e_unclosed_expression_sequence[]
	INIT(= N_("E540: Unclosed expression sequence"));
// E541 unused
EXTERN char e_unbalanced_groups[]
	INIT(= N_("E542: unbalanced groups"));
#ifdef MSWIN
EXTERN char e_not_valid_codepage[]
	INIT(= N_("E543: Not a valid codepage"));
#endif
EXTERN char e_keymap_file_not_found[]
	INIT(= N_("E544: Keymap file not found"));
#ifdef CURSOR_SHAPE
EXTERN char e_missing_colon_2[]
	INIT(= N_("E545: Missing colon"));
EXTERN char e_illegal_mode[]
	INIT(= N_("E546: Illegal mode"));
#endif
#ifdef FEAT_MOUSESHAPE
EXTERN char e_illegal_mouseshape[]
	INIT(= N_("E547: Illegal mouseshape"));
#endif
EXTERN char e_digit_expected[]
	INIT(= N_("E548: digit expected"));
EXTERN char e_illegal_percentage[]
	INIT(= N_("E549: Illegal percentage"));
EXTERN char e_missing_colon_3[]
	INIT(= N_("E550: Missing colon"));
EXTERN char e_illegal_component[]
	INIT(= N_("E551: Illegal component"));
EXTERN char e_digit_expected_2[]
	INIT(= N_("E552: digit expected"));
EXTERN char e_no_more_items[]
	INIT(= N_("E553: No more items"));
EXTERN char e_syntax_error_in_str_curlies[]
	INIT(= N_("E554: Syntax error in %s{...}"));
EXTERN char e_at_bottom_of_tag_stack[]
	INIT(= N_("E555: at bottom of tag stack"));
EXTERN char e_at_top_of_tag_stack[]
	INIT(= N_("E556: at top of tag stack"));
EXTERN char e_cannot_open_termcap_file[]
	INIT(= N_("E557: Cannot open termcap file"));
EXTERN char e_terminal_entry_not_found_in_terminfo[]
	INIT(= N_("E558: Terminal entry not found in terminfo"));
EXTERN char e_terminal_entry_not_found_in_termcap[]
	INIT(= N_("E559: Terminal entry not found in termcap"));
EXTERN char e_usage_cscope_str[]
	INIT(= N_("E560: Usage: cs[cope] %s"));
EXTERN char e_unknown_cscope_search_type[]
	INIT(= N_("E561: unknown cscope search type"));
EXTERN char e_usage_cstag_ident[]
	INIT(= N_("E562: Usage: cstag <ident>"));
EXTERN char e_stat_str_error_nr[]
	INIT(= N_("E563: stat(%s) error: %d"));
EXTERN char e_str_is_not_directory_or_valid_cscope_database[]
	INIT(= N_("E564: %s is not a directory or a valid cscope database"));
EXTERN char e_not_allowed_to_change_text_or_change_window[]
	INIT(= N_("E565: Not allowed to change text or change window"));
EXTERN char e_could_not_create_cscope_pipes[]
	INIT(= N_("E566: Could not create cscope pipes"));
EXTERN char e_no_cscope_connections[]
	INIT(= N_("E567: no cscope connections"));
EXTERN char e_duplicate_cscope_database_not_added[]
	INIT(= N_("E568: duplicate cscope database not added"));
// E569 unused
EXTERN char e_fatal_error_in_cs_manage_matches[]
	INIT(= N_("E570: fatal error in cs_manage_matches"));
#ifdef DYNAMIC_TCL
EXTERN char e_sorry_this_command_is_disabled_tcl_library_could_not_be_loaded[]
	INIT(= N_("E571: Sorry, this command is disabled: the Tcl library could not be loaded."));
#endif
EXTERN char e_exit_code_nr[]
	INIT(= N_("E572: exit code %d"));
EXTERN char e_invalid_server_id_used_str[]
	INIT(= N_("E573: Invalid server id used: %s"));
EXTERN char e_unknown_register_type_nr[]
	INIT(= N_("E574: Unknown register type %d"));
	// E575
EXTERN char e_illegal_starting_char[]
	INIT(= N_("Illegal starting char"));
	// E576
EXTERN char e_nonr_missing_gt[]
	INIT(= N_("Missing '>'"));
	// E577
EXTERN char e_illegal_register_name[]
	INIT(= N_("Illegal register name"));
EXTERN char e_not_allowed_to_change_text_here[]
	INIT(= N_("E578: Not allowed to change text here"));
#ifdef FEAT_EVAL
EXTERN char e_if_nesting_too_deep[]
	INIT(= N_("E579: :if nesting too deep"));
EXTERN char e_block_nesting_too_deep[]
	INIT(= N_("E579: block nesting too deep"));
EXTERN char e_endif_without_if[]
	INIT(= N_("E580: :endif without :if"));
EXTERN char e_else_without_if[]
	INIT(= N_("E581: :else without :if"));
EXTERN char e_elseif_without_if[]
	INIT(= N_("E582: :elseif without :if"));
EXTERN char e_multiple_else[]
	INIT(= N_("E583: multiple :else"));
EXTERN char e_elseif_after_else[]
	INIT(= N_("E584: :elseif after :else"));
EXTERN char e_while_for_nesting_too_deep[]
	INIT(= N_("E585: :while/:for nesting too deep"));
EXTERN char e_continue_without_while_or_for[]
	INIT(= N_("E586: :continue without :while or :for"));
EXTERN char e_break_without_while_or_for[]
	INIT(= N_("E587: :break without :while or :for"));
EXTERN char e_endwhile_without_while[]
	INIT(= N_("E588: :endwhile without :while"));
EXTERN char e_endfor_without_for[]
	INIT(= N_("E588: :endfor without :for"));
#endif
EXTERN char e_backupext_and_patchmode_are_equal[]
	INIT(= N_("E589: 'backupext' and 'patchmode' are equal"));
EXTERN char e_preview_window_already_exists[]
	INIT(= N_("E590: A preview window already exists"));
EXTERN char e_winheight_cannot_be_smaller_than_winminheight[]
	INIT(= N_("E591: 'winheight' cannot be smaller than 'winminheight'"));
EXTERN char e_winwidth_cannot_be_smaller_than_winminwidth[]
	INIT(= N_("E592: 'winwidth' cannot be smaller than 'winminwidth'"));
EXTERN char e_need_at_least_nr_lines[]
	INIT(= N_("E593: Need at least %d lines"));
EXTERN char e_need_at_least_nr_columns[]
	INIT(= N_("E594: Need at least %d columns"));
EXTERN char e_showbreak_contains_unprintable_or_wide_character[]
	INIT(= N_("E595: 'showbreak' contains unprintable or wide character"));
EXTERN char e_invalid_fonts[]
	INIT(= N_("E596: Invalid font(s)"));
EXTERN char e_cant_select_fontset[]
	INIT(= N_("E597: can't select fontset"));
EXTERN char e_invalid_fontset[]
	INIT(= N_("E598: Invalid fontset"));
EXTERN char e_value_of_imactivatekey_is_invalid[]
	INIT(= N_("E599: Value of 'imactivatekey' is invalid"));
#ifdef FEAT_EVAL
EXTERN char e_missing_endtry[]
	INIT(= N_("E600: Missing :endtry"));

EXTERN char e_endtry_without_try[]
	INIT(= N_("E602: :endtry without :try"));
EXTERN char e_catch_without_try[]
	INIT(= N_("E603: :catch without :try"));
EXTERN char e_finally_without_try[]
	INIT(= N_("E606: :finally without :try"));
EXTERN char e_multiple_finally[]
	INIT(= N_("E607: multiple :finally"));
#endif

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
#ifdef HAVE_FSYNC
EXTERN char e_fsync_failed[]
	INIT(= N_("E667: Fsync failed"));
#endif
EXTERN char e_no_matching_autocommands_for_acwrite_buffer[]
	INIT(= N_("E676: No matching autocommands for acwrite buffer"));
EXTERN char e_buffer_nr_invalid_buffer_number[]
	INIT(= N_("E680: <buffer=%d>: invalid buffer number"));
EXTERN char e_invalid_search_pattern_or_delimiter[]
	INIT(= N_("E682: Invalid search pattern or delimiter"));
#ifdef FEAT_EVAL
EXTERN char e_list_index_out_of_range_nr[]
	INIT(= N_("E684: list index out of range: %ld"));
#endif
EXTERN char e_internal_error_str[]
	INIT(= N_("E685: Internal error: %s"));
#ifdef FEAT_EVAL
EXTERN char e_argument_of_str_must_be_list[]
	INIT(= N_("E686: Argument of %s must be a List"));
EXTERN char e_missing_in_after_for[]
	INIT(= N_("E690: Missing \"in\" after :for"));
// E693 unused
EXTERN char e_cannot_index_a_funcref[]
	INIT(= N_("E695: Cannot index a Funcref"));
EXTERN char e_missing_end_of_list_rsb_str[]
	INIT(= N_("E697: Missing end of List ']': %s"));

// E706 unused
EXTERN char e_list_value_has_more_items_than_targets[]
	INIT(= N_("E710: List value has more items than targets"));
EXTERN char e_list_value_does_not_have_enough_items[]
	INIT(= N_("E711: List value does not have enough items"));
EXTERN char e_argument_of_str_must_be_list_or_dictionary[]
	INIT(= N_("E712: Argument of %s must be a List or Dictionary"));
EXTERN char e_cannot_use_empty_key_for_dictionary[]
	INIT(= N_("E713: Cannot use empty key for Dictionary"));
EXTERN char e_list_required[]
	INIT(= N_("E714: List required"));
EXTERN char e_dictionary_required[]
	INIT(= N_("E715: Dictionary required"));
EXTERN char e_key_not_present_in_dictionary[]
	INIT(= N_("E716: Key not present in Dictionary: \"%s\""));
EXTERN char e_cannot_slice_dictionary[]
	INIT(= N_("E719: Cannot slice a Dictionary"));
EXTERN char e_missing_colon_in_dictionary[]
	INIT(= N_("E720: Missing colon in Dictionary: %s"));
EXTERN char e_duplicate_key_in_dicitonary[]
	INIT(= N_("E721: Duplicate key in Dictionary: \"%s\""));
EXTERN char e_missing_comma_in_dictionary[]
	INIT(= N_("E722: Missing comma in Dictionary: %s"));
EXTERN char e_missing_dict_end[]
	INIT(= N_("E723: Missing end of Dictionary '}': %s"));
EXTERN char e_wrong_variable_type_for_str_equal[]
	INIT(= N_("E734: Wrong variable type for %s="));
EXTERN char e_value_is_locked[]
	INIT(= N_("E741: Value is locked"));
EXTERN char e_value_is_locked_str[]
	INIT(= N_("E741: Value is locked: %s"));
EXTERN char e_cannot_change_value[]
	INIT(= N_("E742: Cannot change value"));
EXTERN char e_cannot_change_value_of_str[]
	INIT(= N_("E742: Cannot change value of %s"));
#endif
#ifdef FEAT_NETBEANS_INTG
EXTERN char e_netbeans_does_not_allow_changes_in_read_only_files[]
	INIT(= N_("E744: NetBeans does not allow changes in read-only files"));
#endif
EXTERN char e_empty_buffer[]
	INIT(= N_("E749: empty buffer"));
#ifdef FEAT_SPELL
EXTERN char e_spell_checking_is_not_possible[]
	INIT(= N_("E756: Spell checking is not possible"));
#endif
#if defined(FEAT_SYN_HL) || defined(FEAT_COMPL_FUNC)
EXTERN char e_option_str_is_not_set[]
	INIT(= N_("E764: Option '%s' is not set"));
#endif
#ifdef FEAT_QUICKFIX
EXTERN char e_no_location_list[]
	INIT(= N_("E776: No location list"));
#endif
#ifdef FEAT_EVAL
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

#ifndef FEAT_ARABIC
EXTERN char e_arabic_cannot_be_used_not_enabled_at_compile_time[]
	INIT(= N_("E800: Arabic cannot be used: Not enabled at compile time\n"));
#endif
#ifdef FEAT_EVAL
EXTERN char e_cannot_use_percent_with_float[]
	INIT(= N_("E804: Cannot use '%' with Float"));
#endif
#ifdef FEAT_FLOAT
EXTERN char e_using_float_as_string[]
	INIT(= N_("E806: using Float as a String"));
#endif
EXTERN char e_cannot_close_autocmd_or_popup_window[]
	INIT(= N_("E813: Cannot close autocmd or popup window"));
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
// E839 unused
#ifndef FEAT_CLIPBOARD
EXTERN char e_invalid_register_name[]
	INIT(= N_("E850: Invalid register name"));
#endif
EXTERN char e_autocommands_caused_command_to_abort[]
	INIT(= N_("E855: Autocommands caused command to abort"));
#ifdef FEAT_EVAL
EXTERN char e_assert_fails_second_arg[]
	INIT(= N_("E856: \"assert_fails()\" second argument must be a string or a list with one or two strings"));
EXTERN char e_dictionary_key_str_required[]
	INIT(= N_("E857: Dictionary key \"%s\" required"));
#endif
#ifdef FEAT_PROP_POPUP
EXTERN char e_number_required[]
	INIT(= N_("E889: Number required"));
#endif
#ifdef FEAT_EVAL
EXTERN char e_argument_of_str_must_be_list_dictionary_or_blob[]
	INIT(= N_("E896: Argument of %s must be a List, Dictionary or Blob"));
EXTERN char e_list_or_blob_required[]
	INIT(= N_("E897: List or Blob required"));
#endif

#ifdef FEAT_EVAL
EXTERN char e_using_invalid_value_as_string_str[]
	INIT(= N_("E908: using an invalid value as a String: %s"));
EXTERN char e_cannot_index_special_variable[]
	INIT(= N_("E909: Cannot index a special variable"));
#endif
EXTERN char e_directory_not_found_in_str_str[]
	INIT(= N_("E919: Directory not found in '%s': \"%s\""));
#ifdef FEAT_EVAL
EXTERN char e_string_required[]
	INIT(= N_("E928: String required"));
#endif
EXTERN char e_buffer_cannot_be_registered[]
	INIT(= N_("E931: Buffer cannot be registered"));
#ifdef FEAT_EVAL
EXTERN char e_function_was_deleted_str[]
	INIT(= N_("E933: Function was deleted: %s"));
#endif
EXTERN char e_cannot_delete_current_group[]
	INIT(= N_("E936: Cannot delete the current group"));
EXTERN char e_attempt_to_delete_buffer_that_is_in_use_str[]
	INIT(= N_("E937: Attempt to delete a buffer that is in use: %s"));
EXTERN char e_positive_count_required[]
	INIT(= N_("E939: Positive count required"));
#ifdef FEAT_EVAL
EXTERN char e_cannot_lock_or_unlock_variable_str[]
	INIT(= N_("E940: Cannot lock or unlock variable %s"));
#endif
#ifdef FEAT_TERMINAL
EXTERN char e_job_still_running[]
	INIT(= N_("E948: Job still running"));
EXTERN char e_job_still_running_add_bang_to_end_the_job[]
	INIT(= N_("E948: Job still running (add ! to end the job)"));
#endif
EXTERN char e_file_changed_while_writing[]
	INIT(= N_("E949: File changed while writing"));
EXTERN char e_autocommand_caused_recursive_behavior[]
	INIT(= N_("E952: Autocommand caused recursive behavior"));
EXTERN char e_invalid_window_number[]
	INIT(= N_("E957: Invalid window number"));
EXTERN char_u e_invalid_column_number_nr[]
	INIT(= N_("E964: Invalid column number: %ld"));
EXTERN char_u e_invalid_line_number_nr[]
	INIT(= N_("E966: Invalid line number: %ld"));
EXTERN char e_blob_value_does_not_have_right_number_of_bytes[]
	INIT(= N_("E972: Blob value does not have the right number of bytes"));
#ifdef FEAT_EVAL
EXTERN char e_invalid_operation_for_blob[]
	INIT(= N_("E978: Invalid operation for Blob"));
EXTERN char e_blob_index_out_of_range_nr[]
	INIT(= N_("E979: Blob index out of range: %ld"));
#endif
EXTERN char e_duplicate_argument_str[]
	INIT(= N_("E983: Duplicate argument: %s"));
#ifdef FEAT_EVAL
EXTERN char e_cannot_modify_existing_variable[]
	INIT(= N_("E995: Cannot modify existing variable"));
EXTERN char e_cannot_lock_an_option[]
	INIT(= N_("E996: Cannot lock an option"));
EXTERN char e_reduce_of_an_empty_str_with_no_initial_value[]
	INIT(= N_("E998: Reduce of an empty %s with no initial value"));
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
