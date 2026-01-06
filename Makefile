CC = gcc
CFLAGS = -Wall -Wextra -O2
# Flag-ul -pthread este necesar atât pentru compilare cât și pentru linkare în aplicații multithreaded
LDFLAGS = -pthread

# Fișiere obiect
OBJS = http_edit.o

# Executabile generate
TARGETS = client proxy server

all: $(TARGETS)
	@echo "Build complet (client, proxy, server)."

client: client.c
	$(CC) $(CFLAGS) client.c -o client

# Proxy necesită obiectul http_edit și biblioteca pthread
proxy: proxy.c $(OBJS)
	$(CC) $(CFLAGS) proxy.c $(OBJS) -o proxy $(LDFLAGS)
	@echo "Proxy compilat."

# Serverul modificat folosește acum pthreads, deci are nevoie de LDFLAGS
server: server.c
	$(CC) $(CFLAGS) server.c -o server $(LDFLAGS)
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