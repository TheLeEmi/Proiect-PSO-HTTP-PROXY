#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h> // Header necesar pentru thread-uri

#define PORT 8000

// Mutex pentru a sincroniza scrierea la stdout (consola)
pthread_mutex_t print_mutex = PTHREAD_MUTEX_INITIALIZER;

// Structura pentru a pasa argumente thread-ului (optional, aici doar socket-ul)
void *handle_client_thread(void *socket_desc) {
    int new_socket = *(int*)socket_desc;
    free(socket_desc); // Eliberam memoria alocata in main

    char buffer[2048] = {0};

    // Citim cererea
    int n = read(new_socket, buffer, sizeof(buffer));
    if (n <= 0) {
        close(new_socket);
        return NULL;
    }

    // Sectiune critica: Afisare la consola
    pthread_mutex_lock(&print_mutex);
    printf("[SERVER Thread %lu] Received request:\n%s\n", pthread_self(), buffer);
    pthread_mutex_unlock(&print_mutex);

    const char *response =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: 24\r\n"
        "\r\n"
        "<h1>Hello from Server</h1>";

    write(new_socket, response, strlen(response));
    close(new_socket);
    
    return NULL;
}

int main() {
    int server_fd, *new_sock;
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);

    // Initializare mutex (desi PTHREAD_MUTEX_INITIALIZER face asta static)
    // pthread_mutex_init(&print_mutex, NULL);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    
    // Optiune pentru a putea reporni serverul rapid
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    listen(server_fd, 5);
    printf("[SERVER] Multithreaded server listening on port %d...\n", PORT);

    while (1) {
        // Alocam memorie pentru socket ca sa evitam race condition
        new_sock = malloc(sizeof(int));
        *new_sock = accept(server_fd, (struct sockaddr*)&address, &addrlen);
        
        if (*new_sock < 0) {
            perror("accept failed");
            free(new_sock);
            continue;
        }

        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, handle_client_thread, (void*)new_sock) < 0) {
            perror("could not create thread");
            free(new_sock);
            continue;
        }

        // Detach thread pentru a elibera resursele automat cand termina
        pthread_detach(thread_id);
    }
    
    pthread_mutex_destroy(&print_mutex);
    close(server_fd);
    return 0;
}