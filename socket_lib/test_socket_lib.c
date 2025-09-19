#include "socket_lib.h"
#include <assert.h>
#include <unistd.h>
#include <sys/wait.h>

// Test de création et destruction de socket
void test_create_destroy_socket() {
    printf("Test: Création et destruction de socket...\n");
    
    Socket* sock = create_socket();
    assert(sock != NULL);
    assert(is_socket_valid(sock));
    assert(get_socket_fd(sock) >= 0);
    
    destroy_socket(sock);
    printf("✓ Test création/destruction réussi\n\n");
}

// Test de bind et listen
void test_bind_listen() {
    printf("Test: Bind et listen...\n");
    
    Socket* server = create_socket();
    assert(server != NULL);
    
    // Test bind
    int result = bind_socket(server, 8080);
    assert(result == 0);
    assert(server->is_server == 1);
    assert(server->address.port == 8080);
    
    // Test listen
    result = listen_socket(server, 5);
    assert(result == 0);
    
    destroy_socket(server);
    printf("✓ Test bind/listen réussi\n\n");
}

// Test de connexion client-serveur
void test_client_server_connection() {
    printf("Test: Connexion client-serveur...\n");
    
    pid_t pid = fork();
    
    if (pid == 0) {
        // Processus enfant (serveur)
        Socket* server = create_socket();
        assert(server != NULL);
        
        assert(bind_socket(server, 8081) == 0);
        assert(listen_socket(server, 5) == 0);
        
        Socket* client = accept_connection(server);
        assert(client != NULL);
        assert(is_socket_valid(client));
        
        // Envoyer un message de test
        const char* message = "Hello from server!";
        int sent = send_data(client, message, strlen(message));
        assert(sent == strlen(message));
        
        destroy_socket(client);
        destroy_socket(server);
        exit(0);
        
    } else {
        // Processus parent (client)
        sleep(1); // Attendre que le serveur soit prêt
        
        Socket* client = create_socket();
        assert(client != NULL);
        
        int result = connect_socket(client, "127.0.0.1", 8081);
        assert(result == 0);
        
        // Recevoir le message
        char buffer[256];
        int received = receive_data(client, buffer, sizeof(buffer) - 1);
        assert(received > 0);
        buffer[received] = '\0';
        
        assert(strcmp(buffer, "Hello from server!") == 0);
        
        destroy_socket(client);
        
        // Attendre la fin du processus enfant
        int status;
        wait(&status);
        
        printf("✓ Test connexion client-serveur réussi\n\n");
    }
}

// Test de gestion d'erreurs
void test_error_handling() {
    printf("Test: Gestion d'erreurs...\n");
    
    // Test avec socket NULL
    Socket* null_sock = NULL;
    assert(bind_socket(null_sock, 8080) == -1);
    assert(strlen(get_last_error()) > 0);
    
    // Test avec port déjà utilisé
    Socket* server1 = create_socket();
    Socket* server2 = create_socket();
    
    assert(bind_socket(server1, 8082) == 0);
    assert(bind_socket(server2, 8082) == -1); // Port déjà utilisé
    
    destroy_socket(server1);
    destroy_socket(server2);
    
    printf("✓ Test gestion d'erreurs réussi\n\n");
}

// Test de communication bidirectionnelle
void test_bidirectional_communication() {
    printf("Test: Communication bidirectionnelle...\n");
    
    pid_t pid = fork();
    
    if (pid == 0) {
        // Serveur
        Socket* server = create_socket();
        bind_socket(server, 8083);
        listen_socket(server, 5);
        
        Socket* client = accept_connection(server);
        
        // Recevoir et renvoyer
        char buffer[256];
        int received = receive_data(client, buffer, sizeof(buffer) - 1);
        buffer[received] = '\0';
        
        // Renvoyer en majuscules
        for (int i = 0; i < received; i++) {
            if (buffer[i] >= 'a' && buffer[i] <= 'z') {
                buffer[i] = buffer[i] - 'a' + 'A';
            }
        }
        
        send_data(client, buffer, received);
        
        destroy_socket(client);
        destroy_socket(server);
        exit(0);
        
    } else {
        // Client
        sleep(1);
        
        Socket* client = create_socket();
        connect_socket(client, "127.0.0.1", 8083);
        
        const char* message = "hello world";
        send_data(client, message, strlen(message));
        
        char response[256];
        int received = receive_data(client, response, sizeof(response) - 1);
        response[received] = '\0';
        
        assert(strcmp(response, "HELLO WORLD") == 0);
        
        destroy_socket(client);
        
        int status;
        wait(&status);
        
        printf("✓ Test communication bidirectionnelle réussi\n\n");
    }
}

int main() {
    printf("=== Tests de la librairie de sockets ===\n\n");
    
    test_create_destroy_socket();
    test_bind_listen();
    test_client_server_connection();
    test_error_handling();
    test_bidirectional_communication();
    
    printf("=== Tous les tests sont passés avec succès! ===\n");
    return 0;
}
