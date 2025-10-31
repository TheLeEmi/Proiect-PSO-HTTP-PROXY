/*#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> // pentru read, write, close
#include <arpa/inet.h>// pentru socket, connect, inet_pton, sockaddr_in

#define PROXY_PORT 8080
//proxy care trimite aceeasi cerere test catre server/proxy
int main() {
    int sock;
    struct sockaddr_in proxy_addr;//structura cu adr IP si port-ul proxy-ului
    char buffer[4096];
    //conectare la localhost:8080
    sock = socket(AF_INET, SOCK_STREAM, 0);//socket tcp
    proxy_addr.sin_family = AF_INET; //familia ipv4
    proxy_addr.sin_port = htons(PROXY_PORT);
    inet_pton(AF_INET, "127.0.0.1", &proxy_addr.sin_addr);

    //trimit cererea de conexiune TCP catre proxy
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
}*/


// client.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PROXY_PORT 8080
#define PROXY_HOST "127.0.0.1"
#define TARGET_HOST "localhost"
#define TARGET_PORT 8000

int main() {
    char path[512];

    while (1) {
        printf("Enter path to request (e.g. /index.html or index.html) or 'q' to quit: ");
        if (!fgets(path, sizeof(path), stdin)) break;
        // remove newline
        path[strcspn(path, "\r\n")] = 0;
        if (path[0] == '\0') continue;
        if (strcmp(path, "q") == 0 || strcmp(path, "quit") == 0) break;

        // ensure path starts with '/'
        char fullpath[600];
        if (path[0] == '/') snprintf(fullpath, sizeof(fullpath), "%s", path);
        else snprintf(fullpath, sizeof(fullpath), "/%s", path);

        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) {
            perror("socket");
            continue;
        }

        struct sockaddr_in proxy_addr;
        proxy_addr.sin_family = AF_INET;
        proxy_addr.sin_port = htons(PROXY_PORT);
        if (inet_pton(AF_INET, PROXY_HOST, &proxy_addr.sin_addr) != 1) {
            perror("inet_pton");
            close(sock);
            continue;
        }

        if (connect(sock, (struct sockaddr*)&proxy_addr, sizeof(proxy_addr)) < 0) {
            perror("connect to proxy failed");
            close(sock);
            continue;
        }

        char request[1024];
        // build full URL for proxy (proxy expects URL complet)
        snprintf(request, sizeof(request),
            "GET http://%s:%d%s HTTP/1.1\r\n"
            "Host: %s:%d\r\n"
            "Connection: close\r\n"
            "\r\n",
            TARGET_HOST, TARGET_PORT, fullpath,
            TARGET_HOST, TARGET_PORT);

        if (write(sock, request, strlen(request)) < 0) {
            perror("write");
            close(sock);
            continue;
        }

        printf("----- Request sent to proxy -----\n%s", request);
        printf("----- Response start -----\n");

        // read full response until server/proxy close the connection
        char buffer[4096];
        ssize_t n;
        while ((n = read(sock, buffer, sizeof(buffer))) > 0) {
            fwrite(buffer, 1, n, stdout);
        }
        if (n < 0) perror("read");

        printf("\n----- Response end -----\n\n");
        close(sock);
    }

    printf("Client exiting.\n");
    return 0;
}
