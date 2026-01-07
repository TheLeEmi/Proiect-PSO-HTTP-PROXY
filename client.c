#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>

#define PROXY_PORT 8080
#define PROXY_HOST "127.0.0.1"
//diferite tipuri de date din cadru pt a randomiza cererea trimisa
//use case: coada de prioritati
typedef enum {
    REQ_GET,
    REQ_POST,
    REQ_PUT,
    REQ_COUNT
} HttpMethod;

const char *method_to_string(HttpMethod m) {
    switch (m) {
        case REQ_GET:  return "GET";
        case REQ_POST: return "POST";
        case REQ_PUT:  return "PUT";
        default:       return "GET";
    }
}

const char *urls[] = {
    "/",
    "/index.html",
    "/admin",
    "/login",
    "/api/data",
    "/static/img.png"
};

const char *hosts[] = {
    "localhost:8000",
    "admin.localhost:8000",
    "api.localhost:8000"
};

const char *auth_headers[] = {
    "",  // fara Authorization
    "Authorization: Bearer test123\r\n"
};

//creez cererea
void build_random_request(char *request, size_t size) {
    HttpMethod method = rand() % REQ_COUNT;
    const char *url = urls[rand() % (sizeof(urls) / sizeof(urls[0]))];
    const char *host = hosts[rand() % (sizeof(hosts) / sizeof(hosts[0]))];
    const char *auth = auth_headers[rand() % 2];

    // Body doar pentru POST / PUT
    const char *body = "";
    char body_buf[128] = "";
    int content_length = 0;

    if (method == REQ_POST || method == REQ_PUT) {
        snprintf(body_buf, sizeof(body_buf),
                 "data=example&time=%ld", time(NULL));
        body = body_buf;
        content_length = strlen(body);
    }

    snprintf(request, size,
        "%s %s HTTP/1.1\r\n"
        "Host: %s\r\n"
        "%s"
        "Content-Length: %d\r\n"
        "Connection: keep-alive\r\n"
        "\r\n"
        "%s",
        method_to_string(method),
        url,
        host,
        auth,
        content_length,
        body
    );
}


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

    srand(time(NULL));

    while (1) {
        printf("Apasa ENTER pentru request random sau q pentru quit: ");
        char input[8];
        if (!fgets(input, sizeof(input), stdin)) break;
        if (input[0] == 'q') break;
    
        char request[1024];
        build_random_request(request, sizeof(request));
    
        printf("\n----- REQUEST TRIMIS -----\n%s\n", request);
        write(sock, request, strlen(request));
    
        printf("\n----- RASPUNS -----\n");
        int n;
        while ((n = read(sock, buffer, sizeof(buffer))) > 0) {
            fwrite(buffer, 1, n, stdout);
            if (n < (int)sizeof(buffer)) break;
        }
        printf("\n-------------------\n");
    }
    

    close(sock);
    printf("Client terminat.\n");
}
