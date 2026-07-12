CFLAGS = -std=c99 -Wall -Wextra

all: chip8

chip8: main.o instructions.o
	gcc $(CFLAGS) main.o instructions.o -o chip8 -lSDL3

main.o: main.c
	gcc $(CFLAGS) -c main.c -o main.o

instructions.o: instructions.c
	gcc $(CFLAGS) -c instructions.c -o instructions.o

clean:
	rm -f *.o chip8
