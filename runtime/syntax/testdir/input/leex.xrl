% Header comment
%% Header comment
%%% Header comment

Definitions.
floats	= (\+|-)?[0-9]+\.[0-9]+((E|e)(\+|-)?[0-9]+)?
D	= [0-9]
A	= ({D}|_|@)
WS	= ([\000-\s]|%.*) % whitespace

Rules.
{D}+ :
  % Comment
  {token,{integer,TokenLine,list_to_integer(TokenChars)}}.
{D}+\.{D}+((E|e)(\+|\-)?{D}+)? :
  % Coment with period.
  {token,{float,TokenLine,list_to_float(TokenChars)}}.
{A} :  ErlangCode. % comment
{WS} : ErlangCode.
:= :{token,{':=',TokenLine}}.

Erlang code.

-export([reserved_word/1]).

%% reserved_word(Atom) -> Bool
reserved_word('reserved') -> true;
reserved_word(_) -> false.
