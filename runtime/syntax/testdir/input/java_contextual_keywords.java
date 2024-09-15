// VIM_TEST_SETUP :highlight link javaConceptKind NonText



class ContextualKeywordsTests		// See JLS, §3.9 Keywords.
{
	private ContextualKeywordsTests() { throw new Error(); }

	// ModuleDeclaration: module open.
	void module()	{ Object module = null;		when(); }
	void open()	{ Object open = null;		module(); }
	// ModuleDirective: exports opens provides requires to uses with.
	void exports()	{ Object exports = null;	open(); }
	void opens()	{ Object opens = null;		exports(); }
	void provides()	{ Object provides = null;	opens(); }
	void requires()	{ Object requires = null;	provides(); }
	void to()	{ Object to = null;		requires(); }
	void uses()	{ Object uses = null;		to(); }
	void with()	{ Object with = null;		uses(); }
	// RequiresModifier: transitive.
	void transitive() { Object transitive = null;	with(); }
	// LocalVariableType | LambdaParameterType: var.
	void var()	{ var var = new Object();	transitive(); }
	// YieldStatement: yield (see java_switch.java).
	void yield()	{ Object yield = null;		var(); }
	// RecordDeclaration: record.
	void record()	{ Object record = null;		this.yield(); }
	// Normal{Class,Interface}Declaration: non-sealed permits sealed.
	void permits()	{ Object permits = null;	record(); }
	void sealed()	{ Object sealed = null;		permits(); }
	// Guard: when (see java_switch.java).
	void when()	{ Object when = null;		sealed(); }

	sealed interface I1 permits C1, I3 { }
	sealed interface I2 permits C1, I3 { }
	non-sealed interface I3 extends I1, I2 { }
	interface I4 extends I3 { }

	abstract sealed class C1 implements I1, I2 permits C2, C3 { }
	abstract non-sealed class C2 extends C1 { }
	final class C3 extends C1 implements I3 { }
	class C4 extends C2 { }

	record R() implements I3 { }
	enum E implements I3 { INSTANCE }

	static <T> I<T> i1() { return (var var) -> var; }
	static <T> I<T> i2() { return (T var) -> var; }
	static <T> I<T> i3() { return (var) -> var; }
	static <T> I<T> i4() { return var -> var; }
	interface I<T> { T i(T i); default I<T> self() { return this; } }
}
