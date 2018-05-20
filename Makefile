CC = gcc -g -Wall -Werror

#target list
all: myshell

myshell: myshell.o LineParser.o job_control.o
	@echo "start buliding target"
	$(CC) -o myshell myshell.o LineParser.o job_control.o -lm
	@echo "finished buliding target"

myshell.o: myshell.c LineParser.o
	@echo "compiling myshell.c"
	$(CC) -o myshell.o -c myshell.c -lm
	@echo "finished compiling myshell.c"

LineParser.o: LineParser.c LineParser.h
	@echo "compiling LineParser.c"
	$(CC) -o LineParser.o -c LineParser.c -lm
	@echo "finished compiling LineParser.c"

job_control.o: job_control.c job_control.h
	@echo "compiling job_control.c"
	$(CC) -o job_control.o -c job_control.c -lm
	@echo "finished compiling job_control.c"

clean:
	rm -f *.o myshell

run:
	./myshell
