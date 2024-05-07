// VIM_TEST_SETUP let g:java_highlight_functions = 'indent8'
// VIM_TEST_SETUP set encoding=utf-8 termencoding=utf-8

import java.lang.annotation.ElementType;
import java.lang.annotation.Target;

abstract class Indent8MethodsTests
{ // DO NOT retab! THIS FILE; REMEMBER ABOUT testdir/ftplugin.
        // TYPES.
        record Τʬ<α>(α a) { }

        enum 𝓔
        {
                A("𝕬"), B("𝕭"), C("𝕮"), D("𝕯"),
                E("𝕰"), F("𝕱"), G("𝕲"), H("𝕳");
                final String 𝐬;
                private 𝓔(String 𝐬) { this.𝐬 = 𝐬; }
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
        @Tɐggablɘ @Tɐggablɘ protected Indent8MethodsTests() { }
        <T extends Comparable<T>> Indent8MethodsTests(T t, Void v) { }
        private <T extends Comparable<T>> Indent8MethodsTests(T t) { }

        // METHODS.
        @Tɐggablɘ @Tɐggablɘ abstract void ascii$0_(////////////////
                                                                );
        @Tɐggablɘ @Tɐggablɘ abstract <α, β> Τʬ<α> μʭʭ$0_(
                        /* TODO: @SuppressWarnings("bespoke")*/ β 𝛽);

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
        Indent8MethodsTests.<α, β>μʭʭ$98_(ƒ)[0];
        }

        public static Class<?> classLock() { return Indent8MethodsTests.class; }

        @Override @SuppressWarnings("cast")
        public String toString() { return (String) "Indent8MethodsTests"; }
}

enum 𝓔
{
        @SuppressWarnings("bespoke") A("𝗔"),
        B("𝗕"),
        C("𝗖"), D("𝗗"),
        E("𝗘"), F("𝗙"), G("𝗚"), H("𝗛");
        final String 𝐬;
        private 𝓔(String 𝐬) { this.𝐬 = 𝐬; }
}
