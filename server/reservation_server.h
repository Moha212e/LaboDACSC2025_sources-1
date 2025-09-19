#ifndef RESERVATION_SERVER_H
#define RESERVATION_SERVER_H

#include "../socket_lib/socket_lib.h"
#include "config_parser.h"
#include "database.h"
#include "cbp_protocol.h"
#include <pthread.h>
#include <sys/time.h>

// Structure pour représenter un patient connecté
typedef struct {
    int patient_id;
    char last_name[30];
    char first_name[30];
    char ip_address[INET_ADDRSTRLEN];
    time_t connection_time;
    Socket* socket;
} ConnectedPatient;

// Structure pour le serveur
typedef struct {
    Socket* server_socket;
    ServerConfig config;
    MYSQL* db_connection;
    pthread_t* thread_pool;
    int thread_pool_size;
    int server_running;
    pthread_mutex_t patients_mutex;
    ConnectedPatient* connected_patients;
    int max_patients;
    int current_patients;
} ReservationServer;

// Structure pour passer des données aux threads
typedef struct {
    ReservationServer* server;
    Socket* client_socket;
    int thread_id;
} ThreadData;

// Fonctions principales du serveur
ReservationServer* create_reservation_server(const char* config_file);
int start_reservation_server(ReservationServer* server);
void stop_reservation_server(ReservationServer* server);
void destroy_reservation_server(ReservationServer* server);

// Fonctions de gestion des threads
void* thread_worker(void* arg);
void handle_client_connection(ReservationServer* server, Socket* client_socket);

// Fonctions de gestion des patients connectés
int add_connected_patient(ReservationServer* server, int patient_id, const char* last_name, 
                         const char* first_name, const char* ip_address, Socket* socket);
void remove_connected_patient(ReservationServer* server, int patient_id);
ConnectedPatient* find_connected_patient(ReservationServer* server, int patient_id);
void print_connected_patients(ReservationServer* server);

// Fonctions de traitement des commandes CBP
void process_login_command(ReservationServer* server, Socket* client_socket, const CBPMessage* msg);
void process_logout_command(ReservationServer* server, Socket* client_socket, const CBPMessage* msg);
void process_get_specialties_command(ReservationServer* server, Socket* client_socket, const CBPMessage* msg);
void process_get_doctors_command(ReservationServer* server, Socket* client_socket, const CBPMessage* msg);
void process_search_consultations_command(ReservationServer* server, Socket* client_socket, const CBPMessage* msg);
void process_book_consultation_command(ReservationServer* server, Socket* client_socket, const CBPMessage* msg);

// Fonctions utilitaires
void send_error_response(Socket* client_socket, const char* error_message);
void send_success_response(Socket* client_socket, int patient_id, const char* message);

#endif // RESERVATION_SERVER_H
