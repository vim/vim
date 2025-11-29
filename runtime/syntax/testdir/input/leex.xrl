%% Header comment

Definitions.

UNIXCOMMENT     = #[^\n]*
D		= [0-9]
Atoms		= [a-z][0-9a-zA-Z_]*
Variables	= [A-Z_][0-9a-zA-Z_]*
Floats		= (\+|-)?[0-9]+\.[0-9]+((E|e)(\+|-)?[0-9]+)?

Rules.

{UNIXCOMMENT} : skip_unix_comment(TokenChars, TokenLine).

{D}+ :
  {token,{integer,TokenLine,list_to_integer(TokenChars)}}.

{D}+\.{D}+((E|e)(\+|\-)?{D}+)? :
  {token,{float,TokenLine,list_to_float(TokenChars)}}.

Erlang code.

skip_unix_comment("#" ++ _Rest, _Line) -> skip_token.
