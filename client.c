#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PROXY_PORT 8080

int main() {
    int sock;
    struct sockaddr_in proxy_addr;
    char buffer[4096];

    sock = socket(AF_INET, SOCK_STREAM, 0);
    proxy_addr.sin_family = AF_INET;
    proxy_addr.sin_port = htons(PROXY_PORT);
    inet_pton(AF_INET, "127.0.0.1", &proxy_addr.sin_addr);

    if (connect(sock, (struct sockaddr*)&proxy_addr, sizeof(proxy_addr)) < 0) {
        perror("connect failed");
        exit(EXIT_FAILURE);
    }

    // Cerere HTTP completă (cu URL complet)
    char *request =
        "GET http://localhost:8000/index.html HTTP/1.1\r\n"
        "Host: localhost:8000\r\n"
        "Connection: close\r\n\r\n";

    write(sock, request, strlen(request));
    printf("[CLIENT] Sent request:\n%s\n", request);

    int n = read(sock, buffer, sizeof(buffer) - 1);
    buffer[n] = '\0';
    printf("[CLIENT] Response from proxy:\n%s\n", buffer);

    close(sock);
    return 0;
}
