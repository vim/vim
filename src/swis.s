; Thomas Leonard
; 24/5/98

ar0	rn	0
ar1	rn	1
ar2	rn	2
ar3	rn	3
ar4	rn	4
ar5	rn	5
ar6	rn	6
ar7	rn	7
ar10	rn	10
ar11	rn	11
lk	rn	14
ar15	rn	15

	AREA	DATA
	align	4

	export	|r0|
r0:	dcd	0

	export	|r1|
r1:	dcd	0

	export	|r2|
r2:	dcd	0

	export	|r3|
r3:	dcd	0

	export	|r4|
r4:	dcd	0

	export	|r5|
r5:	dcd	0

	export	|r6|
r6:	dcd	0

	export	|r7|
r7:	dcd	0

	export	|time_of_last_poll|
time_of_last_poll:  dcd	0

	AREA	CODE, READONLY
	align 4
	import	|r0|
	export	|swi|
	=	"swi"
	align 4
swi:
	; r0 = swi number
	stmfd	sp!,{ar4-ar10,lk}
	orr	ar10,ar0,#1<<17		;always use the X form
	mov	ar0,ar1
	mov	ar1,ar2
	mov	ar2,ar3
	add	ar3,sp,#4*8
	ldmia	ar3,{ar3-ar7}
	swi	0x6f		; OS_CallASWI
	ldr	ar10,regs_addr
	stmia	ar10,{ar0-ar7}
	ldmvcfd	sp!,{ar4-ar10,pc}^
	; report the error and quit on Cancel
	mov	r1,#0x17
	adr	r2,s_title
	swi	0x400df		; Wimp_ReportError
	cmp	r1,#1			;OK selected?
	ldmeqfd	sp!,{ar4-ar10,pc}^	;yes - try to continue
	swi	0x11			;no  - die (OS_Exit)
s_title:
	= "Nasty error - Cancel to quit"
	= 0

	align 4
	export	|xswi|
	=	"xswi"
	align 4
xswi:
	; r0 = swi number
	stmfd	sp!,{ar4-ar10,lk}
	orr	ar10,ar0,#1<<17		;always use the X form
	mov	ar0,ar1
	mov	ar1,ar2
	mov	ar2,ar3
	add	ar3,sp,#4*8
	ldmia	ar3,{ar3-ar7}
	swi	0x6f		; OS_CallASWI
	ldr	ar10,regs_addr
	stmia	ar10,{ar0-ar7}
	mov	ar0,#0
	orr	ar0,ar0,ar15
	ldmfd	sp!,{ar4-ar10,pc}^

regs_addr:
	dcd	r0

	; The Wimp_Poll swis have to be done specially because,
	; for some reason, r13 sometimes gets corrupted by Wimp_Poll
	; (eg when running FileFind)
	AREA	CODE, READONLY
	align 4
	import  |time_of_last_poll|
	export	|wimp_poll|
	=	"wimp_poll"
	align 4
wimp_poll:
	mov	ar3,sp
	swi	0x400c7		; Wimp_Poll
	mov	sp,ar3

	mov	ar3,ar0
	swi	0x42		; OS_ReadMonotonicTime
	ldr	ar2,addr_time
	str	ar0,[ar2]
	mov	ar0,ar3

	mov	ar2,#0
	wfs	ar2		; Write floating point status. Needed?
	movs	pc,lk

	align 4
	export	|wimp_pollidle|
	=	"wimp_pollidle"
	align 4
wimp_pollidle:
	mov	ar3,sp
	swi	0x400e1		; Wimp_PollIdle
	mov	sp,ar3

	mov	ar3,ar0
	swi	0x42		; OS_ReadMonotonicTime
	ldr	ar2,addr_time
	str	ar0,[ar2]
	mov	ar0,ar3

	mov	ar2,#0
	wfs	ar2		; Write floating point status. Needed?
	movs	pc,lk

addr_time: dcd	time_of_last_poll
