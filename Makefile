
CC = x86_64-w64-mingw32-gcc

TINY_FILE_DIALOGS_PATH = libtinyfiledialogs
CLFAGS = -g -Iraylib/include -I$(TINY_FILE_DIALOGS_PATH)
LDFLAGS = -L./raylib/lib -l:libraylib.a -lwinmm -lgdi32 -lopengl32 -luser32 -lkernel32 -lcomdlg32 -lole32

all:
	$(CC) -Iraylib/include main.c ui.c $(TINY_FILE_DIALOGS_PATH)/tinyfiledialogs.c -o main.exe $(LDFLAGS)
