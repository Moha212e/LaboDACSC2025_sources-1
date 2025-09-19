#include "socket_lib.h"

// Variable globale pour stocker la dernière erreur
static char last_error[256] = {0};

// Fonction pour définir l'erreur
static void set_error(const char* error_msg) {
    strncpy(last_error, error_msg, sizeof(last_error) - 1);
    last_error[sizeof(last_error) - 1] = '\0';
}

// Fonction pour définir l'erreur avec errno
static void set_error_with_errno(const char* context) {
    snprintf(last_error, sizeof(last_error), "%s: %s", context, strerror(errno));
}

// Créer un nouveau socket
Socket* create_socket() {
    Socket* sock = (Socket*)malloc(sizeof(Socket));
    if (!sock) {
        set_error("Erreur d'allocation mémoire pour le socket");
        return NULL;
    }
    
    sock->socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock->socket_fd < 0) {
        set_error_with_errno("Erreur de création du socket");
        free(sock);
        return NULL;
    }
    
    sock->address.port = 0;
    strcpy(sock->address.ip, "0.0.0.0");
    sock->is_server = 0;
    
    return sock;
}

// Détruire un socket
void destroy_socket(Socket* sock) {
    if (sock) {
        if (sock->socket_fd >= 0) {
            close(sock->socket_fd);
        }
        free(sock);
    }
}

// Lier un socket à un port (pour serveur)
int bind_socket(Socket* sock, int port) {
    if (!sock || sock->socket_fd < 0) {
        set_error("Socket invalide");
        return -1;
    }
    
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    
    if (bind(sock->socket_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        set_error_with_errno("Erreur de bind");
        return -1;
    }
    
    sock->address.port = port;
    strcpy(sock->address.ip, "0.0.0.0");
    sock->is_server = 1;
    
    return 0;
}

// Mettre le socket en mode écoute
int listen_socket(Socket* sock, int backlog) {
    if (!sock || sock->socket_fd < 0) {
        set_error("Socket invalide");
        return -1;
    }
    
    if (listen(sock->socket_fd, backlog) < 0) {
        set_error_with_errno("Erreur de listen");
        return -1;
    }
    
    return 0;
}

// Accepter une connexion (pour serveur)
Socket* accept_connection(Socket* server_sock) {
    if (!server_sock || server_sock->socket_fd < 0 || !server_sock->is_server) {
        set_error("Socket serveur invalide");
        return NULL;
    }
    
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    
    int client_fd = accept(server_sock->socket_fd, (struct sockaddr*)&client_addr, &client_len);
    if (client_fd < 0) {
        set_error_with_errno("Erreur d'accept");
        return NULL;
    }
    
    Socket* client_sock = (Socket*)malloc(sizeof(Socket));
    if (!client_sock) {
        set_error("Erreur d'allocation mémoire pour le socket client");
        close(client_fd);
        return NULL;
    }
    
    client_sock->socket_fd = client_fd;
    client_sock->is_server = 0;
    
    // Récupérer l'adresse IP du client
    inet_ntop(AF_INET, &client_addr.sin_addr, client_sock->address.ip, INET_ADDRSTRLEN);
    client_sock->address.port = ntohs(client_addr.sin_port);
    
    return client_sock;
}

// Se connecter à un serveur (pour client)
int connect_socket(Socket* sock, const char* ip, int port) {
    if (!sock || sock->socket_fd < 0) {
        set_error("Socket invalide");
        return -1;
    }
    
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    
    if (inet_pton(AF_INET, ip, &addr.sin_addr) <= 0) {
        set_error("Adresse IP invalide");
        return -1;
    }
    
    if (connect(sock->socket_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        set_error_with_errno("Erreur de connexion");
        return -1;
    }
    
    strncpy(sock->address.ip, ip, INET_ADDRSTRLEN - 1);
    sock->address.ip[INET_ADDRSTRLEN - 1] = '\0';
    sock->address.port = port;
    
    return 0;
}

// Envoyer des données
int send_data(Socket* sock, const void* data, size_t data_size) {
    if (!sock || sock->socket_fd < 0) {
        set_error("Socket invalide");
        return -1;
    }
    
    ssize_t bytes_sent = send(sock->socket_fd, data, data_size, 0);
    if (bytes_sent < 0) {
        set_error_with_errno("Erreur d'envoi");
        return -1;
    }
    
    return (int)bytes_sent;
}

// Recevoir des données
int receive_data(Socket* sock, void* buffer, size_t buffer_size) {
    if (!sock || sock->socket_fd < 0) {
        set_error("Socket invalide");
        return -1;
    }
    
    ssize_t bytes_received = recv(sock->socket_fd, buffer, buffer_size, 0);
    if (bytes_received < 0) {
        set_error_with_errno("Erreur de réception");
        return -1;
    }
    
    return (int)bytes_received;
}

// Obtenir le descripteur de fichier du socket
int get_socket_fd(Socket* sock) {
    if (!sock) return -1;
    return sock->socket_fd;
}

// Obtenir l'adresse du socket
Address get_socket_address(Socket* sock) {
    Address empty = {0};
    if (!sock) return empty;
    return sock->address;
}

// Vérifier si le socket est valide
int is_socket_valid(Socket* sock) {
    return (sock && sock->socket_fd >= 0);
}

// Fermer le socket
void close_socket(Socket* sock) {
    if (sock && sock->socket_fd >= 0) {
        close(sock->socket_fd);
        sock->socket_fd = -1;
    }
}

// Obtenir la dernière erreur
const char* get_last_error() {
    return last_error;
}

// Afficher l'erreur du socket
void print_socket_error(const char* context) {
    printf("Erreur %s: %s\n", context, get_last_error());
}
