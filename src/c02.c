#include "c0.h"

/* P
 * arses a function, the function name and the opening parenthesis of the
 * argument list has already been read. name contains the name of the function.
 */
function(name)
char name[]; {
#ifdef UNIXV5_ABI
	printf( ".text; %p:\n", name);
#else
    printf( ".data; %p:1f\n.text; 1:", name);
#endif
	printf("mov r5,-(sp); mov sp,r5\n");  /* set up stack frame */
	declare(8);  /* read parameter list */
	declist(); /* type declarations of parameters */

	statement(1);
	retseq();
}

/* Parses the next function/global variable definition. */
void extdef() {
	auto o, c, *cs;
	char *s;

	if(((o=symbol())==0) || o==1)	/* EOF */
		return;
	if(o!=20)  /* not a name -> syntax error */
		goto syntax;
	csym[0] = 6;
	cs = &csym[4];  // name of the symbol
	printf(".globl	%p\n", cs);
	s = ".data; %p:1f\n";
	switch(o=symbol()) {

	case 6:				/* ( - function definition*/
		function(cs);
		return;

	case 21:			/* const - variable has default value*/
		printf(".data; %p: %o\n", cs, cval);
		if((o=symbol())!=1)	/* ; */
			goto syntax;
		return;

	case 1:				/* ; */
		printf(".bss; %p: .=.+2\n", cs);  /* unitialized variable */
		return;

	case 4:				/* [ */
		c = 0;
		if((o=symbol())==21) {	/* const */
			c = cval<<1;  /* multiply by two (number of bytes per word), should be changed for 32bit systems */
			o = symbol();
		}
		if(o!=5)		/* ] */
			goto syntax;
		printf(s, cs);
		if((o=symbol())==1) {	/* ; */
			printf(".bss; 1:.=.+%o\n", c);
			return;
		}
		/* symbol list, e.g.:
		arrname[] 23, 43 ,5; */
		printf("1:");
		while(o==21) {		/* const */
			printf("%o\n", cval);
			c -= 2;
			if((o=symbol())==1)	/* ; */
				goto done;
			if(o!=9)		/* , */
				goto syntax;
			else
				o = symbol();
		}
		goto syntax;
	done:
		if(c>0)
			printf(".=.+%o\n", c);
		return;

	case 0:				/* EOF */
		return;
	}

syntax:
	error("External definition syntax");
	errflush(o);
	statement(0);
}

/*
 * Parses (a block of) statements.
 * d - true if this is a start of a function block
 */
void statement(d) {
	int o, o1, o2, o3, *np;

stmt:
	switch(o=symbol()) {

	/* EOF */
	case 0:
		error("Unexpected EOF");
	/* ; */
	case 1:
	/* } */
	case 3:
		return;

	/* { */
	case 2: {
		if(d)
			blkhed();  /* process definitions at the start of the funciton */
		/* recursively process this block of code */
		while (!eof) {
			if ((o=symbol())==3)	/* } */
				goto bend;
			peeksym = o;
			statement(0);
		}
		error("Missing '}'");
	bend:
		return;
	}

	/* keyword */
	case 19:
		switch(cval) {

		/* goto */
		case 10:
			o1 = block(1,102,0,0,tree());
			rcexpr(o1, regtab);
			goto semi;

		/* return */
		case 11:
			if((peeksym=symbol())==6)	/* ( */
				rcexpr(pexpr(), regtab);
			retseq();
			goto semi;

		/* if */
		case 12:
			jumpc(pexpr(), o1=isn++, 0);
			statement(0);
			if ((o=symbol())==19 & cval==14) {  /* else */
				o2 = isn++;
				(easystmt()?branch:jump)(o2);  /* branch can only jump to a close location */
				label(o1);
				statement(0);
				label(o2);
				return;
			}
			peeksym = o;
			label(o1);
			return;

		/* while */
		case 13:
			o1 = contlab;
			o2 = brklab;
			label(contlab = isn++);
			jumpc(pexpr(), brklab=isn++, 0);
			o3 = easystmt();
			statement(0);
			(o3?branch:jump)(contlab);
			label(brklab);
			contlab = o1;
			brklab = o2;
			return;

		/* break */
		case 17:
			if(brklab==0)
				error("Nothing to break from");
			jump(brklab);
			goto semi;

		/* continue */
		case 18:
			if(contlab==0)
				error("Nothing to continue");
			jump(contlab);
			goto semi;

		/* do */
		case 19:
			o1 = contlab;
			o2 = brklab;
			contlab = isn++;
			brklab = isn++;
			label(o3 = isn++);
			statement(0);
			label(contlab);
			contlab = o1;
			if ((o=symbol())==19 & cval==13) { /* while */
				jumpc(tree(), o3, 1);
				label(brklab);
				brklab = o2;
				goto semi;
			}
			goto syntax;

		/* case */
		case 16:
			if ((o=symbol())!=21)	/* constant */
				goto syntax;
			if ((o=symbol())!=8)	/* : */
				goto syntax;
			if (swp==0) {
				error("Case not in switch");
				goto stmt;
			}
			if(swp>=swtab+swsiz) {
				error("Switch table overflow");
			} else {
				*swp++ = isn;  /* add value and label to switch table */
				*swp++ = cval;
				label(isn++);
			}
			goto stmt;

		/* switch */
		case 15:
			o1 = brklab;
			brklab = isn++;
			np = pexpr();
			if (np[1]>1 & np[1]<16)
				error("Integer required");
			rcexpr(np, regtab);
			pswitch();
			brklab = o1;
			return;

		/* default */
		case 20:
			if (swp==0)
				error("Default not in switch");
			if ((o=symbol())!=8)	/* : */
				goto syntax;
			deflab = isn++;
			label(deflab);
			goto stmt;
		}

		error("Unknown keyword");
		goto syntax;

	/* name */
	case 20:
		if (peekc==':') {  /* label */
			peekc = 0;
			if (csym[0]>0) {
				error("Redefinition");
				goto stmt;
			}
			csym[0] = 2;
			csym[1] = 020;	/* int[] */
			if (csym[2]==0)
				csym[2] = isn++;
			slabel();
			goto stmt;
		}
	}

	peeksym = o;
	rcexpr(tree(), efftab);
	goto semi;

semi:
	if ((o=symbol())!=1)		/* ; */
		goto syntax;
	return;

syntax:
	error("Statement syntax");
	errflush(o);
	goto stmt;
}

