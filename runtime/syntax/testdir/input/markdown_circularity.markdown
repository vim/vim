No code is recognised in HTML snippets.

~~~html
<pre><code>
/** HTML syntax circularity tests. */
class HTMLSyntaxCircularityTests
{
    /// @hidden
    ///
    /// @param args optional command-line arguments
    public static void main(String[] args)
    {
        System.out.println("""
            ```java
            class SyntaxCircularityTests
            {
                public static void main(String[] args)
                {
                    System.out.println(".");
                }
            }
            ```
        """);
    }
}
</code></pre>
~~~


Markdown documentation comments are not recognised in Java snippets.

```java
/** Java syntax circularity tests. */
class JavaSyntaxCircularityTests
{
    /// @hidden
    ///
    /// @param args optional command-line arguments
    public static void main(String[] args)
    {
        System.out.println("""
            <pre><code>
            class SyntaxCircularityTests
            {
                public static void main(String[] args)
                {
                    System.out.println(".");
                }
            }
            </code></pre>
        """);
    }
}
```
