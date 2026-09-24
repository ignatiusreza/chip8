CC=g++

all: chip8 chip8_test

chip8: sdl.o
	$(CC) src/chip8.cc -o chip8 `sdl-config --cflags --libs`

chip8_test:
	$(CC) -I./test/include test/src/main.cc -o chip8_test -L./test/lib -lgtest -lpthread

