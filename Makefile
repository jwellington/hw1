dime: dime.c dime.h util.c util.h
	gcc -Wall -g -o dime dime.c util.c
	
test: dime
	./dime -L funnier

clean:
	rm -f dime

infiniteloop1: infiniteloop2
	echo This will never end.

infiniteloop2: infiniteloop1
	echo This will never end?
