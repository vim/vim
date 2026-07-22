// This module declaration belongs to the sample project published at
// https://github.com/zzzyxwvut/module-info.git .
import module java.base;
import java.util.ServiceLoader;

/**
 * Defines demo related support.
 *
 * Note that the {@code Testable} service is not exported.
 *
 * @uses org.demo.internal.Testable
 * @provides org.demo.internal.Testable
 * @see ServiceLoader
 */
module org.module.info.demo	// JDK 23+ (--enable-preview --release 23).
{
	requires static jdk.jfr;
	requires java.base;
	requires transitive java.logging;
	requires transitive static org.module.info.tester;

	exports org.demo;
	exports org.demo.internal to
		org.module.info.demo;

	opens org.demo.internal to
		org.module.info.demo;
	opens org.demo.tests to
		org.module.info.demo, org.module.info.tester;

	uses org.demo.internal.Testable;

	provides org.demo.internal.Testable with
		org.demo.tests.ArithmeticOperationTests;
}
