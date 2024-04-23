CC = g++
CFLAGS = -Wall -Wextra -std=c++17

build: server subscriber

server: server.cpp
	$(CC) $(CFLAGS) $^ -o $@

subscriber: subscriber.cpp
	$(CC) $(CFLAGS) $^ -o $@

clean:
	rm server subscriber