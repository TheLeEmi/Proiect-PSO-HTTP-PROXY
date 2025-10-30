#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define SERVER_PORT 8000   // unde trimite cererile
#define PROXY_PORT 8080    // portul pe care ascultă proxy


//Functie pentru a gasi content lenght in cadrul pachetului HTTP
int get_content_length(const char *headers)
{
    char *cl_ptr = strcasestr(headers, "Content-Length:"); ///strcasestr nu e case sensitive
    if(cl_ptr)
    {
        return atoi(cl_ptr + 15); //Sare peste "Content-Length:"
    }

    return 0;
}

void handle_client(int client_sock) {
    char buffer[4096];
    int bytes_read;

    // citește cererea de la client
    bytes_read = read(client_sock, buffer, sizeof(buffer));
    if (bytes_read <= 0) {
        close(client_sock);
        return;
    }

    buffer[bytes_read] = '\0';
    printf("[PROXY] Received request:\n%s\n", buffer);

    // conectare la server
    int server_sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

    if (connect(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("[PROXY] connect to server failed");
        close(client_sock);
        return;
    }

    printf("[ACTION] (f)orward, (b)lock, (e)dit, (r)eplace, (s)ave: ");
    char action_buffer[10];
    fgets(action_buffer, sizeof(action_buffer), stdin);
    char action = action_buffer[0];



    switch (action) {
        case 'f':
            // forward cererea
            write(server_sock, buffer, strlen(buffer));

            // citește răspunsul serverului și îl retrimite clientului
            int n;
            while ((n = read(server_sock, buffer, sizeof(buffer))) > 0) {
                write(client_sock, buffer, n);
            }

            printf("[PROXY] Forwarded response to client.\n");
            break;

        case 'b':
            //(Block)
            break;

        case 'e':
            // (Edit)
            // Dupa edit => forward
            break;

        case 'r':
            //Replace)
            // Dupa replace => forward
            break;
        
        case 's':
            //(Save)
            // Dupa salvare => actiunea salvata
            break;

        default:
            printf("[PROXY] Acțiune necunoscută. Se blochează cererea.\n");
            // Block by default
            break;
    }

    // Logica de inchidere
    if (server_sock > 0) {
        close(server_sock);
    }
    close(client_sock);

}

int main() {
    int proxy_fd, client_sock;
    struct sockaddr_in proxy_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    proxy_fd = socket(AF_INET, SOCK_STREAM, 0);
    proxy_addr.sin_family = AF_INET;
    proxy_addr.sin_addr.s_addr = INADDR_ANY;
    proxy_addr.sin_port = htons(PROXY_PORT);

    bind(proxy_fd, (struct sockaddr*)&proxy_addr, sizeof(proxy_addr));
    listen(proxy_fd, 5);

    printf("[PROXY] Listening on port %d...\n", PROXY_PORT);

    while (1) {
        client_sock = accept(proxy_fd, (struct sockaddr*)&client_addr, &client_len);
        if (fork() == 0) { // proces separat pt client
            close(proxy_fd);
            handle_client(client_sock);
            exit(0);
        }
        close(client_sock);
    }

    return 0;
}
