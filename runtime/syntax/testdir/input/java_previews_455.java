// VIM_TEST_SETUP let g:java_syntax_previews = [455]



class PrimitiveSwitchTests	// JDK 23+ (--enable-preview --release 23).
{
	static void echo(Object o) { System.out.println(o); }

	static {
		long g = 2L;

		switch (g) {
		case 0L:		{ echo(0L); break; }
		case 1L:		{ echo(1L); break; }
		default:		{ echo(-1L); break; }
		}

		echo(switch (g) {
			case 0L		-> 0L;
			case 1L		-> 1L;
			default		-> -1L;
		});

		boolean bool = false;

		switch (bool) {
		case true:		{ echo(true); break; }
		case false:		{ echo(false); break; }
		}

		echo(switch (bool) {
			case true	-> true;
			case false	-> false;
		});

		float f = 2.0f;

		switch (f) {
		case 0.0f:		{ echo(0.0f); break; }
		case 1.0f:		{ echo(1.0f); break; }
		default:		{ echo(-1.0f); break; }
		}

		echo(switch (f) {
			case 0.0f	-> 0.0f;
			case 1.0f	-> 1.0f;
			default		-> -1.0f;
		});

		double d = 2.0;

		switch (d) {
		case 0.0:		{ echo(0.0); break; }
		case 1.0:		{ echo(1.0); break; }
		default:		{ echo(-1.0); break; }
		}

		echo(switch (d) {
			case 0.0	-> 0.0;
			case 1.0	-> 1.0;
			default		-> -1.0;
		});
	}
}
