
CC = gcc

TINY_FILE_DIALOGS_PATH = libtinyfiledialogs
CLFAGS = -g -I$(TINY_FILE_DIALOGS_PATH)
LDFLAGS = -lraylib -lm

all:
	$(CC) main.c ui.c darray.c $(TINY_FILE_DIALOGS_PATH)/tinyfiledialogs.c -o main.exe $(LDFLAGS)
