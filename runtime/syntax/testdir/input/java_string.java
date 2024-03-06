class StringTests	// JDK 21+ (--enable-preview --release 21).
{
	static {
		String s1 = "A quick brown fox jumps over the lazy dog";
		String s2 = "\"Woof\s!\"";
		String s3 = """
			A\s\
			quick \
			brown\s\
			fox \
			jumps\s\
			over \
			the\s\
			lazy \
			dog""";
		String s4 = """
			"Woof\s!\"""";
		String s5 = """
		String s3 = \"""
			A\\s\\
			quick \\
			brown\\s\\
			fox \\
			jumps\\s\\
			over \\
			the\\s\\
			lazy \\
			dog\""";""";

		// There are SPACE, FF, HT, CR, and LF after """.
		String empty = """ 	
			""";

		System.out.println("""
		"
		""
		""\u005c"
		""\u005c""
		""\"\u0022\u0022
		""\"""\u005c\u0022
		""\"""\""
		""\"""\""\"""");

		System.out.println(STR."""
		"
		\{"\"\""}
		\{"\"\""}\{"\u005c\u0022"}
		\{"\"\""}\{"\u005c\u0022"}"
		\{"\"\""}\"\{"\u005c\u0022\u005c\u0022"}
		\{"\"\""}\"\{"\"\""}\{"\u005c\u0022"}
		\{"\"\""}\"\{"\"\""}\""
		\{"\"\""}\"\{"\"\""}\""\"""");		// JDK 21+.

		String woof = "Woof", dog = "dog", fox = "fox";

		String s6 = STR
			."A quick brown \{fox} jumps over the lazy \{dog}";
		String s7 = STR.process(StringTemplate.RAW
			."\"\{woof}\s!\"");
		String s8 = STR."""
			A\s\
			quick \
			brown\s\
			\{fox} \
			jumps\s\
			over \
			the\s\
			lazy \
			\{dog}""";
		String s9 = STR.process(StringTemplate.RAW
			.
			"""
			"\{woof}\s!\"""");
		String s10 = java.util.FormatProcessor.FMT
			. "%-14s\{"A\s" + STR . "quick" + "brown"}%s\{fox} "
			+ java.util.FormatProcessor.FMT
			. "%-20s\{"jumps\sover the\s"
					+ STR . "lazy"}%s\{dog}";
		String s11 = STR."""
			\"\{			// A nested comment.
		(new java.util.function.Function<String, String>() {
			public String apply(String bay) { return bay; };
		}).apply(woof)
			}\s!\"""";
		String s12 = java.util.FormatProcessor.FMT
			."""
			%-14s\{STR."""
				A\s\
				\{ "quick" } \
				brown"""}\
			%s\{ fox } \
			%-20s\{STR."""
				jumps\s\
				over \
				the\s\
				\{ "lazy" } """}\
			%s\{ dog }""";
		String s13 = STR
			."\"\{			/* A nested comment. */
		((java.util.function.Function<String, String>) bay -> bay)
							.apply(woof)
			}\s!\"";
	}
}
