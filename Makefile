CC = gcc -g -Wall -Werror

#target list
all: myshell

myshell: myshell.o LineParser.o
	@echo "start buliding target"
	$(CC) -o myshell myshell.o LineParser.o -lm
	@echo "finished buliding target"

myshell.o: myshell.c LineParser.o
	@echo "compiling myshell.c"
	$(CC) -o myshell.o -c myshell.c -lm
	@echo "finished compiling myshell.c"

LineParser.o: LineParser.c LineParser.h
	@echo "compiling LineParser.c"
	$(CC) -o LineParser.o -c LineParser.c -lm
	@echo "finished compiling LineParser.c"

clean:
	rm -f *.o myshell

run:
	./myshell
