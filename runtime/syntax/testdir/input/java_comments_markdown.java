// VIM_TEST_SETUP unlet! g:java_no_tab_space_error g:java_ignore_javadoc
// VIM_TEST_SETUP unlet! g:java_no_trail_space_error
// VIM_TEST_SETUP let[g:java_space_errors,g:java_comment_strings]=[1,1]
// VIM_TEST_SETUP let[g:java_ignore_html,g:markdown_syntax_conceal]=[1,1]
// VIM_TEST_SETUP let g:html_syntax_folding = 1
// VIM_TEST_SETUP let g:java_consent_to_html_syntax_folding = 1



// VIM_TEST_SETUP defer execute('match Visual /\%>21l\s\+$/')
// VIM_TEST_SETUP setl spell fdc=2 fdl=64 fdm=syntax fen cole=3 cocu=n
// VIM_TEST_SETUP highlight link javaCommentStart Todo
// VIM_TEST_SETUP highlight link javaMarkdownCommentTitle Underlined
// VIM_TEST_SETUP highlight link markdownH2 NonText
// VIM_TEST_SETUP highlight link markdownHeadingRule NonText





/**/ /*/ */ /* /*/ /*/*/ /*//*/ /// Markdown comment tests.
///
/// There is no entry point method `main`:
/// {@snippet file = MarkdownSnippets.java region = main id = _01}
///
/// There is no textual representation:
/// {@snippet class = MarkdownSnippets region = toString id = _02}
class MarkdownCommentsTests implements Comparable<MarkdownCommentsTests>
{	// JDK 23+.
	private MarkdownCommentsTests() { }

	/// No-op, i. e. no operation.
	/// ({@literal@literal} may be used with `.` for contraction.)
	/// @return `null`
	Void noOp1() { return null; }

	/// No-op, i.e. no operation.
	/// ({@literal<!-- -->} may be used after `.` for contraction.)
	/// @return `null`
	Void noOp2() { return null; }

	/// No-op, i.e\u002e no operation.
	/// ({@literal\u005cu002e} is processed early, use alternatives.)
	/// @return `null`
	Void noOp3() { return null; }

	/// No-op, i.e{@literal .} no operation.
	/// @return `null`
	Void noOp4() { return null; }

	/// No-op, i.e.<!-- --> no operation.
	/// @return `null`
	Void noOp5() { return null; }

	/// No-op, i.e.&nbsp;no operation.
	/// @return `null`
	Void noOp6() { return null; }

	/// {@return `null`, with no-op, i.e. no operation}
	Void noOp7() { return null; }

	/// {@return `null`, with no-op, i.e. no operation}..
	Void noOp8() { return null; }

	/// {@return `null`, with no-op, i.e. no operation} . .
	Void noOp9() { return null; }

	/// {@return the major Java version}
	/// @hidden
	protected int majorVersion() { return 23; }

	///    {@summary Compares this instance with the passed `that`
	/// instance for order by invoking [Integer.compare(int, int)]
	/// and passing it `this.majorVersion()` and
	/// `that.majorVersion()` as respective @arguments.}
	/// {@inheritDoc}
	@Override public int compareTo(MarkdownCommentsTests that)
	{
		java.util.Objects.requireNonNull(that, "that");
		return Integer.compare(this.majorVersion(),
						that.majorVersion());
	}

	/** Returns an empty string for an @Override annotated method
	* (see Chapter 9.6.4.4 {@literal @Override} in a Java Language
	* Specification) overridden from <code>java.lang.Object</code>
	*
	* @return an empty string */// No period for the above summary!
	private String asString() { return ""; }

	/// Returns an empty string for an @Override annotated method
	/// (see Chapter 9.6.4.4 {@literal @Override} in a Java Language
	/// Specification) overridden from `java.lang.Object`
	///
	/// @return an empty string /// No period for the above summary!
	@Override public String toString() { return asString(); }

	/// A summary comment.
	static final String MARKDOWN_COMMENT_A = "///";
	/// No summary comment (try fiddling with the above string value).
	static final String MARKDOWN_COMMENT_B = "///";
}

// javadoc --snippet-path . --source-path . -d /tmp/md_docs/ -package \
// 	-tag 'jls:a:See Java Language Specification:' MarkdownSnippets.java
/// Snippets for Markdown comment tests.
class MarkdownSnippets
{	/* 	TRAILING BLANKS AND MESSPILLINGS ARE SIGNIFICANT! */
	private MarkdownSnippets() { }

	/// The method [`main`] must be declared `public`,
	/// `static`, and `void`.  It must specify a formal parameter
	/// whose declared type is array of [String].  Therefore,
	/// _either_ of the following declarations is acceptable.  [^\*]
	///
	/// ---------------------------------------------------------------
	/// DECLARATIONS ([main(String\[\])][#main(String\[\])]):
	/// -----------------------------------------------------
	/// * E.g. {@snippet lang="java":
	/// // @highlight substring="main" type="italic":
	///    public static void main(String[] args) { }
	/// }
	/// + E.g.
	/// <pre class="snippet">
	///    public static void main(String... args) { }
	/// </pre>
	///
	/// @param args optional commande-line arguments 
	/// @jls 12.1.4 Invoke `Test.main`
	///
	/// [^\*]: @jls 12.1.4 Invoke `Test.main`
	// @start region = main		
	// @link substring = 'String' target = 'java.lang.String' :
	public static void main(String[] args) { }
	// @end 

	/// {@return an empty string}
	/// @see String [3.10.5 String Literals](https://docs.oracle.com/javase/specs/jls/se21/html/jls-3.html#jls-3.10.5)
	/// @see Object
	/// [4.3.2 The Class Object](https://docs.oracle.com/javase/specs/jls/se21/html/jls-4.html#jls-4.3.2)
	/// @see java.base/java.lang.Object#toString()
	// @start region = toString	
	// @replace substring = '""' replacement = "\u0022\u0022"
	// @link regex = '\bString' target = java.lang.String type = linkplain :
	@Override public String toString() { return ""; }
	// @end 
}
