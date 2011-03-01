dime: dime.c dime.h util.c util.h logger.c logger.h
	gcc -Wall -g -o dime dime.c util.c logger.c
	
test: dime
	./dime -L funnier

clean:
	rm -f dime
