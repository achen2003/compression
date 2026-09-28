CC      = gcc
CFLAGS  = -std=c11 -Wall -Wextra
TARGET  = pairs_of_bytes.exe

$(TARGET): pairs_of_bytes.c
	$(CC) $(CFLAGS) -o $(TARGET) pairs_of_bytes.c

.PHONY: clean
clean:
	rm -f $(TARGET)