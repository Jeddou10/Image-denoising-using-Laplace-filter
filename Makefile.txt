# Add additional compiler flags here
# $@ ist der Name des targets
# $^ sind die Namen aller prerequisites
# $< ist der Name der ersten prerequisite 
# target: prerequisite1 prerequisite2 ...
#	recipe1
#	recipe2
#-Wall -Wextra -Wpedantic -std=c11 -g

CC = gcc
CFLAGS=-O0  -Wall

all: a220_implementation
a220_implementation: a220_implementation.c denoise.c benchmark.c
	$(CC) $(CFLAGS) -o $@ $^
	
clean:
	rm -f a220_implementation
