// VIM_TEST_SETUP setlocal foldenable foldcolumn=2 foldmethod=syntax


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
class FoldingTests {
	interface Foldenable
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
		}
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
/*\\\*/	{
		(new java.util.function.Function<Object, Object>() {
			/**
			 * {@inheritDoc} */
			public Object apply(Object o) { return o; };
		}).apply(
		(new java.util.function.Function<Object, Object>() {
			/** {@inheritDoc}
			 */
			public Object apply(Object o) { return o; };
		}));
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

/* 122|..........................................................................................*/ interface Foldenable {
}
