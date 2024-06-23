// VIM_TEST_SETUP let g:java_highlight_functions = 'indent8'
// VIM_TEST_SETUP let g:java_highlight_signature = 1
// VIM_TEST_SETUP set encoding=utf-8 termencoding=utf-8
import java.lang.annotation.ElementType;
import java.lang.annotation.Target;

abstract class Indent8$MethodsTests
{ // DO NOT retab! THIS FILE; REMEMBER ABOUT testdir/ftplugin.
        // TYPES.
        record Τʬ<α>(α a) { }

        enum E
        {
                A("a"), B("b"), C("c"), D("d"),
                E("e"), F("f"), G("g"), H("h");
                final String s;
                private E(String s) { this.s = s; }
        }

        @Target({ElementType.METHOD, ElementType.CONSTRUCTOR})
        @java.lang.annotation.Repeatable(Tɐggablɘs.class)
        @interface Tɐggablɘ
        {
                String[] value() default "";
        }

        @Target({ElementType.METHOD, ElementType.CONSTRUCTOR})
        @interface Tɐggablɘs
        {
                Tɐggablɘ[] value();
        }

        interface Stylable<Α>
        {
                default void ascii$0_() { }
                default Α μʭʭ$0_() { return null; }
        }

        // FIELDS.
        private static final Class<?> CLASS_LOCK = classLock();

        private final Object instanceLock = new Object();

        // CONSTRUCTORS.
        @Tɐggablɘ @Tɐggablɘ protected Indent8$MethodsTests() { }
        <T extends Comparable<T>> Indent8$MethodsTests(T t, Void v) { }
        private <T extends Comparable<T>> Indent8$MethodsTests(T t) { }

        // METHODS.
        @Tɐggablɘ @Tɐggablɘ abstract void ascii$0_(////////////////
                                                                );
        @Tɐggablɘ @Tɐggablɘ abstract <α, β> Τʬ<α> μʭʭ$0_(
                                @SuppressWarnings("bespoke") β b);

        @Tɐggablɘ private native void ascii$1_(/*////////////*/);
        @Tɐggablɘ private native <α, β> Τʬ<α>[] μʭʭ$1_(
                        java.util.function.Function<β, Τʬ<α>[]> ƒ);

        void Ascii$2_() { }
        <T, U extends Stylable<T>> void Μʭʭ$2_(U u) { }

        static final native synchronized void ascii$98_();
        static final native synchronized <α, β> Τʬ<α>[][] μʭʭ$98_(
                        java.util.function.Function<β, Τʬ<α>[][]> ƒ);

        @SuppressWarnings("strictfp")
        protected static final synchronized strictfp void ascii$99_()
        { ascii$98_(); }

        @SuppressWarnings("strictfp")
        protected static final synchronized strictfp <α, β> Τʬ<α>[] μʭʭ$99_(
                        java.util.function.Function<β, Τʬ<α>[][]> ƒ)
        {
                return
        Indent8$MethodsTests.<α, β>μʭʭ$98_(ƒ)[0];
        }

        public static Class<?> classLock() { return Indent8$MethodsTests.class; }

        @Override @SuppressWarnings("cast")
        public String toString() { return (String) "Indent8$MethodsTests"; }
}

enum E8$
{
        @SuppressWarnings("bespoke") A("a"),
        B("b"
                /*\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\*/),
        C("c", new Thread(

                () -> {
        })), D("d", (java.util.function.BooleanSupplier) () -> true),
        E("e", new char[] { 'a', 'b', 'c', 'd' }), F("f", new Object() {
                transient String name = "";
                @Override public String toString() { return this.name; }
        }), //\//\//\//\//\//\//\//\//\//\//\//\//\//\//\//\//\//\//\//\//
        G("g"), @Deprecated H("h");

        final String s;
        private E8$(String s) { this.s = s; }
        private <δ> E8$(String s, δ dummy) { this(s); }

        @Override public String toString() { return name().toUpperCase(); }
}
