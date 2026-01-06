#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <pthread.h>    // Threading
#include <semaphore.h>  // Semafoare

#include "http_edit.h" // Functii pentru editarea request-urilor

#define SERVER_PORT 8000
#define PROXY_PORT 8080
#define LOG_FILE "proxy.log"
#define MAX_CLIENTS 10  // Limita impusa de semafor

// --- GLOBALS PENTRU SINCRONIZARE ---
pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;     // Protejeaza scrierea in log
pthread_mutex_t console_mutex = PTHREAD_MUTEX_INITIALIZER; // Protejeaza STDIN/STDOUT pentru meniu
sem_t connection_sem;                                      // Limiteaza nr de conexiuni simultane

// Functie helper pentru timestamp
void get_timestamp(char *buffer, size_t size) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(buffer, size, "%Y-%m-%d %H:%M:%S", t);
}

// Scriere in log protejata de Mutex
void write_to_log(const char *tag, const char *message) {
    int fd;
    char log_buffer[512];
    char time_str[32];

    get_timestamp(time_str, sizeof(time_str));
    int len = snprintf(log_buffer, sizeof(log_buffer), "[%s] [%s] %s\n", time_str, tag, message);

    // CRITICAL SECTION START
    pthread_mutex_lock(&log_mutex);
    
    fd = open(LOG_FILE, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd != -1) {
        write(fd, log_buffer, len);
        close(fd);
    } else {
        perror("[SYSTEM] Error opening log file");
    }

    pthread_mutex_unlock(&log_mutex);
    // CRITICAL SECTION END
}

void forward_request(int client_sock, int server_sock, char *buffer) {
    write(server_sock, buffer, strlen(buffer));

    int n;
    // Buffer local thread-ului
    char local_buf[4096]; 
    while ((n = read(server_sock, local_buf, 4096)) > 0) {
        write(client_sock, local_buf, n);
    }
    
    // Folosim un lock rapid doar pentru printf daca vrem output curat, 
    // dar aici il lasam liber pentru performanta (doar logul e critic)
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
    write_to_log("BLOCK", "403 Forbidden sent to client.");
}

