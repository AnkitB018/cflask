CFLAGS = -Wall -Wextra
TARGET = bin/cflask
SRCS = cflask.c http-parser.c functions.c

all: $(TARGET)

$(TARGET): $(SRCS)
	mkdir -p bin
	gcc $(CFLAGS) $(SRCS) -o $(TARGET)

clean:
	rm -R bin

.PHONY: all clean
