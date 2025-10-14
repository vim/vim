// VIM_TEST_SETUP let g:java_highlight_functions = 'style'


import java.lang.annotation.ElementType;
import java.lang.annotation.Target;

class AnnotationsTests
{
	@Target(ElementType.TYPE_USE)
	@interface Tag
	{
		String value() default "";
		String kind() default "";
	}

	@Target(ElementType.TYPE_USE)
	@interface Text
	{
		String[] value() default {""};
	}

	@Target({
		ElementType.METHOD,
		ElementType.PARAMETER,
		ElementType.TYPE,
	})
	@interface Labels
	{
		Label[] value();
	}

	@java.lang.annotation.Target({
		java.lang.annotation.ElementType.METHOD,
		java.lang.annotation.ElementType.PARAMETER,
		java.lang.annotation.ElementType.TYPE,
	})
	@java.lang.annotation.Repeatable(Labels.class)
	@interface Label
	{
		String value() default "";
		Class<?> type() default Label.class;
		boolean redundant() default true;
		Text text() default @Text;
		Tag head() default @Tag();
		Tag tail() default @Tag(value = "", kind = "");
	}

	/* Use identity cast expressions to nest TYPE_USE annotations. */
								@Label(
							(@Text({
		(@Text({ "a", "aa", "aaa", "aaaa", }) String) "as",
		(@Text({ "b", "bb", "bbb", "bbbb", }) String) "bs",
		(@Text({ "c", "cc", "ccc", "cccc", }) String) "cs",
		(@Text({ "d", "dd", "ddd", "dddd", }) String) "ds",
							}) String) "abcd")
	interface Primer { }

	@Label @Label() @Label("""
	n\
	o\
	O\
	p""")
	@Label(head = @Tag(value = "@Label"/*, kind = "name"*/))
	@Label(// value = "Method",
		type = AnnotationsTests.class,
		redundant = !!!(1 != 1),
		head = @Tag(value = "@Label"),
		text = @Text({ "})", "({" }))
	static void noOp(@Label @Label() @Label("dummy")
		@Label(head = @Tag(/*value = "@Label",*/ kind = "name"))
		@Label(// value = "Parameter",
			type = AnnotationsTests.class,
			head = @Tag(value = "@Label"),
			text = @Text({ "){", "}(" }))
		Object dummy)
	{
	}
}