// Aceasta functie este chemata cand console_mutex este deja blocat
void edit_request(char *req) {
    write_to_log("EDIT", "User entered edit menu.");
    
    while (1) {
        printf("\n[EDIT MENU - Thread %lu]\n"
               "1. Change method\n"
               "2. Change URL\n"
               "3. Change Host\n"
               "4. Add/replace header\n"
               "5. Remove header\n"
               "6. Edit body\n"
               "0. Done\n"
               "Choice: ", pthread_self());

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
    write(server_sock, buffer, strlen(buffer)); // Send to server anyway to clear pipe
    
    const char *custom =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: 29\r\n"
        "\r\n"
        "<h1>Replaced by Proxy :)</h1>";

    write(client_sock, custom, strlen(custom));
    write_to_log("REPLACE", "Original server response replaced with custom message.");
}

void save_request(const char *buffer) {
    // Si aici folosim mutex-ul de log sau unul dedicat, dar il refolosim pe cel de log pentru simplitate
    pthread_mutex_lock(&log_mutex);
    
    int fd = open("requests.log", O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd != -1) {
        char header_msg[] = "---- New Request ----\n";
        write(fd, header_msg, strlen(header_msg));
        write(fd, buffer, strlen(buffer));
        write(fd, "\n", 1);
        close(fd);
    }
    
    pthread_mutex_unlock(&log_mutex);
    write_to_log("SAVE", "Request content saved to requests.log.");
}

// Functia executata de fiecare thread
void *handle_client_thread(void *client_sock_ptr) {
    int client_sock = *(int*)client_sock_ptr;
    free(client_sock_ptr);

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
        // Eliberam semaforul la iesire
        sem_post(&connection_sem);
        return NULL;
    }

    write_to_log("CONN", "Connected to upstream server.");

    while (1) {
        bytes_read = read(client_sock, buffer, sizeof(buffer) - 1);
        if (bytes_read <= 0) {
            write_to_log("CONN", "Client disconnected.");
            break; 
        }
        buffer[bytes_read] = '\0';

        // --- INTERACTIUNE CU UTILIZATORUL ---
        // Blocăm accesul la consolă pentru ca administratorul să se ocupe 
        // doar de ACEST request, chiar dacă vin și altele în fundal.
        pthread_mutex_lock(&console_mutex);
        
        printf("\n[PROXY Thread %lu] Received request:\n%s\n", pthread_self(), buffer);
        write_to_log("REQ", "Received new HTTP request.");

        int keep_processing = 1;
        while(keep_processing)
        {
            printf("[ACTION Thread %lu] (f)orward, (b)lock, (e)dit, (r)eplace, (s)ave, (n)ext, (q)uit client: ", pthread_self());
            char action_buffer[10];
            if (!fgets(action_buffer, sizeof(action_buffer), stdin)) break;
            char action = action_buffer[0];

            switch (action) {
                case 'f': 
                    // Deblocam consola inainte de operatiile lungi de retea
                    pthread_mutex_unlock(&console_mutex); 
                    forward_request(client_sock, server_sock, buffer);
                    // Dupa forward, trecem la asteptarea urmatorului request (iesim din loop-ul de actiuni)
                    keep_processing = 0; 
                    // Nota: Nu mai blocam consola aici, o va bloca urmatorul read loop
                    break;
                    
                case 'b': 
                    block_request(client_sock); 
                    // Dupa block, ramanem in loop sau iesim? De obicei iesim.
                    keep_processing = 0;
                    pthread_mutex_unlock(&console_mutex);
                    break;

                case 'e': 
                    // Editarea necesita consola, deci ramanem cu ea blocata
                    edit_request(buffer); 
                    // Dupa editare, re-afisam meniul (continue)
                    continue; 

                case 'r': 
                    pthread_mutex_unlock(&console_mutex);
                    replace_response(client_sock, server_sock, buffer); 
                    keep_processing = 0;
                    break;

                case 's': 
                    // Save nu necesita retea, dar e rapid.
                    save_request(buffer); 
                    printf("[PROXY] Saved.\n");
                    continue;

                case 'n':
                    printf("[PROXY] Skipping...\n");
                    keep_processing = 0;
                    pthread_mutex_unlock(&console_mutex);
                    break;

                case 'q':
                    keep_processing = 0;
                    // Fortam iesirea din while-ul exterior
                    bytes_read = 0; 
                    pthread_mutex_unlock(&console_mutex);
                    break;
                    
                case 'h': 
                     printf("\n------- SHOW REQUEST -------\n%s\n-----------------------------\n", buffer);
                     continue;

                default:
                    printf("[PROXY] Unknown action.\n");
            }
        }
        
        // Daca am iesit cu 'q' sau 'n', ne asiguram ca am deblocat mutexul
        // Verificarea e complexa aici, logica de mai sus deblocheaza pe ramurile care fac I/O
        if (bytes_read == 0) break;
    }

    close(server_sock);
    close(client_sock);
    
    // Semnalizam ca s-a eliberat un loc
    sem_post(&connection_sem);
    
    printf("[PROXY Thread %lu] Connection closed.\n", pthread_self());
    return NULL;
}

int main() {
    int proxy_fd, *client_sock;
    struct sockaddr_in proxy_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    // Initializare semafor: max MAX_CLIENTS conexiuni simultane
    sem_init(&connection_sem, 0, MAX_CLIENTS);

    proxy_fd = socket(AF_INET, SOCK_STREAM, 0);
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
    printf("[PROXY] Multithreaded Proxy listening on port %d...\n", PROXY_PORT);
    write_to_log("START", "Proxy server started.");

    while (1) {
        // Asteptam sa fie disponibil un "slot" in semafor
        sem_wait(&connection_sem);

        client_sock = malloc(sizeof(int));
        *client_sock = accept(proxy_fd, (struct sockaddr*)&client_addr, &client_len);
        
        if (*client_sock < 0) {
            perror("Accept failed");
            free(client_sock);
            sem_post(&connection_sem); // Daca esueaza, eliberam slotul
            continue;
        }

        pthread_t tid;
        if (pthread_create(&tid, NULL, handle_client_thread, client_sock) != 0) {
            perror("Failed to create thread");
            free(client_sock);
            close(*client_sock);
            sem_post(&connection_sem);
            continue;
        }

        pthread_detach(tid);
    }

    sem_destroy(&connection_sem);
    pthread_mutex_destroy(&log_mutex);
    pthread_mutex_destroy(&console_mutex);
    return 0;
}