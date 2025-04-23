# Makefile for running O.S. Lab 3: A Simple File System

# The target to build the program
build:
	gcc -o filesys filesys.c

# Clean up the generated files
clean:
	rm -f main
