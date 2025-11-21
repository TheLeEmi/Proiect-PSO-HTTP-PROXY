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
void forward_request(int client_sock, int server_sock, char *buffer) {
    // Trimite cererea la server
    write(server_sock, buffer, strlen(buffer));

    // Citește răspunsul de la server și îl retrimite clientului
    int n;
    while ((n = read(server_sock, buffer, 4096)) > 0) {
        write(client_sock, buffer, n);
    }

    printf("[FORWARD] Forwarded response to client.\n");
}
void block_request(int client_sock) {
    const char *response =
        "HTTP/1.1 403 Forbidden\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: 36\r\n"
        "\r\n"
        "<h1>Request blocked by proxy</h1>";

    write(client_sock, response, strlen(response));
    printf("[BLOCK] Sent 403 Forbidden to client.\n");
}
void edit_request(char *req) {
    printf("[EDIT] Editing request before forwarding...\n");
    while (1) {
        printf("\n[EDIT MENU]\n"
               "1. Change method\n"
               "2. Change URL\n"
               "3. Change Host\n"
               "4. Add/replace header\n"
               "5. Remove header\n"
               "6. Edit body\n"
               "0. Done\n"
               "Choice: ");

        char c[8];
        fgets(c, sizeof(c), stdin);

        switch (c[0]) {
            case '1': edit_change_method(req); break;
            case '2': edit_change_url(req); break;
            case '3': edit_change_host(req); break;
            case '4': edit_add_or_replace_header(req); break;
            case '5': edit_remove_header(req); break;
            case '6': edit_modify_body(req); break;
            case '0': return;
            default: printf("Optiune invalida.\n");
        }
    }
}
void replace_response(int client_sock, int server_sock, char *buffer) {
    printf("[REPLACE] Forwarding request, but replacing response...\n");

    write(server_sock, buffer, strlen(buffer));

    // ignorăm răspunsul real și trimitem altul
    const char *custom =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: 29\r\n"
        "\r\n"
        "<h1>Replaced by Proxy :)</h1>";

    write(client_sock, custom, strlen(custom));
    printf("[REPLACE] Sent custom response to client.\n");
}
void save_request(const char *buffer) {
    FILE *f = fopen("requests.log", "a");
    if (!f) {
        perror("[SAVE] fopen");
        return;
    }
    fprintf(f, "---- New Request ----\n%s\n", buffer);
    fclose(f);
    printf("[SAVE] Request saved to requests.log\n");
}
void handle_client(int client_sock) {
    char buffer[4096];
    int bytes_read;

    // conectare la server o singură dată
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

    printf("[PROXY] Connected to server.\n");

    while (1) {
        // citește o cerere HTTP completa de la client
        bytes_read = read(client_sock, buffer, sizeof(buffer) - 1);
        if (bytes_read <= 0) {
            printf("[PROXY] Client disconnected. Closing connection.\n");
            break; // ieșim din buclă → închidem sockets
        }

        buffer[bytes_read] = '\0';
        printf("\n[PROXY] Received request:\n%s\n", buffer);

        //bucla pt buffer-ul actual
        while(1)
        {
            printf("[ACTION] (f)orward, (b)lock, (e)dit, (r)eplace, (s)ave, (n)ext ,(h)show, (q)uit: ");
            char action_buffer[10];
            if (!fgets(action_buffer, sizeof(action_buffer), stdin)) break;
            char action = action_buffer[0];

            if (action == 'q') {
                printf("[PROXY] Quit command received.\n");
                close(server_sock);
                close(client_sock);
                return;
            }
            if (action == 'n') {
                printf("[PROXY] Moving to next request...\n");
                break;  // ies din bucla internă, citesc alta cerere
            }

        switch (action) {
            case 'f': forward_request(client_sock, server_sock, buffer); break;
            case 'b': block_request(client_sock); continue;
            case 'e': edit_request(buffer); continue;
            case 'r': replace_response(client_sock, server_sock, buffer); continue;;
            case 's': save_request(buffer); continue;
            case 'h': printf("\n------- SHOW REQUEST -------\n%s\n-----------------------------\n", buffer);continue;
            default:
                printf("[PROXY] Unknown action. Blocking request.\n");
                block_request(client_sock);
                break;
        }
    }
    }

    close(server_sock);
    close(client_sock);
    printf("[PROXY] Connection closed.\n");
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