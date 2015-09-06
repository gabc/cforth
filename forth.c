#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>

typedef struct Word {
	int id;
	char *name;
	int flags;
	int codeSize;
	struct Word **code;
	struct Word *next;
} Word;

enum {
	FALSE = 0,
	TRUE = 1,
} bool;

enum {
	EVAL = 0b0001,
	COMPILE = 0b00010,
	IMMEDIATE = 0b00100,
};

int stack[100];
Word *rstack[100];
int sp, rt;
Word *current, *dict;
char buffer[100];
int bp;
int mode = EVAL;
int nameWait = FALSE;
int cp = 0;
int idcnt = 11;

int
nextId(void)
{
	return ++idcnt;
}

void*
emalloc(int size)
{
	void *t = malloc(size);
	if(t == NULL)
		err(1, NULL);
	return t;
}

void*
ecalloc(int nb, int size)
{
	void *t = calloc(nb, size);
	if(t == NULL)
		err(1, NULL);
	return t;
}

void*
erealloc(void *ptr, int size)
{
	void *t = realloc(ptr, size);
	if(t == NULL)
		err(1, NULL);
	return t;
}

void
reset(void)
{
	printf("%s ?\n", buffer);
	bzero(buffer, bp);
	bzero(stack, sp);
	bzero(rstack, rt);
	bp = 0;
	sp = 0;
	rt = 0;
	mode = EVAL;
}

int
pop(void)
{
	int r;

	r = stack[--sp];
	if(sp < 0)
		sp = 0;
	return r;
}

void
push(int c)
{
	stack[sp] = c;
	sp++;
}

Word *
popr(void)
{
	Word *rp;
	rp = rstack[rt];
	rt--;
	if(rt < 0)
		rt = 0;
	return rp;
}

void
pushr(Word *w)
{
	rt++;
	rstack[rt] = w;
}

/* This is dumb */
void
initwords(void)
{
	dict = (Word *)emalloc(sizeof(*dict));

	dict->next = NULL;
	dict->name = "NOOP";
	dict->id = 0;

	current = (Word *)emalloc(sizeof(*current));
	current->next = dict;
	current->name = "+";
	current->id = 1;

	dict = current;

	current = (Word *)emalloc(sizeof(*current));
	current->next = dict;
	current->name = "-";
	current->id = 2;

	dict = current;

	current = (Word *)emalloc(sizeof(*current));
	current->next = dict;
	current->name = "*";
	current->id = 3;

	dict = current;

	current = (Word *)emalloc(sizeof(*current));
	current->next = dict;
	current->name = ".";
	current->id = 5;

	dict = current;

	current = (Word *)emalloc(sizeof(*current));
	current->next = dict;
	current->name = ".s";
	current->id = 6;

	dict = current;

	current = (Word *)emalloc(sizeof(*current));
	current->next = dict;
	current->name = "bye";
	current->id = 7;

	dict = current;

	current = (Word *)emalloc(sizeof(*current));
	current->next = dict;
	current->name = ":";
	current->id = 8;

	dict = current;

	current = (Word *)emalloc(sizeof(*current));
	current->next = dict;
	current->flags = IMMEDIATE;
	current->name = ";";
	current->id = 9;

	dict = current;

	current = (Word *)emalloc(sizeof(*current));
	current->next = dict;
	current->flags = 0;
	current->name = "emit";
	current->id = 10;

	dict = current;
}

Word *
newword()
{
	Word *w;
	w = (Word *)emalloc(sizeof(*w));

	/* w->name = name; */
	w->id = ++idcnt;
	w->flags = 0;
	w->codeSize = 20;
	w->code = (Word **)ecalloc(20, sizeof(Word **));
	return w;
}

void
endcolon(void)
{
	mode = EVAL;
	current->next = dict;
	dict = current;
}

int
isnum(void)
{
	int i;

	for(i = 0; i < bp; i++){
		if(buffer[i] < '0' || buffer[i] > '9')
			return FALSE;
	}
	return TRUE;
}

Word *
findword()
{
	Word *wp;

	wp = dict;

	do{
		if(strcmp(wp->name, buffer) == 0)
			return wp;
	}while((wp = wp->next) != NULL);
	return NULL;
}

Word *
findbyname(char *s)
{
	Word *wp;

	wp = dict;

	do{
		if(strcmp(wp->name, s) == 0)
			return wp;
	}while((wp = wp->next) != NULL);
	return NULL;
}

void
basic(Word *wp)
{
	int t, y;
	int pos = 0;

	if(!wp)
		return;
	switch(wp->id){
	case 0:			/* NOOP */
		popr();
		break;
	case 1:			/* + */
		push(pop() + pop());
		popr();
		break;
	case 2:			/* - */
		t = pop();
		push(pop() - t);
		popr();
		break;
	case 3:			/* * */
		push(pop() * pop());
		popr();
		break;
	case 4:			/* / */
		t = pop();
		push(pop() / t);
		popr();
		break;
	case 5:			/* . */
		printf("%d ", pop());
		popr();
		break;
	case 6:			/* .s */
		for(t = 0; t < sp; t++)
			printf("%d ", stack[t]);
		popr();
		break;
	case 7:			/* bye */
		printf("\n");
		popr();
		exit(0);
		break;
	case 8:			/* : */
		mode = COMPILE;
		nameWait = TRUE;
		current = newword();
		popr();
		break;
	case 9:			/* ; */
		mode = EVAL;
		endcolon();
		popr();
		break;
	case 10:		/* emit */
		printf("%c", pop());
		popr();
		break;
	case 11:		/* litteral */
		push(wp->codeSize);
		popr();
		break;
	default:
		for(pos = 0; pos < wp->codeSize; pos++)
			basic(wp->code[pos]);	
	}
}

void
eval(void)
{
	Word *w = NULL;

	w = findword();
	if(isnum() == 1){
		push(atoi(buffer));
		return;
	}
	if(w == NULL)
		goto fail;

	basic(w);
	return;
 fail:
	reset();
}

Word *
litteral(void)
{
	Word *lit;
	int s;
	s = atoi(buffer);
	
	lit = newword();
	lit->id = 11;
	lit->codeSize = s;
	lit->code[0] = findbyname("litteral");
	return lit;
}

void
compile(void)
{
	Word *w = NULL;

	if(nameWait){
		current->name = (char *)ecalloc(bp+1, sizeof(char *));
		sprintf(current->name, "%s", buffer);
		nameWait = FALSE;
		return;
	}
	w = findword();
	if(isnum())
		w = litteral();
	if(w == NULL){
		reset();
	}

	if(w->flags&IMMEDIATE){
		printf("ok\n");
		basic(w);
		return;
	}
	if(cp > current->codeSize){
		current->code = erealloc(current->code, current->codeSize*2);
		current->codeSize *= 2;
	}
	current->code[cp] = w;
	cp++;
}

int
main()
{
	int c;
	sp = 0;
	bp = 0;
	initwords();

	push(2);
	push(4);
	while((c = getc(stdin)) != EOF){
		if(bp > 99)
			err(1, NULL);
		if(c == '\n'){
			printf("\n");
			continue;
		}
		if(c == '\b' || c == 127) {
			buffer[bp] = 0;
			bp--;
			printf("\b \b");
			continue;
		}
		if(c == ' ') {
			printf(" ");
			if(mode == EVAL)
				eval();
			else
				compile();
			bzero(buffer, bp);
			bp = 0;
		} else {
			printf("%c", c);
			buffer[bp] = c;
			bp++;
		}
	}
	
	return 0;
}
