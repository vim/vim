" Filter that removes the Shell Prompt from the xxd command
:1s#|\$+0&\#ffffff0| \S\+/|x@1|d|.*\n#|$+0\&\#ffffff0| #e
