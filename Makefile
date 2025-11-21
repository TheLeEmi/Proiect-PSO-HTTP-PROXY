CC = gcc
CFLAGS = -Wall -Wextra -O2

# Fișiere obiect
OBJS = http_edit.o

# Executabile generate
TARGETS = client proxy server

all: $(TARGETS)
	@echo "Build complet (client, proxy, server)."

client: client.c
	$(CC) $(CFLAGS) client.c -o client

proxy: proxy.c $(OBJS)
	$(CC) $(CFLAGS) proxy.c $(OBJS) -o proxy -lpthread
	@echo "Proxy compilat."

server: server.c
	$(CC) $(CFLAGS) server.c -o server
	@echo "Server compilat."

http_edit.o: http_edit.c http_edit.h
	$(CC) $(CFLAGS) -c http_edit.c

clean:
	rm -f $(TARGETS) *.o
	@echo "Build curățat."

run-server: server
	@echo "Pornesc serverul (port 8000)..."
	./server

run-proxy: proxy
	@echo "Pornesc proxy-ul (port 8080)..."
	./proxy

run-client: client
	@echo "Rulez clientul..."
	./client
