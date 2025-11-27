CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -I. -L. $(shell pkg-config --cflags libmongoc-1.0 libbson-1.0)
LDFLAGS = -L. -lhiredis -Wl,-rpath='$$ORIGIN' $(shell pkg-config --libs libmongoc-1.0 libbson-1.0)

SRC = $(shell find server -name "*.c")
OBJ = $(SRC:.c=.o)

TARGET = a.out

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(OBJ) $(LDFLAGS) -o $(TARGET)

clean:
	rm -f $(OBJ) $(TARGET)
