CC=clang

all:
	$(CC) myshell.c -o myshell -Wall -lreadline
clean:
	rm -rf myshell
