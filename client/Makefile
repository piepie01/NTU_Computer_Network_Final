.PHONY: all clean

# compiler name
CC=g++

# place options like -O2, -g here
CFLAGS=-O2 -pthread

all:
	$(CC) -Wall -g -o client $(CFLAGS) client.c json_include/cJSON/cJSON.c connect.c command.c


clean:
	rm -f client
