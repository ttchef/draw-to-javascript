
CC = 

TINY_FILE_DIALOGS_PATH = libtinyfiledialogs
CFLAGS = -g -I$(TINY_FILE_DIALOGS_PATH)
LDFLAGS = 

EXE_NAME = 

OS ?= windows
ifeq ($(OS),linux)
	CC := gcc 
	CFLAGS += 
	LDFLAGS := -lraylib -lm
	EXE_NAME := main
else
	CC := x86_64-w64-mingw32-gcc
	CFLAGS += -Iraylib/include
	LDFLAGS := -L./raylib/lib -l:libraylib.a -lwinmm -lgdi32 -lopengl32 -luser32 -lkernel32 -lcomdlg32 -lole32
	EXE_NAME := main.exe
endif

all:
	$(CC) $(CFLAGS) main.c ui.c darray.c $(TINY_FILE_DIALOGS_PATH)/tinyfiledialogs.c -o $(EXE_NAME) $(LDFLAGS)

clean:
	rm -rf main main.exe
