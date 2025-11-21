#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PROXY_PORT 8080
#define PROXY_HOST "127.0.0.1"

int main() {
    int sock;
    struct sockaddr_in proxy_addr;
    char path[512];
    char buffer[4096];

    // creez socket o singură dată
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        return 1;
    }

    proxy_addr.sin_family = AF_INET;
    proxy_addr.sin_port = htons(PROXY_PORT);
    inet_pton(AF_INET, PROXY_HOST, &proxy_addr.sin_addr);

    if (connect(sock, (struct sockaddr*)&proxy_addr, sizeof(proxy_addr)) < 0) {
        perror("connect");
        return 1;
    }

    printf("Conexiune stabilita cu proxy-ul.\n");

    while (1) {
        printf("Path (ex: /index.html) sau q pentru quit: ");
        if (!fgets(path, sizeof(path), stdin)) break;

        path[strcspn(path, "\n")] = 0;
        if (strcmp(path, "q") == 0) break;

        if (path[0] != '/')
            snprintf(path, sizeof(path), "/%s", path);

        char request[1024];
        snprintf(request, sizeof(request),
                 "GET %s HTTP/1.1\r\n"
                 "Host: localhost:8000\r\n"
                 "Connection: keep-alive\r\n"
                 "\r\n",
                 path);

        write(sock, request, strlen(request));

        printf("\n----- Raspuns -----\n");

        int n;
        while ((n = read(sock, buffer, sizeof(buffer))) > 0) {
            fwrite(buffer, 1, n, stdout);

            if (n < sizeof(buffer)) break;
        }

        printf("\n-------------------\n");
    }

    close(sock);
    printf("Client terminat.\n");
}