/*
 * Parses an expression enclosed in parenthesis.
 * Returns the address to the parsed tree.
 */
pexpr()
{
	auto o, t;

	if ((o=symbol())!=6)	/* ( */
		goto syntax;
	t = tree();
	if ((o=symbol())!=7)	/* ) */
		goto syntax;
	return(t);
syntax:
	error("Statement syntax");
	errflush(o);
	return(0);
}

/* Parses the contents of a switch block. */
pswitch() {
	int *sswp, dl, cv, swlab;  /* ?, default label, current ?, switch label */

	sswp = swp;  /* save swp */
	if (swp==0)
		swp = swtab;
	swlab = isn++;
	printf("jsr	pc,bswitch; l%d\n", swlab);
	dl = deflab;  /* save deflab */
	deflab = 0;
	statement(0);
	if (!deflab) {
		deflab = isn++;
		label(deflab);
	}
	/* generate switch table */
	printf("L%d:.data;L%d:", brklab, swlab);
	while(swp>sswp & swp>swtab) {
		cv = *--swp;
		printf("%o; l%d\n", cv, *--swp);
	}
	printf("L%d; 0\n.text\n", deflab);
	deflab = dl;
	swp = sswp;
}

/*
 * Function block head: processes variable definitions.
 */
blkhed()
{
	int o, al, pl, *cs, hl;

	declist();
	stack = al = -2;
	pl = 4;
	while(paraml) {
		*parame = 0;  /* set the end of linked list to 0 (originally -1), will break the loop */
		paraml = *(cs = paraml);  /* next element in list */
		cs[2] = pl;  /* location relative to stack frame */
		*cs = 10;
		pl += rlength(cs[1]);
	}
	cs = hshtab;
	hl = hshsiz;
	while(hl--) {  /* go through symbol table, i.e. all defined names */
	    if (cs[4])
		switch(cs[0]) {  /* if defined */

		/* sort unmentioned */
		case -2:
			cs[0] = 5;		/* auto */

		/* auto */
		case 5:
			if (cs[3]) {	/* array */
				al -= (cs[3]*length(cs[1]-020)+1) & 077776;  /* push array on stack */
				setstk(al);
				defvec(al);
			}
			cs[2] = al;
			al -= rlength(cs[1]);
			goto loop;

		/* parameter */
		case 10:
			cs[0] = 5;
			goto loop;

		/* static */
		case 7:
			cs[2] = isn++;
			defstat(cs);
			goto loop;

		loop:;
		}
		cs = cs+pssiz;
	}
	setstk(al);
}

/*
 * Clears all elements from the symbol table,
 * except keywords.
 */
blkend() {
	auto i, hl;

	i = 0;
	hl = hshsiz;
	while(hl--) {
		if(hshtab[i+4])
			if (hshtab[i]==0)
				error1("%p undefined", &hshtab[i+4]);
			if(hshtab[i]!=1) {	/* not keyword */
				hshused--;
				hshtab[i+4] = 0;
			}
		i += pssiz;
	}
}

/* Throw away symbols until end of statement. */
errflush(o) {
	while(o>3)	/* ; { } */
		o = symbol();
	peeksym  = o;
}

/*
 * Variable declarations. Either function parameter type declaration
 * or function local variable declarations.
 */
declist()
{
	auto o;

	while((o=symbol())==19 & cval<10)  /* cval<10 means it is a type/storage area definition (int/char/extern/static) */
		declare(cval);
	peeksym = o;
}

/*
 * Detects whether the next statement is expected to be small - i.e. not a compound statement
* This usually means it is a single statement or conditional.
* e.g. goto, break, or not a label or block */
easystmt()
{
	if((peeksym=symbol())==20)	/* name */
		return(peekc!=':');	 /* not label */
	if (peeksym==19) {		/* keyword */
		switch(cval)

		case 10:	/* goto */
		case 11:	/* return */
		case 17:	/* break */
		case 18:	/* continue */
			return(1);
		return(0);
	}
	return(peeksym!=2);		/* { */
}

/* Emits a branch instruction */
branch(lab)
{
	printf("br	L%d\n", lab);
}

