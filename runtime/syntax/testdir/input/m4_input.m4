dnl VIM_TEST_SETUP hi def link m4Disabled m4Comment

########################################
## <summary>
##	Dontaudit search ssh home directory
## </summary>
## <param name="domain">
##	<summary>
##	Domain to not audit.
##	</summary>
## </param>
#
interface(`ssh_dontaudit_search_user_home_dir',`
	gen_require(`
		type ssh_home_t;
	')

	dontaudit $1 ssh_home_t:dir search_dir_perms;
')
