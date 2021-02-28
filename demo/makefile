C_SRC = lex.c expr.c error.c decl.c stmt.c main.c
H_SRC = lex.h expr.h error.h decl.h stmt.h tokens.
INC = ./include
CC = gcc

all:
	$(CC) -o demo $(C_SRC) -I ${INC}
	cat demo.c
	./demo
	rm demo
clean:
	rm -f *.o demo

