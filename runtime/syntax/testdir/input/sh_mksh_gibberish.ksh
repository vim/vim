#!/bin/mksh
# Welcome to mksh wild west gibberish!

# Valid function names

function 7fo@o.f() {
	echo "Gibberish not-KornShell function (the ending '()' is a bashism mksh allows)"
}
!:@-+.8vfo%o,_() {
	echo 'Gibberish POSIX function'
}

,() {
	foo
}

function a%a() {
	foo
}

%() {
	foo
}

.() {
	foo
}

-() {
	foo
}

_() {
	foo
}

+a() {
	foo
}

.b() {
	foo
}

!a() {
	foo
}

@a() {
	foo
}

!!a() {
	foo
}

!a!a() {
	foo
}

@a@@a() {
	foo
}

+a+a() {
	foo
}

a:() {
	foo
}

# Invalid function names
@a@() {
	foo
}

@() {
	foo
}

@@() {
	foo
}

a@() {
	foo
}

!() {
	foo
}

!!() {
	foo
}

!a!() {
	foo
}

+() {
	foo
}

++() {
	foo
}

+a+() {
	foo
}

-#a() {
	foo
}

-?a() {
	foo
}

 () {
	 no name
}

!:@-+.8vfo%o,_
