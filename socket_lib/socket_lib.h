#ifndef SOCKET_LIB_H
#define SOCKET_LIB_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

// Structure abstraite pour représenter une adresse
typedef struct {
    char ip[INET_ADDRSTRLEN];
    int port;
} Address;

// Structure abstraite pour représenter un socket
typedef struct {
    int socket_fd;
    Address address;
    int is_server;
} Socket;

// Fonctions de création et destruction
Socket* create_socket();
void destroy_socket(Socket* sock);

// Fonctions serveur
int bind_socket(Socket* sock, int port);
int listen_socket(Socket* sock, int backlog);
Socket* accept_connection(Socket* server_sock);

// Fonctions client
int connect_socket(Socket* sock, const char* ip, int port);

// Fonctions de communication
int send_data(Socket* sock, const void* data, size_t data_size);
int receive_data(Socket* sock, void* buffer, size_t buffer_size);

// Fonctions utilitaires
int get_socket_fd(Socket* sock);
Address get_socket_address(Socket* sock);
int is_socket_valid(Socket* sock);
void close_socket(Socket* sock);

// Fonctions de gestion d'erreurs
const char* get_last_error();
void print_socket_error(const char* context);

#endif // SOCKET_LIB_H
