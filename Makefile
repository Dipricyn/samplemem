# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -O2

# Source and target
SRC = main.c
OUT = samplemem

# Default target
all: $(OUT)

$(OUT): $(SRC) Makefile
	$(CC) $(CFLAGS) -o $(OUT) $(SRC)

clean:
	rm -f $(OUT)

# Phony targets (not files)
.PHONY: all clean
