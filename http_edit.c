// http_edit.c
#include "http_edit.h"
#include <stdio.h>
#include <string.h>

void edit_change_method(char *req) {
    char new_method[32];

    printf("Noua metoda (GET/POST/PUT/DELETE): ");
    fgets(new_method, sizeof(new_method), stdin);
    new_method[strcspn(new_method, "\n")] = 0;

    char *sp = strchr(req, ' ');
    if (!sp) return;

    char new_req[4096];
    snprintf(new_req, sizeof(new_req), "%s%s", new_method, sp);
    strcpy(req, new_req);

    printf("[EDIT] Metoda schimbata cu succes.\n");
}
void edit_change_url(char *req) {
    char new_url[512];

    printf("Noul URL (ex: /test.html): ");
    fgets(new_url, sizeof(new_url), stdin);
    new_url[strcspn(new_url, "\n")] = 0;

    char *sp1 = strchr(req, ' ');
    char *sp2 = strchr(sp1 + 1, ' ');
    if (!sp1 || !sp2) return;

    char new_req[4096];
    snprintf(new_req, sizeof(new_req),
             "%.*s %s%s",
             (int)(sp1 - req), req, new_url, sp2);

    strcpy(req, new_req);

    printf("[EDIT] URL schimbat cu succes.\n");
}
void edit_change_host(char *req) {
    char new_host[256];

    printf("Noul Host (ex: localhost:8000): ");
    fgets(new_host, sizeof(new_host), stdin);
    new_host[strcspn(new_host, "\n")] = 0;

    char *host = strcasestr(req, "Host:");
    if (!host) return;

    char *end = strstr(host, "\r\n");
    if (!end) return;

    char new_req[4096];
    snprintf(new_req, sizeof(new_req),
             "%.*sHost: %s\r\n%s",
             (int)(host - req), req,
             new_host,
             end + 2);

    strcpy(req, new_req);

    printf("[EDIT] Host schimbat.\n");
}
void edit_add_or_replace_header(char *req) {
    char header[128], value[256];

    printf("Header nou (ex: User-Agent): ");
    fgets(header, sizeof(header), stdin);
    header[strcspn(header, "\n")] = 0;

    printf("Valoare: ");
    fgets(value, sizeof(value), stdin);
    value[strcspn(value, "\n")] = 0;

    char *pos = strcasestr(req, header);
    char *end_headers = strstr(req, "\r\n\r\n");
    if (!end_headers) return;

    char new_req[4096];

    if (pos) {
        // Replace existing header
        char *end = strstr(pos, "\r\n");
        snprintf(new_req, sizeof(new_req),
                 "%.*s%s: %s\r\n%s",
                 (int)(pos - req), req, header, value, end + 2);
    } else {
        // Add new header
        snprintf(new_req, sizeof(new_req),
                 "%.*s%s: %s\r\n%s",
                 (int)(end_headers - req), req, header, value, end_headers);
    }

    strcpy(req, new_req);
    printf("[EDIT] Header adaugat sau modificat.\n");
}
void edit_remove_header(char *req) {
    char header[128];
    printf("Header de sters (ex: Connection): ");
    fgets(header, sizeof(header), stdin);
    header[strcspn(header, "\n")] = 0;

    char *pos = strcasestr(req, header);
    if (!pos) {
        printf("[EDIT] Headerul nu exista.\n");
        return;
    }

    char *end = strstr(pos, "\r\n");
    if (!end) return;

    memmove(pos, end + 2, strlen(end + 2) + 1); // shift left ca
    //sa elimin headerul

    printf("[EDIT] Header sters.\n");
}
void edit_modify_body(char *req) {
    char *body = strstr(req, "\r\n\r\n");
    if (!body) {
        printf("[EDIT] Nu exista body. (probabil e GET)\n");
        return;
    }

    body += 4; // trec de separator

    printf("Body nou: ");
    fgets(body, 4096 - (body - req), stdin);
    body[strcspn(body, "\n")] = 0;

    printf("[EDIT] Body modificat.\n");
}
