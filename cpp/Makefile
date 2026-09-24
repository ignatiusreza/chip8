CC=g++

all: chip8 chip8_test

chip8: src/chip8.cc src/*.h
	$(CC) src/chip8.cc -o chip8 `sdl-config --cflags --libs`

# uses the system googletest (e.g. libgtest-dev); the copy under ./test is for VisualStudio
chip8_test: test/src/*.cc test/src/*.h src/stack.h
	$(CC) test/src/main.cc -o chip8_test -lgtest -lpthread
