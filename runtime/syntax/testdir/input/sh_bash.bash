#!/bin/bash

# bash 5.3+ supports command substitution that is very similar
# (but not identical) to ksh/mksh.
echo ${ echo one;}
echo ${	echo two
}
echo ${
echo three	;}
echo ${ echo 'four'; }
echo ${ echo 'five' ;}
echo ${ echo 'six'
}
echo ${	echo 'seven'	;}
echo ${ echo 'eight';	}
typeset nine=${ pwd; }
echo ${ echo 'nine' ; 
 }

valsubfunc() {
	REPLY=$1
}
echo ${|valsubfunc ten;}
echo "${|valsubfunc eleven; }"
printf '%s\n' "${|valsubfunc twelve	;}"
unlucky=${|valsubfunc thirteen
}
typeset notafloat=${|valsubfunc notanumber	; 
 }
echo $unlucky $notanumber
${|echo fourteen;}
${|echo fifteen
}
