
CC = gcc
CFLAGS = -Wall -Wextra -O2

# Executabilele generate
TARGETS = client proxy server


all: $(TARGETS)
	@echo "✅ Build complet! (client, proxy, server)"

client: client.c
	$(CC) $(CFLAGS) client.c -o client

proxy: proxy.c
	$(CC) $(CFLAGS) proxy.c -o proxy -lpthread

server: server.c
	$(CC) $(CFLAGS) server.c -o server

clean:
	rm -f $(TARGETS)
	@echo "🧹 Build curățat!"

# Rulează serverul în fundal
run-server: server
	@echo "🚀 Pornesc serverul (port 8000)..."
	@./server

# Rulează proxy-ul
run-proxy: proxy
	@echo "🔁 Pornesc proxy-ul (port 8080)..."
	@./proxy

# Rulează clientul
run-client: client
	@echo "💻 Trimit cererea prin proxy..."
	@./client
