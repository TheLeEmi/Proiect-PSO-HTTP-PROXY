#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8000

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);
    char buffer[2048] = {0};

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    listen(server_fd, 5);
    printf("[SERVER] Listening on port %d...\n", PORT);

    while (1) {
        new_socket = accept(server_fd, (struct sockaddr*)&address, &addrlen);
        read(new_socket, buffer, sizeof(buffer));
        printf("[SERVER] Received request:\n%s\n", buffer);

        const char *response =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html\r\n"
            "Content-Length: 24\r\n"
            "\r\n"
            "<h1>Hello from Server</h1>";

        write(new_socket, response, strlen(response));
        close(new_socket);
    }
}
