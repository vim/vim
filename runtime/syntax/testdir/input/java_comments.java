// VIM_TEST_SETUP unlet! g:java_ignore_javadoc g:java_no_trail_space_error 
// VIM_TEST_SETUP unlet! g:java_no_tab_space_error	
// VIM_TEST_SETUP let [g:java_space_errors,g:java_comment_strings] = [1,1]
// 	VIM_TEST_SETUP setlocal spell
class CommentsTests
{	/* 	TRAILING BLANKS AND MESSPILLINGS ARE SIGNIFICANT! */ 	
	/**	
	 * The method {@code main} must be declared {@code public}, {@code
	 * static}, and {@code void}.  It must specify a formal parameter
	 * whose declared type is array of {@link String}.  Therefore,
	 * either of the following declarations is acceptable:
	 * 	{@snippet lang="java":
	 * // @highlight substring="main" type="italic":
	 * public static void main(String[] args) { }
	 * }<br /><pre class="snippet">
	 *{@code public static void main(String... args) { }}</pre>
	 *
	 * @param args optional commande-line arguments 
	 * @jls 12.1.4 Invoke {@code Test.main}
	 */
	// @start region = main
	// @link substring = 'String' target = 'java.lang.String' :
	public static void main(String[] args) { }
	// @end
}
