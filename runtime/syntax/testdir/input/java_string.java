class StringTests
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
	}
}
