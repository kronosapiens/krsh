all: krsh

krsh: shell.c
	gcc -Wall shell.c -o krsh

clean:
	rm krsh