// VIM_TEST_SETUP unlet! g:java_ignore_javadoc g:java_no_trail_space_error
// VIM_TEST_SETUP unlet! g:java_no_tab_space_error
// VIM_TEST_SETUP let [g:java_space_errors,g:java_comment_strings] = [1,1]
// VIM_TEST_SETUP setlocal spell | highlight link javaCommentStart Todo




/**/ /*/ */ /* /*/ /*/*/ /*//*/ /** Comment tests.
 * <p>There is no entry point method {@code main}:
 * {@snippet file = Snippets.java region = main id = _01}
 * <p>There is no textual representation:
 * {@snippet class = Snippets region = toString id = _02} */
class CommentsTests
{
	/** No-op, i. e. no operation.
	* ({@literal@literal} may be used with {@code .} for contraction.)
	* @return {@code null} */
	Void noOp1() { return null; }

	/** No-op, i.e. no operation.
	* ({@literal<!-- -->} may be used after {@code .} for contraction.)
	* @return {@code null} */
	Void noOp2() { return null; }

	/** No-op, i.e\u002e no operation.
	* ({@literal\u005cu002e} is processed early, use alternatives.)
	* @return {@code null} */
	Void noOp3() { return null; }

	/** No-op, i.e{@literal .} no operation.
	* @return {@code null} */
	Void noOp4() { return null; }

	/** No-op, i.e.<!-- --> no operation.
	* @return {@code null} */
	Void noOp5() { return null; }

	/** No-op, i.e.&nbsp;no operation.
	* @return {@code null} */
	Void noOp6() { return null; }

	/** {@return {@code null}, with no-op, i.e. no operation} */
	Void noOp7() { return null; }

	/** {@return {@code null}, with no-op, i.e. no operation}.. */
	Void noOp8() { return null; }

	/** {@return {@code null}, with no-op, i.e. no operation} . . */
	Void noOp9() { return null; }

	/** Returns an empty string for an @Override annotated method
	* (see Chapter 9.6.4.4 {@literal @Override} in a Java Language
	* Specification) overridden from <code>java.lang.Object</code>
	*
	* @return an empty string */// No period for the above summary!
	@Override public String toString() { return ""; }
}

// javadoc --snippet-path . --source-path . -d /tmp/doc/ -package \
// 	-tag 'jls:a:See Java Language Specification:' Snippets.java
/** Snippets for comment tests. */
class Snippets
{	/* 	TRAILING BLANKS AND MESSPILLINGS ARE SIGNIFICANT! */
	/** The method {@code main} must be declared {@code public}, {@code
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
	 * @jls 12.1.4 Invoke {@code Test.main} */
	// @start region = main		
	// @link substring = 'String' target = 'java.lang.String' :
	public static void main(String[] args) { }
	// @end 

	/** {@return an empty string}	
	 * @see <a href="https://docs.oracle.com/javase/specs/jls/se21/html/jls-3.html#jls-3.10.5">3.10.5 String Literals</a>
	 * @see Object#toString() */
	// @start region = toString	
	// @replace substring = '""' replacement = "\u0022\u0022"
	// @link regex = '\bString' target = java.lang.String type = linkplain :
	@Override public String toString() { return ""; }
	// @end 
}
