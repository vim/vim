// VIM_TEST_SETUP let g:java_highlight_functions = 'style'
// VIM_TEST_SETUP let g:java_highlight_generics = 1


import java.lang.invoke.MethodHandle;
import java.util.function.BiPredicate;
import java.util.function.Consumer;
import java.util.function.Function;
import java.util.function.IntFunction;
import java.util.function.IntSupplier;
import java.util.function.Predicate;
import java.util.function.Supplier;
import java.util.function.ToIntFunction;
import java.util.function.UnaryOperator;

class MethodReferencesTests
{
	static {
		// Primary :: [TypeArguments] Identifier
		try {
			Runnable r1 = ((Runtime) null)::gc;
		} catch (NullPointerException expected) {
		}

		Supplier<Integer> s1 = ((Number) 0)::hashCode;
		Supplier<Integer> s2 = ((Comparable<?>) '\0')::hashCode;
		Supplier<Integer> s3 = ((Comparable<?>) false)::hashCode;
		Supplier<Integer> s4 = "::"::hashCode;
		Supplier<Class<?>> s5 = int[].class::arrayType;
		Supplier<Integer> s6 = new MethodReferencesTests() ::
			hashCode;
		Supplier<Integer> s7 = ((Number)
			(new MethodReferencesTests().xy)[0])::intValue;
		Supplier<int[]> s8 = new MethodReferencesTests().xy::
			clone;
		Consumer<Object> c1 = System.out :: println;
		Supplier<byte[]> s9 = ((Supplier<String>) ()->"()").get()
			::getBytes;
		Supplier<String> sa = ((Supplier<String>)
			((Supplier<String>) ((Supplier<String>)
			((Supplier<String>) ((Supplier<String>)
			() -> "() -> ()")
			::toString)
			::toString)
			::get)
			::toString)
			::toString;

		// ExpressionName :: [TypeArguments] Identifier
		// ReferenceType :: [TypeArguments] Identifier
		Function<String, IntSupplier> f1 = s ->
						s :: length;
		Function<int[][], Supplier<int[]>> f2 = ii ->
			((int[]) (ii.length > 0 ? ii[0] : ii))
							:: clone;
		UnaryOperator<String> uo1 = String::valueOf;
		ToIntFunction<String> tif1 = s -> s.transform(
						String :: length);

		// ClassType :: [TypeArguments] new
		// ArrayType :: new
		Function<Object, C2> f3 = C2::<Object>new;
		Function<C2, C2.C21> f4 = pci -> pci.new
					<String>C21(null); // Cf. "d".
		Supplier<C1<?>> sb = C1::new;
		Function<Byte, C1<?>> f5 = C1<Void> :: <Byte> new;
		IntFunction<C1<?>[]> if1 = C1<?>[] :: new;
		IntFunction<byte[]> if2 = byte[] :: new;
	}

	final int[] xy = { 0, 1 };

	// super :: [TypeArguments] Identifier
	// TypeName . super :: [TypeArguments] Identifier
	<T> MethodReferencesTests()
	{
		Predicate<T> p1 = MethodReferencesTests.super::equals;
		Predicate<T> p2 = MethodReferencesTests.this::equals;
	}

	interface I4<T> extends I3<T>
	{
		default Predicate<T> superEqualist()
		{
			return I3
				.super::equals;	/* "a" */
		}
	}

	interface I3<T> extends I2<T>
	{
		default Predicate<T> superEqualist()
		{
			return I2.
				super::equals;	/* "b" */
		}
	}

	interface I2<T> extends I1<T>
	{
		default Predicate<T> superEqualist()
		{	/* Non-capturing gymnastics for super::equals. */
			return Function.<Function<MethodHandle,
							Predicate<T>>>
								identity()
				.apply(mh -> o -> MethodReferencesTests
						.invokePredicate(mh, o))
				.apply(EQUALS.bindTo(this));
		}
	}

	interface I1<T>
	{
		default Predicate<T> equalist()
		{	/* Non-capturing gymnastics for this::equals. */
			return Function.<Function<I1<T>, Predicate<T>>>
								identity()
				.apply(that -> o -> Function
						.<BiPredicate<I1<T>, T>>
								identity()
					.apply(I1<T>::	/* "c" */
						equals)
					.test(that, o))
				.apply(I1.this);
		}
	}

	static <T> boolean invokePredicate(MethodHandle mh, T o)
	{
		try {
			return (boolean) mh.invokeExact(o);
		} catch (Throwable th) {
			throw new RuntimeException(th);
		}
	}

	private static final MethodHandle EQUALS;

	static {
		try {
			EQUALS = java.lang.invoke.MethodHandles.lookup()
							.findSpecial(
				I1.class,
				"equals",
				java.lang.invoke.MethodType.methodType(
							boolean.class,
							Object.class),
				I2.class);
		} catch (ReflectiveOperationException e) {
			throw new Error(e);
		}
	}

	static class C1<T>
	{
		C1() { }
		<A> C1(A dummy) { }
	}

	static class C2
	{
		C2() { <String> this(""); }

		<A> C2(A dummy)
		{
			C2.stringer().apply(((Function<C2, C2.C21>)
						C2.C21::new)	/* "d" */
					.apply(C2.this));
		}

		class C21
		{
			C21() { <String> this(""); }

			<B> C21(B dummy)
			{
				C2.stringer().apply(C2.this);
			}
		}

		static <T extends Object> Function<T, String> stringer()
		{
			return T::toString;	/* "e" */
		}
	}
}
