/*
 * VIM_TEST_SETUP set filetype=progress
 */

define variable customer_name as character no-undo.

/* The test setup above is an example of a multi-line comment.
This is too; the leading * and left-hand alignment are not required. */
for each customer no-lock
    where customer.customer_id = 12345
:
    assign cust_name = customer.customer_name. /* Comments can also appear
                                                  at the end of a line. */
end. /* for each customer */

/* Comments can be /* nested */. Here's the same query as above, but
commented out this time:

for each customer no-lock
    where customer.customer_id = 12345
:
    assign cust_name = customer.customer_name. /* Comments can also appear
                                                  at the end of a line. */
end. /* for each customer */

TODO: Note that /*/ does not end the comment, because it actually starts a
new comment whose first character is a '/'. Now we need two end-comment
markers to return to actual code. */ */

display customer_name.

