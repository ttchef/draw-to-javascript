
CC = x86_64-w64-mingw32-gcc

all:
	$(CC) -Iraylib/include main.c -o main.exe -L./raylib/lib -l:libraylib.a -lwinmm -lgdi32 -lopengl32 -luser32 -lkernel32
