// VIM_TEST_SETUP setlocal nofoldenable
// VIM_TEST_SETUP let g:java_mark_braces_in_parens_as_errors = 1

	@SuppressWarnings({
	"""
	bespoke
	/*
	 *
	 */
	/**
	 *
	 */
	//
	//
	//
	{
	}
"""
})
class UnfoldingTests {
	interface Unfoldenable
	{
	}

	static {
		new Object() {
			{
				{
					new Object() {{{
						new Object() {{{}}};
					}}};
				}
			}
		};

		switch (0) {
			case 0:
			case 1: {
				break;
			}
			default: ;
		};
	}

	{ Object bb = ((Object) new byte[]{}); }
	{
out: {
		do {
			if (true)
				break out;
		} while (false);
}
	}

	/**
	 * No operation.
	 */
	void noOp1() { }
	/** No operation. */
	void noOp2()
	{
	}
	/** No operation. */
	void noOp3() {
	}
	/** No operation. */
	void noOp4() {
	/*/\/\/\*/ ; }
}

/*
 * Some note.
 * {
 * }
 */
/**
 * A summary.
 * {
 * }
 */
//
// {
// }

/* 122|........................................................................................*/ interface Unfoldenable {
}
