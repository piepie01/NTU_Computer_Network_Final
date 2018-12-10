.PHONY: all clean

# compiler name
CC=g++

# place options like -O2, -g here
CFLAGS=-O3 -ldl -l pthread

all: server

server: server.c json_include/cJSON/cJSON.c
	$(CC) -Wall -g -o server $(CFLAGS) server.c json_include/cJSON/cJSON.c

clean:
	rm -f server
