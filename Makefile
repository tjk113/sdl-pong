CC = gcc
S = src/
I = include/
B = bin/

FLAGS = -Iinclude/SDL2/include -Linclude/SDL2/lib/x64 -lSDL2main -lSDL2 -mwindows -std=c17

$(B)pong.exe: $(B)main.o
	$(CC) $^ -o $@ $(FLAGS)

$(B)main.o: $(S)main.c
	$(CC) -c $^ -o $@ $(FLAGS)


debug: FLAGS += -g
debug: $(B)pong.exe

opt: FLAGS += -O3
opt: $(B)pong.exe

exe:
	$(RM) $(B)main.o

clean:
	$(RM) $(B)pong.exe $(B)main.o