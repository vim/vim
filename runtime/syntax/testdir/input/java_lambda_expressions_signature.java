// VIM_TEST_SETUP let g:java_highlight_functions = 'style'
// VIM_TEST_SETUP let g:java_highlight_signature = 1

import java.lang.annotation.ElementType;
import java.util.function.BinaryOperator;
import java.util.function.Function;
import java.util.function.Predicate;

class LambdaExpressions$Tests	// JDK 21+.
{
	<I1, C1, C2, T1, T2, T3, Z1, Z2, Z3, S1, S2, S3> void test()
	{	// Schönfinkel's functions.
		I<I1> i = x -> x;
		C<C1, C2> c = x -> y -> x;
		T<T1, T2, T3> t = f -> y -> x -> f.apply(x).apply(y);
		Z<Z1, Z2, Z3> z = f -> g -> x -> f.apply(g.apply(x));
		S<S1, S2, S3> s = f -> g -> x -> f.apply(x)
						.apply(g.apply(x));

		I<I1> i01 = (var x) -> x;
		I<I1> i02 = (@Taggable var x) -> x;
		I<I1> i03 = (@Taggable @Taggable var x) -> x;
		I<I1> i04 = (final var x) -> x;
		I<I1> i05 = (@Taggable final var x) -> x;
		I<I1> i06 = (@Taggable @Taggable final var x) -> x;
		I<I1> i07 = (I1 x) -> x;
		I<I1> i08 = (@Taggable I1 x) -> x;
		I<I1> i09 = (@Taggable @Taggable I1 x) -> x;
		I<I1> i10 = (final I1 x) -> x;
		I<I1> i11 = (@Taggable final I1 x) -> x;
		I<I1> i12 = (@Taggable @Taggable final I1 x) -> x;

		I<I1[]> ii01 = (I1... x) -> x;
		I<I1[]> ii02 = (@Taggable I1... x) -> x;
		I<I1[]> ii03 = (@Taggable @Taggable I1... x) -> x;
		I<I1[]> ii04 = (final I1... x) -> x;
		I<I1[]> ii05 = (@Taggable final I1... x) -> x;
		I<I1[]> ii06 = (@Taggable @Taggable final I1... x) -> x;

		BinaryOperator<I1> leftConst01 = (var x, var y) -> x;
		BinaryOperator<I1> leftConst02 = (@Taggable var x,
					@Taggable var y) -> x;
		BinaryOperator<I1> leftConst03 = (@Taggable @Taggable var
					x, @Taggable @Taggable var y) -> x;
		BinaryOperator<I1> leftConst04 = (final var x,
					final var y) -> x;
		BinaryOperator<I1> leftConst05 = (@Taggable final
					var x, @Taggable final var y) -> x;
		BinaryOperator<I1> leftConst06 = (@Taggable
					@Taggable final var x,
					@Taggable
					@Taggable final var y) -> x;
		BinaryOperator<I1> leftConst07 = (I1 x, I1 y) -> x;
		BinaryOperator<I1> leftConst08 = (@Taggable I1 x,
					@Taggable I1 y) -> x;
		BinaryOperator<I1> leftConst09 = (@Taggable @Taggable I1
					x, @Taggable @Taggable I1 y) -> x;
		BinaryOperator<I1> leftConst10 = (final I1 x,
					final I1 y) -> x;
		BinaryOperator<I1> leftConst11 = (@Taggable final
					I1 x, @Taggable final I1 y) -> x;
		BinaryOperator<I1> leftConst12 = (@Taggable
					@Taggable final I1 x,
					@Taggable
					@Taggable final I1 y) -> x;

		Runnable noOp = () -> {};
		BinaryOperator<I1> leftConst = (x, y) -> x;
		I<I1> id1 = (x) -> (x);
		@SuppressWarnings("unchecked") I<I1> id2 =
				((I<I<I1>>) (I<?>) (Function<I1,
					I1> x) -> x).apply(switch (0) {
				case ((int) (byte) 1) -> (I1 x) -> x;
				default -> (@Taggable I1 x) -> x; });
		C<C1, C2> const1 = (x) -> (y) -> (x);
		C<C1, C2> const2 = switch(switch ("") {
						case "->"->"(s)->(s)";
						default->"default"; }) {
			case ("->")->(var x)->(var y)->(x);
			default->(@Taggable var x)->(@Taggable var y)
				->(x);
		};
	}

	@java.lang.annotation.Target(ElementType.PARAMETER)
	@java.lang.annotation.Repeatable(Taggables.class)
	@interface Taggable { String[] value() default ""; }

	@java.lang.annotation.Target(ElementType.PARAMETER)
	@interface Taggables { Taggable[] value(); }

	interface I<A1> extends Function<A1, A1> { }
	interface C<A1, A2> extends Function<A1, Function<A2, A1>> { }
	interface T<A1, A2, A3> extends
				Function<Function<A1, Function<A2, A3>>,
					Function<A2,
					Function<A1, A3>>> { }
	interface Z<A1, A2, A3> extends Function<Function<A2, A3>,
					Function<Function<A1, A2>,
					Function<A1, A3>>> { }
	interface S<A1, A2, A3> extends
				Function<Function<A1, Function<A2, A3>>,
					Function<Function<A1, A2>,
					Function<A1, A3>>> { }

	static void echo(Object o) { System.out.println(o); }

	static {
		enum Letters { OTHER, ALPHA, BETA }

		Letters other = Letters.OTHER;

		switch (other) {
		case Letters alpha when Letters.ALPHA == alpha:
						{ echo(alpha); break; }
		case Letters beta when Letters.BETA == beta:
						{ echo(beta); break; }
		default:			{ echo(other); }
		}

		echo(switch (other) {
			case Letters alpha when Letters.ALPHA == alpha
						-> alpha;
			case Letters beta when Letters.BETA == beta
						-> beta;
			default			-> other;
		});

		switch (null) {
		case String str when !"<empty>".equals(switch (str) {
			case String str_ when
					Predicate.<String>not(text ->
							!text.isEmpty())
						.test(str_)
							-> "<empty>";
			case String str_		-> str_;
			}):			{ echo(str); break; }
		case null: default:		{ echo("Other"); }
		}

		echo(switch (null) {
			case String str when !"<empty>".equals(
							switch (str) {
				case String str_ when
					Predicate.<String>not(text ->
							!text.isEmpty())
						.test(str_)
							-> "<empty>";
				case String str_	-> str_;
				})		-> str;
			case null, default	-> "Other";
		});
	}
}
