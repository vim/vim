// VIM_TEST_SETUP let g:java_syntax_previews = [430]



class StringTemplateTests	// JDK 21+ (--enable-preview --release 21).
{
	static {
		System.out.println(STR."""
		"
		\{"\"\""}
		\{"\"\""}\{"\u005c\u0022"}
		\{"\"\""}\{"\u005c\u0022"}"
		\{"\"\""}\"\{"\u005c\u0022\u005c\u0022"}
		\{"\"\""}\"\{"\"\""}\{"\u005c\u0022"}
		\{"\"\""}\"\{"\"\""}\""
		\{"\"\""}\"\{"\"\""}\""\"""");

		String woof = "Woof", dog = "dog", fox = "fox";

		String s1 = STR
			."A quick brown \{fox} jumps over the lazy \{dog}";
		String s2 = STR.process(StringTemplate.RAW
			."\"\{woof}\s!\"");
		String s3 = STR."""
			A\s\
			quick \
			brown\s\
			\{fox} \
			jumps\s\
			over \
			the\s\
			lazy \
			\{dog}""";
		String s4 = STR.process(StringTemplate.RAW
			.
			"""
			"\{woof}\s!\"""");
		String s5 = java.util.FormatProcessor.FMT
			. "%-14s\{"A\s" + STR . "quick" + "brown"}%s\{fox} "
			+ java.util.FormatProcessor.FMT
			. "%-20s\{"jumps\sover the\s"
					+ STR . "lazy"}%s\{dog}";
		String s6 = STR."""
			\"\{			// A nested comment.
		(new java.util.function.Function<String, String>() {
			public String apply(String bay) { return bay; };
		}).apply(woof)
			}\s!\"""";
		String s7 = java.util.FormatProcessor.FMT
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
		String s8 = STR
			."\"\{			/* A nested comment. */
		((java.util.function.Function<String, String>) bay -> bay)
							.apply(woof)
			}\s!\"";
	}
}
