#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>      // Necesar pentru open si flag-uri
#include <sys/stat.h>   // Necesar pentru permisiuni fisier
#include <sys/types.h>
#include <time.h>       // Necesar pentru timestamp

#define SERVER_PORT 8000
#define PROXY_PORT 8080
#define LOG_FILE "proxy.log"


void edit_change_method(char *req);
void edit_change_url(char *req);
void edit_change_host(char *req);
void edit_add_or_replace_header(char *req);
void edit_remove_header(char *req);
void edit_modify_body(char *req);
// -----------------------------------------------------------------------------

// Functie helper pentru a obtine timpul curent ca string
void get_timestamp(char *buffer, size_t size) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(buffer, size, "%Y-%m-%d %H:%M:%S", t);
}

// MECANISM DE LOGGING FOLOSIND APELURI DE SISTEM
void write_to_log(const char *tag, const char *message) {
    int fd;
    char log_buffer[512]; // Buffer pentru linia de log
    char time_str[32];

    get_timestamp(time_str, sizeof(time_str));

    // Formatam linia in memorie (sprintf e safe, nu e operatie I/O pe disc)
    // Format: [YYYY-MM-DD HH:MM:SS] [TAG] Message
    int len = snprintf(log_buffer, sizeof(log_buffer), "[%s] [%s] %s\n", time_str, tag, message);

    // 1. OPEN - Deschidem fisierul folosind apel de sistem
    // O_WRONLY: Doar scriere
    // O_CREAT: Creaza fisierul daca nu exista
    // O_APPEND: Scrie la finalul fisierului
    // 0644: Permisiuni (rw-r--r--)
    fd = open(LOG_FILE, O_WRONLY | O_CREAT | O_APPEND, 0644);

    if (fd == -1) {
        perror("[SYSTEM] Error opening log file");
        return;
    }

    // 2. WRITE - Scriem efectiv folosind descriptorul de fisier
    if (write(fd, log_buffer, len) == -1) {
        perror("[SYSTEM] Error writing to log file");
    }

    // 3. CLOSE - Inchidem descriptorul
    close(fd);
}

int get_content_length(const char *headers)
{
    char *cl_ptr = strcasestr(headers, "Content-Length:");
    if(cl_ptr)
    {
        return atoi(cl_ptr + 15);
    }
    return 0;
}

void forward_request(int client_sock, int server_sock, char *buffer) {
    write(server_sock, buffer, strlen(buffer));

    int n;
    while ((n = read(server_sock, buffer, 4096)) > 0) {
        write(client_sock, buffer, n);
    }

    printf("[FORWARD] Forwarded response to client.\n");
    write_to_log("FORWARD", "Response forwarded to client successfully.");
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
    write_to_log("BLOCK", "403 Forbidden sent to client.");
}

void edit_request(char *req) {
    printf("[EDIT] Editing request before forwarding...\n");
    write_to_log("EDIT", "User entered edit menu.");
    
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
        if (!fgets(c, sizeof(c), stdin)) break;

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

    // Trimitem cererea la server (ca sa nu ramana serverul blocat asteptand), dar ignoram raspunsul
    write(server_sock, buffer, strlen(buffer));

    const char *custom =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: 29\r\n"
        "\r\n"
        "<h1>Replaced by Proxy :)</h1>";

    write(client_sock, custom, strlen(custom));
    printf("[REPLACE] Sent custom response to client.\n");
    write_to_log("REPLACE", "Original server response replaced with custom message.");
}

// Modificat pentru a folosi System Calls in loc de fopen/fprintf
void save_request(const char *buffer) {
    int fd = open("requests.log", O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd == -1) {
        perror("[SAVE] open");
        return;
    }
    
    char header_msg[] = "---- New Request ----\n";
    write(fd, header_msg, strlen(header_msg));
    write(fd, buffer, strlen(buffer));
    write(fd, "\n", 1);
    
    close(fd);
    
    printf("[SAVE] Request saved to requests.log using system calls.\n");
    write_to_log("SAVE", "Request content saved to requests.log.");
}

void handle_client(int client_sock) {
    char buffer[4096];
    int bytes_read;

    int server_sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

    if (connect(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("[PROXY] connect to server failed");
        write_to_log("ERROR", "Failed to connect to upstream server.");
        close(client_sock);
        return;
    }

    printf("[PROXY] Connected to server.\n");
    write_to_log("CONN", "Connected to upstream server.");

    while (1) {
        bytes_read = read(client_sock, buffer, sizeof(buffer) - 1);
        if (bytes_read <= 0) {
            printf("[PROXY] Client disconnected. Closing connection.\n");
            write_to_log("CONN", "Client disconnected.");
            break; 
        }

        buffer[bytes_read] = '\0';
        printf("\n[PROXY] Received request:\n%s\n", buffer);
        write_to_log("REQ", "Received new HTTP request.");

        while(1)
        {
            printf("[ACTION] (f)orward, (b)lock, (e)dit, (r)eplace, (s)ave, (n)ext ,(h)show, (q)uit: ");
            char action_buffer[10];
            if (!fgets(action_buffer, sizeof(action_buffer), stdin)) break;
            char action = action_buffer[0];

            if (action == 'q') {
                printf("[PROXY] Quit command received.\n");
                write_to_log("QUIT", "User commanded quit.");
                close(server_sock);
                close(client_sock);
                return;
            }
            if (action == 'n') {
                printf("[PROXY] Moving to next request...\n");
                break;
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
    // Adaugam optiunea SO_REUSEADDR ca sa nu primim eroare la restart rapid
    int opt = 1;
    setsockopt(proxy_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    proxy_addr.sin_family = AF_INET;
    proxy_addr.sin_addr.s_addr = INADDR_ANY;
    proxy_addr.sin_port = htons(PROXY_PORT);

    if (bind(proxy_fd, (struct sockaddr*)&proxy_addr, sizeof(proxy_addr)) < 0) {
        perror("Bind failed");
        return 1;
    }
    
    listen(proxy_fd, 5);

    printf("[PROXY] Listening on port %d...\n", PROXY_PORT);
    write_to_log("START", "Proxy server started listening.");

    while (1) {
        client_sock = accept(proxy_fd, (struct sockaddr*)&client_addr, &client_len);
        if (client_sock < 0) {
            perror("Accept failed");
            continue;
        }

        if (fork() == 0) { 
            close(proxy_fd);
            handle_client(client_sock);
            exit(0);
        }
        close(client_sock);
    }

    return 0;
}
