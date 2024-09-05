# Compiler
CC = gcc
CFLAGS = -Wall -Werror -Isrc/include -lpthread -lrt -O2

# Directories
SRCDIR = src
INCDIR = include
SERVERDIR = $(SRCDIR)/server
CLIENTDIR = $(SRCDIR)/client

# Source files
SERVER_SRC = $(SERVERDIR)/server.c $(SERVERDIR)/hashtable.c
CLIENT_SRC = $(CLIENTDIR)/client.c
CLIENT_SR_SRC = $(CLIENTDIR)/clientSingleRequest.c

# Output executables
SERVER_OUT = server
CLIENT_OUT = client
CLIENT_SR_OUT = clientSingleRequest

# Targets
all: $(SERVER_OUT) $(CLIENT_OUT) $(CLIENT_SR_OUT)

# Build server
$(SERVER_OUT): $(SERVER_SRC)
	$(CC) $(CFLAGS) -o $(SERVER_OUT) $(SERVER_SRC)

# Build client
$(CLIENT_OUT): $(CLIENT_SRC)
	$(CC) $(CFLAGS) -o $(CLIENT_OUT) $(CLIENT_SRC)

# Build clientSingleRequest
$(CLIENT_SR_OUT): $(CLIENT_SR_SRC)
	$(CC) $(CFLAGS) -o $(CLIENT_SR_OUT) $(CLIENT_SR_SRC)

# Clean
clean:
	rm -f $(SERVER_OUT) $(CLIENT_OUT) $(CLIENT_SR_OUT)

# Phony targets
.PHONY: all clean

