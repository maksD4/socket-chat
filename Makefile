CC = gcc

# Server config
SERVER_CFLAGS = -std=c11 -Wall -Wextra -I. $(shell pkg-config --cflags libmongoc-1.0 libbson-1.0)
SERVER_LDFLAGS = -L. -lhiredis -Wl,-rpath='$$ORIGIN' $(shell pkg-config --libs libmongoc-1.0 libbson-1.0)

SERVER_SRC = $(shell find server -name "*.c")
SERVER_OBJ = $(SERVER_SRC:.c=.o)

SERVER_TARGET = s.out

# Server build rules
$(SERVER_TARGET): $(SERVER_OBJ)
	$(CC) $(SERVER_OBJ) $(SERVER_LDFLAGS) -o $(SERVER_TARGET)

server/%.o: server/%.c
	$(CC) $(SERVER_CFLAGS) -c $< -o $@

# Client config
CLIENT_CFLAGS = $(shell pkg-config --cflags gtk4) -I. -I./lib
CLIENT_LDFLAGS = -Wl,-rpath,/lib:/usr/lib -Wl,--disable-new-dtags $(shell pkg-config --libs gtk4)

CLIENT_SRC = $(shell find client -name "*.c")
CLIENT_OBJ = $(CLIENT_SRC:.c=.o)

CLIENT_TARGET = c.out

# Client build rules
$(CLIENT_TARGET): $(CLIENT_OBJ)
	$(CC) $(CLIENT_OBJ) $(CLIENT_LDFLAGS) -o $(CLIENT_TARGET)

client/%.o: client/%.c
	$(CC) $(CLIENT_CFLAGS) -c $< -o $@

# Targets
all: $(SERVER_TARGET)
server: $(SERVER_TARGET)
client: $(CLIENT_TARGET)
clean_client:
	rm -f $(CLIENT_OBJ) $(CLIENT_TARGET)
clean:
	rm -f $(SERVER_OBJ) $(SERVER_TARGET) $(CLIENT_OBJ) $(CLIENT_TARGET)
