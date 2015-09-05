prog:	forth.c
	gcc -g forth.c -o forth

test:
	stty -echo; ./forth; stty echo
