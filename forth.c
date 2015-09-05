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
	int *code;
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

int stack[100], rstack[100];
int sp, rt;
Word *current, *dict;
char buffer[100];
int bp;
int mode = EVAL;
int nameWait = FALSE;
int cp = 0;
int idcnt = 0;

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
}

Word *
newword()
{
	Word *w;
	w = (Word *)emalloc(sizeof(*w));

	/* w->name = name; */
	w->id = 11;
	w->flags = 0;
	w->codeSize = 20;
	w->code = (int *)ecalloc(20, sizeof(int*));
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

void
basic(Word *wp)
{
	int t, y;
	int pos = 0;
	
	switch(wp->id){
	case 0:			/* NOOP */
		break;
	case 1:			/* + */
		push(pop() + pop());
		break;
	case 2:			/* - */
		t = pop();
		push(pop() - t);
		break;
	case 3:			/* * */
		push(pop() * pop());
		break;
	case 4:			/* / */
		t = pop();
		push(pop() / t);
		break;
	case 5:			/* . */
		printf("%d ", pop());
		break;
	case 6:			/* .s */
		for(t = 0; t < sp; t++)
			printf("%d ", stack[t]);
		break;
	case 7:			/* bye */
		exit(0);
		break;
	case 8:			/* : */
		mode = COMPILE;
		nameWait = TRUE;
		current = newword();
		break;
	case 9:			/* ; */
		mode = EVAL;
		endcolon();
		break;
	default:
		
		basic(wp->code[pos]);
	}
}

void
eval(void)
{
	Word *w = NULL;

	w = findword();
	if(w == NULL)
		return;
	if(isnum())
		push(atoi(buffer));
	basic(w);
}

void
compile(void)
{
	Word *w = NULL;

	if(nameWait){
		current->name = (char *)ecalloc(bp, sizeof(char *));
		snprintf(current->name, bp-1, "%s", buffer);
		nameWait = FALSE;
		return;
	}
	w = findword();
	if(w == NULL && isnum()){
		/* Put Litteral number */
		return;
	}
	if(w->flags&IMMEDIATE){
		printf("imme ");
		basic(w);
		return;
	}
	if(cp > current->codeSize){
		current->code = erealloc(current->code, current->codeSize*2);
		current->codeSize *= 2;
	}
	current->code[cp] = w->id;
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
		if(c == '\n')
			continue;
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
