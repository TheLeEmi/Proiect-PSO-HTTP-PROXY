
#ifndef HTTP_EDIT_H
#define HTTP_EDIT_H

#include <stddef.h>

// Funcții de editare individuale
void edit_change_method(char *req);
void edit_change_url(char *req);
void edit_change_host(char *req);
void edit_add_or_replace_header(char *req);
void edit_remove_header(char *req);
void edit_modify_body(char *req);

// Funcția principală de meniu (opțional)
void edit_request_menu(char *req);

#endif
