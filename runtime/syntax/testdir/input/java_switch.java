class SwitchTests	// JDK 21+.
{
	static void echo(Object o) { System.out.println(o); }

	static {
		interface Yieldable<T>
		{
			T yield();
			default Yieldable<T> default_()	{ return this; }
			default Yieldable<T> when()	{ return this; }
		}

		// There are 80 bytes (\@80<!) between "::" and "yield;".
		Yieldable<?> y = ((Yieldable<?>) () -> 0)::
                                                                               yield;
		((Yieldable<?>) () -> 0).when().default_().yield();

		enum Letters { OTHER, ALPHA, BETA }

		Letters when = Letters.OTHER;

		switch (when) {
		case ALPHA:	{ echo(Letters.ALPHA); break; }
		case BETA:	{ echo(Letters.BETA); break; }
		default:	{ echo(Letters.OTHER); }
		}

		echo(switch (when) {
			case ALPHA	-> Letters.ALPHA;
			case BETA	-> Letters.BETA;
			default		-> { yield(Letters.OTHER); }
		});

		String yield = null;

		switch (yield) {
		case "A": case "B":	{ echo("A or B"); break; }
		case ":":		{ echo("Colon"); break; }
		case String str when !str.equals(""):
					{ echo("<non-empty>"); break; }
		case null: default:	{ echo("Other"); }
		}

		echo(switch (yield) {
			case "A", "B"		-> { yield("A or B"); }
			case "->"		-> "Arrow";
			case String str when !str.equals("")
						-> "<non-empty>";
			case null, default	-> "Other";
		});

		Object o = new Object();

		switch (o) {
		case null:		{ echo("null"); break; }
		case Letters[] ll:	{ echo("SwitchTests$1Letters[]"); break; }
		default:		{ echo("java.lang.Object"); break; }
		}

		echo(switch (o) {
			case null		-> "null";
			case Letters[] ll	-> "SwitchTests$1Letters[]";
			default			-> "java.lang.Object";
		});

		char ch = 'c';

		switch (ch) {
		case 'a':		{ echo('a'); break; }
		case 'b':		{ echo('b'); break; }
		default:		{ echo('\u0000'); break; }
		}

		echo(switch (ch) {
			case 'a'	-> 'a';
			case 'b'	-> 'b';
			default		-> '\u0000';
		});

		byte b = (byte) 2;

		switch (b) {
		case ((byte) 0):	{ echo((byte) 0); break; }
		case ((byte) 1):	{ echo((byte) 1); break; }
		default:		{ echo((byte) -1); break; }
		}

		echo(switch (b) {
			case ((byte) 0)	-> (byte) 0;
			case ((byte) 1)	-> (byte) 1;
			default		-> (byte) -1;
		});

		short sh = (short) 2;

		switch (sh) {
		case ((short) 0):	{ echo((short) 0); break; }
		case ((short) 1):	{ echo((short) 1); break; }
		default:		{ echo((short) -1); break; }
		}

		echo(switch (sh) {
			case ((short) 0)	-> (short) 0;
			case ((short) 1)	-> (short) 1;
			default			-> (short) -1;
		});

		int i = 2;

		switch (i) {
		case 0b0__00___000:	{ echo(0); break; }
		case 0x000___00__1:	{ echo(1); break; }
		default:		{ echo(-1); break; }
		}

		echo(switch (i) {
			case 0_0_0_0_0	-> 0;
			case 1		-> 1;
			default		-> -1;
		});
	}
}
