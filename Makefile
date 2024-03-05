CC = gcc
TARGET = float
SRC = float.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) -o $@ $<

clean:
	rm -f $(TARGET)

.PHONY: all clean