#include "reservation_server.h"
#include <signal.h>
#include <unistd.h>
#include <errno.h>

// Variable globale pour le serveur (pour gestion des signaux)
static ReservationServer* g_server = NULL;

// Gestionnaire de signal pour arrêt propre
void signal_handler(int sig) {
    if (g_server) {
        printf("\nSignal reçu (%d), arrêt du serveur...\n", sig);
        stop_reservation_server(g_server);
    }
}

// Créer le serveur de réservation
ReservationServer* create_reservation_server(const char* config_file) {
    ReservationServer* server = (ReservationServer*)malloc(sizeof(ReservationServer));
    if (!server) {
        printf("Erreur: Impossible d'allouer la mémoire pour le serveur\n");
        return NULL;
    }
    
    // Initialiser les valeurs par défaut
    server->server_socket = NULL;
    server->db_connection = NULL;
    server->thread_pool = NULL;
    server->thread_pool_size = 0;
    server->server_running = 0;
    server->connected_patients = NULL;
    server->max_patients = 100;
    server->current_patients = 0;
    
    // Initialiser le mutex
    if (pthread_mutex_init(&server->patients_mutex, NULL) != 0) {
        printf("Erreur: Impossible d'initialiser le mutex\n");
        free(server);
        return NULL;
    }
    
    // Charger la configuration
    if (load_config(config_file, &server->config) != 0) {
        printf("Erreur: Impossible de charger la configuration\n");
        pthread_mutex_destroy(&server->patients_mutex);
        free(server);
        return NULL;
    }
    
    // Allouer la mémoire pour les patients connectés
    server->connected_patients = (ConnectedPatient*)calloc(server->max_patients, sizeof(ConnectedPatient));
    if (!server->connected_patients) {
        printf("Erreur: Impossible d'allouer la mémoire pour les patients connectés\n");
        pthread_mutex_destroy(&server->patients_mutex);
        free(server);
        return NULL;
    }
    
    // Se connecter à la base de données
    server->db_connection = connect_database(server->config.db_host, 
                                           server->config.db_user, 
                                           server->config.db_password, 
                                           server->config.db_name);
    if (!server->db_connection) {
        printf("Erreur: Impossible de se connecter à la base de données\n");
        free(server->connected_patients);
        pthread_mutex_destroy(&server->patients_mutex);
        free(server);
        return NULL;
    }
    
    // Créer le socket serveur
    server->server_socket = create_socket();
    if (!server->server_socket) {
        printf("Erreur: Impossible de créer le socket serveur\n");
        disconnect_database(server->db_connection);
        free(server->connected_patients);
        pthread_mutex_destroy(&server->patients_mutex);
        free(server);
        return NULL;
    }
    
    // Configurer le serveur
    if (bind_socket(server->server_socket, server->config.port_reservation) != 0) {
        printf("Erreur: Impossible de lier le socket au port %d\n", server->config.port_reservation);
        destroy_socket(server->server_socket);
        disconnect_database(server->db_connection);
        free(server->connected_patients);
        pthread_mutex_destroy(&server->patients_mutex);
        free(server);
        return NULL;
    }
    
    if (listen_socket(server->server_socket, 10) != 0) {
        printf("Erreur: Impossible de mettre le socket en mode écoute\n");
        destroy_socket(server->server_socket);
        disconnect_database(server->db_connection);
        free(server->connected_patients);
        pthread_mutex_destroy(&server->patients_mutex);
        free(server);
        return NULL;
    }
    
    printf("Serveur de réservation créé avec succès\n");
    print_config(&server->config);
    
    return server;
}

// Démarrer le serveur
int start_reservation_server(ReservationServer* server) {
    if (!server) return -1;
    
    // Allouer la mémoire pour le pool de threads
    server->thread_pool_size = server->config.thread_pool_size;
    server->thread_pool = (pthread_t*)malloc(server->thread_pool_size * sizeof(pthread_t));
    if (!server->thread_pool) {
        printf("Erreur: Impossible d'allouer la mémoire pour le pool de threads\n");
        return -1;
    }
    
    // Configurer le gestionnaire de signaux
    g_server = server;
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // Créer les threads du pool
    for (int i = 0; i < server->thread_pool_size; i++) {
        ThreadData* thread_data = (ThreadData*)malloc(sizeof(ThreadData));
        thread_data->server = server;
        thread_data->thread_id = i;
        thread_data->client_socket = NULL;
        
        if (pthread_create(&server->thread_pool[i], NULL, thread_worker, thread_data) != 0) {
            printf("Erreur: Impossible de créer le thread %d\n", i);
            server->server_running = 0;
            return -1;
        }
    }
    
    server->server_running = 1;
    printf("Serveur démarré sur le port %d avec %d threads\n", 
           server->config.port_reservation, server->thread_pool_size);
    
    // Boucle principale d'acceptation des connexions
    while (server->server_running) {
        Socket* client_socket = accept_connection(server->server_socket);
        if (client_socket) {
            printf("Nouvelle connexion acceptée de %s:%d\n", 
                   client_socket->address.ip, client_socket->address.port);
            
            // Trouver un thread libre pour traiter cette connexion
            // Pour simplifier, on utilise le premier thread disponible
            // Dans une implémentation plus avancée, on utiliserait une queue de tâches
            handle_client_connection(server, client_socket);
        } else if (server->server_running) {
            printf("Erreur lors de l'acceptation de la connexion\n");
        }
    }
    
    return 0;
}

// Arrêter le serveur
void stop_reservation_server(ReservationServer* server) {
    if (!server) return;
    
    server->server_running = 0;
    
    // Attendre que tous les threads se terminent
    for (int i = 0; i < server->thread_pool_size; i++) {
        pthread_join(server->thread_pool[i], NULL);
    }
    
    printf("Serveur arrêté\n");
}

// Détruire le serveur
void destroy_reservation_server(ReservationServer* server) {
    if (!server) return;
    
    // Fermer toutes les connexions clients
    pthread_mutex_lock(&server->patients_mutex);
    for (int i = 0; i < server->max_patients; i++) {
        if (server->connected_patients[i].socket) {
            close_socket(server->connected_patients[i].socket);
            destroy_socket(server->connected_patients[i].socket);
        }
    }
    pthread_mutex_unlock(&server->patients_mutex);
    
    // Nettoyer les ressources
    if (server->server_socket) {
        destroy_socket(server->server_socket);
    }
    
    if (server->db_connection) {
        disconnect_database(server->db_connection);
    }
    
    if (server->thread_pool) {
        free(server->thread_pool);
    }
    
    if (server->connected_patients) {
        free(server->connected_patients);
    }
    
    pthread_mutex_destroy(&server->patients_mutex);
    free(server);
}

// Fonction de travail des threads
void* thread_worker(void* arg) {
    ThreadData* data = (ThreadData*)arg;
    ReservationServer* server = data->server;
    
    printf("Thread %d démarré\n", data->thread_id);
    
    // Dans cette implémentation simplifiée, les threads attendent
    // Dans une version plus avancée, ils traiteraient une queue de tâches
    while (server->server_running) {
        usleep(100000); // 100ms
    }
    
    printf("Thread %d terminé\n", data->thread_id);
    free(data);
    return NULL;
}

// Gérer une connexion client
void handle_client_connection(ReservationServer* server, Socket* client_socket) {
    char buffer[CBP_MAX_MESSAGE_SIZE];
    CBPMessage message;
    
    printf("Traitement de la connexion client...\n");
    
    while (server->server_running) {
        // Recevoir un message
        int bytes_received = receive_data(client_socket, buffer, sizeof(buffer));
        if (bytes_received <= 0) {
            printf("Connexion fermée par le client\n");
            break;
        }
        
        // Désérialiser le message
        if (deserialize_cbp_message(buffer, bytes_received, &message) <= 0) {
            printf("Erreur: Impossible de désérialiser le message\n");
            send_error_response(client_socket, "Message invalide");
            continue;
        }
        
        printf("Message reçu: %s\n", cbp_command_to_string(message.command));
        
        // Traiter la commande
        switch (message.command) {
            case CBP_LOGIN:
                process_login_command(server, client_socket, &message);
                break;
            case CBP_LOGOUT:
                process_logout_command(server, client_socket, &message);
                break;
            case CBP_GET_SPECIALTIES:
                process_get_specialties_command(server, client_socket, &message);
                break;
            case CBP_GET_DOCTORS:
                process_get_doctors_command(server, client_socket, &message);
                break;
            case CBP_SEARCH_CONSULTATIONS:
                process_search_consultations_command(server, client_socket, &message);
                break;
            case CBP_BOOK_CONSULTATION:
                process_book_consultation_command(server, client_socket, &message);
                break;
            default:
                printf("Commande inconnue: %d\n", message.command);
                send_error_response(client_socket, "Commande inconnue");
                break;
        }
    }
    
    // Fermer la connexion
    close_socket(client_socket);
    destroy_socket(client_socket);
}

// Traiter la commande LOGIN
void process_login_command(ReservationServer* server, Socket* client_socket, const CBPMessage* msg) {
    CBPLoginData login_data;
    CBPResponse response;
    
    if (deserialize_login_data(msg->data, msg->data_length, &login_data) <= 0) {
        send_error_response(client_socket, "Données de login invalides");
        return;
    }
    
    printf("Tentative de login: %s %s (ID: %d, Nouveau: %d)\n", 
           login_data.last_name, login_data.first_name, 
           login_data.patient_id, login_data.is_new_patient);
    
    if (login_data.is_new_patient) {
        // Créer un nouveau patient
        int new_patient_id = create_patient(server->db_connection, 
                                          login_data.last_name, 
                                          login_data.first_name, 
                                          "1990-01-01"); // Date par défaut
        
        if (new_patient_id > 0) {
            response.success = 1;
            response.patient_id = new_patient_id;
            strcpy(response.message, "Nouveau patient créé avec succès");
            
            // Ajouter le patient aux patients connectés
            add_connected_patient(server, new_patient_id, 
                                login_data.last_name, 
                                login_data.first_name, 
                                client_socket->address.ip, 
                                client_socket);
        } else {
            response.success = 0;
            response.patient_id = 0;
            strcpy(response.message, "Erreur lors de la création du patient");
        }
    } else {
        // Authentifier le patient existant
        if (authenticate_patient(server->db_connection, 
                               login_data.last_name, 
                               login_data.first_name, 
                               login_data.patient_id)) {
            response.success = 1;
            response.patient_id = login_data.patient_id;
            strcpy(response.message, "Authentification réussie");
            
            // Ajouter le patient aux patients connectés
            add_connected_patient(server, login_data.patient_id, 
                                login_data.last_name, 
                                login_data.first_name, 
                                client_socket->address.ip, 
                                client_socket);
        } else {
            response.success = 0;
            response.patient_id = 0;
            strcpy(response.message, "Authentification échouée");
        }
    }
    
    // Envoyer la réponse
    char response_buffer[CBP_MAX_MESSAGE_SIZE];
    int response_size = serialize_response(&response, response_buffer, sizeof(response_buffer));
    
    CBPMessage response_msg;
    response_msg.command = CBP_LOGIN;
    response_msg.data_length = response_size;
    memcpy(response_msg.data, response_buffer, response_size);
    
    char final_buffer[CBP_MAX_MESSAGE_SIZE];
    int final_size = serialize_cbp_message(&response_msg, final_buffer, sizeof(final_buffer));
    
    send_data(client_socket, final_buffer, final_size);
}

// Traiter la commande LOGOUT
void process_logout_command(ReservationServer* server, Socket* client_socket, const CBPMessage* msg) {
    // Pour simplifier, on considère que le client qui se déconnecte est le patient connecté
    // Dans une vraie implémentation, on identifierait le patient par son ID
    
    pthread_mutex_lock(&server->patients_mutex);
    for (int i = 0; i < server->max_patients; i++) {
        if (server->connected_patients[i].socket == client_socket) {
            printf("Patient %d (%s %s) se déconnecte\n", 
                   server->connected_patients[i].patient_id,
                   server->connected_patients[i].last_name,
                   server->connected_patients[i].first_name);
            
            server->connected_patients[i].patient_id = 0;
            server->connected_patients[i].socket = NULL;
            server->current_patients--;
            break;
        }
    }
    pthread_mutex_unlock(&server->patients_mutex);
    
    CBPResponse response;
    response.success = 1;
    response.patient_id = 0;
    strcpy(response.message, "Déconnexion réussie");
    
    char response_buffer[CBP_MAX_MESSAGE_SIZE];
    int response_size = serialize_response(&response, response_buffer, sizeof(response_buffer));
    
    CBPMessage response_msg;
    response_msg.command = CBP_LOGOUT;
    response_msg.data_length = response_size;
    memcpy(response_msg.data, response_buffer, response_size);
    
    char final_buffer[CBP_MAX_MESSAGE_SIZE];
    int final_size = serialize_cbp_message(&response_msg, final_buffer, sizeof(final_buffer));
    
    send_data(client_socket, final_buffer, final_size);
}

// Traiter la commande GET_SPECIALTIES
void process_get_specialties_command(ReservationServer* server, Socket* client_socket, const CBPMessage* msg) {
    int count;
    Specialty* specialties = get_specialties(server->db_connection, &count);
    
    if (!specialties) {
        send_error_response(client_socket, "Erreur lors de la récupération des spécialités");
        return;
    }
    
    // Convertir en format CBP
    CBPSpecialty* cbp_specialties = (CBPSpecialty*)malloc(count * sizeof(CBPSpecialty));
    for (int i = 0; i < count; i++) {
        cbp_specialties[i].id = specialties[i].id;
        strcpy(cbp_specialties[i].name, specialties[i].name);
    }
    
    // Sérialiser et envoyer
    char response_buffer[CBP_MAX_MESSAGE_SIZE];
    int response_size = serialize_specialties(cbp_specialties, count, response_buffer, sizeof(response_buffer));
    
    CBPMessage response_msg;
    response_msg.command = CBP_GET_SPECIALTIES;
    response_msg.data_length = response_size;
    memcpy(response_msg.data, response_buffer, response_size);
    
    char final_buffer[CBP_MAX_MESSAGE_SIZE];
    int final_size = serialize_cbp_message(&response_msg, final_buffer, sizeof(final_buffer));
    
    send_data(client_socket, final_buffer, final_size);
    
    // Nettoyer
    free_specialties(specialties);
    free(cbp_specialties);
}

// Traiter la commande GET_DOCTORS
void process_get_doctors_command(ReservationServer* server, Socket* client_socket, const CBPMessage* msg) {
    int count;
    Doctor* doctors = get_doctors(server->db_connection, &count);
    
    if (!doctors) {
        send_error_response(client_socket, "Erreur lors de la récupération des médecins");
        return;
    }
    
    // Convertir en format CBP
    CBPDoctor* cbp_doctors = (CBPDoctor*)malloc(count * sizeof(CBPDoctor));
    for (int i = 0; i < count; i++) {
        cbp_doctors[i].id = doctors[i].id;
        strcpy(cbp_doctors[i].last_name, doctors[i].last_name);
        strcpy(cbp_doctors[i].first_name, doctors[i].first_name);
    }
    
    // Sérialiser et envoyer
    char response_buffer[CBP_MAX_MESSAGE_SIZE];
    int response_size = serialize_doctors(cbp_doctors, count, response_buffer, sizeof(response_buffer));
    
    CBPMessage response_msg;
    response_msg.command = CBP_GET_DOCTORS;
    response_msg.data_length = response_size;
    memcpy(response_msg.data, response_buffer, response_size);
    
    char final_buffer[CBP_MAX_MESSAGE_SIZE];
    int final_size = serialize_cbp_message(&response_msg, final_buffer, sizeof(final_buffer));
    
    send_data(client_socket, final_buffer, final_size);
    
    // Nettoyer
    free_doctors(doctors);
    free(cbp_doctors);
}

// Traiter la commande SEARCH_CONSULTATIONS
void process_search_consultations_command(ReservationServer* server, Socket* client_socket, const CBPMessage* msg) {
    CBPSearchData search_data;
    
    if (deserialize_search_data(msg->data, msg->data_length, &search_data) <= 0) {
        send_error_response(client_socket, "Données de recherche invalides");
        return;
    }
    
    printf("Recherche de consultations: spécialité=%d, médecin=%d, dates=%s à %s\n",
           search_data.specialty_id, search_data.doctor_id, 
           search_data.start_date, search_data.end_date);
    
    int count;
    ConsultationDetails* consultations = search_consultations(server->db_connection,
                                                            search_data.specialty_id,
                                                            search_data.doctor_id,
                                                            search_data.start_date,
                                                            search_data.end_date,
                                                            &count);
    
    if (!consultations) {
        send_error_response(client_socket, "Erreur lors de la recherche des consultations");
        return;
    }
    
    // Convertir en format CBP
    CBPConsultation* cbp_consultations = (CBPConsultation*)malloc(count * sizeof(CBPConsultation));
    for (int i = 0; i < count; i++) {
        cbp_consultations[i].id = consultations[i].id;
        strcpy(cbp_consultations[i].specialty, consultations[i].specialty);
        strcpy(cbp_consultations[i].doctor, consultations[i].doctor);
        strcpy(cbp_consultations[i].date, consultations[i].date);
        strcpy(cbp_consultations[i].hour, consultations[i].hour);
    }
    
    // Sérialiser et envoyer
    char response_buffer[CBP_MAX_MESSAGE_SIZE];
    int response_size = serialize_consultations(cbp_consultations, count, response_buffer, sizeof(response_buffer));
    
    CBPMessage response_msg;
    response_msg.command = CBP_SEARCH_CONSULTATIONS;
    response_msg.data_length = response_size;
    memcpy(response_msg.data, response_buffer, response_size);
    
    char final_buffer[CBP_MAX_MESSAGE_SIZE];
    int final_size = serialize_cbp_message(&response_msg, final_buffer, sizeof(final_buffer));
    
    send_data(client_socket, final_buffer, final_size);
    
    // Nettoyer
    free_consultations(consultations);
    free(cbp_consultations);
}

// Traiter la commande BOOK_CONSULTATION
void process_book_consultation_command(ReservationServer* server, Socket* client_socket, const CBPMessage* msg) {
    CBPBookData book_data;
    CBPResponse response;
    
    if (deserialize_book_data(msg->data, msg->data_length, &book_data) <= 0) {
        send_error_response(client_socket, "Données de réservation invalides");
        return;
    }
    
    printf("Tentative de réservation: consultation=%d, raison=%s\n",
           book_data.consultation_id, book_data.reason);
    
    // Trouver le patient connecté
    int patient_id = 0;
    pthread_mutex_lock(&server->patients_mutex);
    for (int i = 0; i < server->max_patients; i++) {
        if (server->connected_patients[i].socket == client_socket) {
            patient_id = server->connected_patients[i].patient_id;
            break;
        }
    }
    pthread_mutex_unlock(&server->patients_mutex);
    
    if (patient_id == 0) {
        response.success = 0;
        response.patient_id = 0;
        strcpy(response.message, "Patient non connecté");
    } else {
        // Effectuer la réservation
        if (book_consultation(server->db_connection, book_data.consultation_id, patient_id, book_data.reason)) {
            response.success = 1;
            response.patient_id = patient_id;
            strcpy(response.message, "Réservation effectuée avec succès");
        } else {
            response.success = 0;
            response.patient_id = patient_id;
            strcpy(response.message, "Erreur lors de la réservation (consultation peut-être déjà réservée)");
        }
    }
    
    // Envoyer la réponse
    char response_buffer[CBP_MAX_MESSAGE_SIZE];
    int response_size = serialize_response(&response, response_buffer, sizeof(response_buffer));
    
    CBPMessage response_msg;
    response_msg.command = CBP_BOOK_CONSULTATION;
    response_msg.data_length = response_size;
    memcpy(response_msg.data, response_buffer, response_size);
    
    char final_buffer[CBP_MAX_MESSAGE_SIZE];
    int final_size = serialize_cbp_message(&response_msg, final_buffer, sizeof(final_buffer));
    
    send_data(client_socket, final_buffer, final_size);
}

// Ajouter un patient connecté
int add_connected_patient(ReservationServer* server, int patient_id, const char* last_name, 
                         const char* first_name, const char* ip_address, Socket* socket) {
    pthread_mutex_lock(&server->patients_mutex);
    
    // Chercher un emplacement libre
    for (int i = 0; i < server->max_patients; i++) {
        if (server->connected_patients[i].patient_id == 0) {
            server->connected_patients[i].patient_id = patient_id;
            strcpy(server->connected_patients[i].last_name, last_name);
            strcpy(server->connected_patients[i].first_name, first_name);
            strcpy(server->connected_patients[i].ip_address, ip_address);
            server->connected_patients[i].connection_time = time(NULL);
            server->connected_patients[i].socket = socket;
            server->current_patients++;
            
            printf("Patient %d (%s %s) connecté depuis %s\n", 
                   patient_id, last_name, first_name, ip_address);
            
            pthread_mutex_unlock(&server->patients_mutex);
            return 1;
        }
    }
    
    pthread_mutex_unlock(&server->patients_mutex);
    printf("Erreur: Impossible d'ajouter le patient, limite atteinte\n");
    return 0;
}

// Supprimer un patient connecté
void remove_connected_patient(ReservationServer* server, int patient_id) {
    pthread_mutex_lock(&server->patients_mutex);
    
    for (int i = 0; i < server->max_patients; i++) {
        if (server->connected_patients[i].patient_id == patient_id) {
            server->connected_patients[i].patient_id = 0;
            server->connected_patients[i].socket = NULL;
            server->current_patients--;
            break;
        }
    }
    
    pthread_mutex_unlock(&server->patients_mutex);
}

// Trouver un patient connecté
ConnectedPatient* find_connected_patient(ReservationServer* server, int patient_id) {
    pthread_mutex_lock(&server->patients_mutex);
    
    for (int i = 0; i < server->max_patients; i++) {
        if (server->connected_patients[i].patient_id == patient_id) {
            pthread_mutex_unlock(&server->patients_mutex);
            return &server->connected_patients[i];
        }
    }
    
    pthread_mutex_unlock(&server->patients_mutex);
    return NULL;
}

// Afficher les patients connectés
void print_connected_patients(ReservationServer* server) {
    pthread_mutex_lock(&server->patients_mutex);
    
    printf("=== Patients connectés (%d/%d) ===\n", server->current_patients, server->max_patients);
    for (int i = 0; i < server->max_patients; i++) {
        if (server->connected_patients[i].patient_id != 0) {
            printf("ID: %d, Nom: %s %s, IP: %s, Connecté: %s\n",
                   server->connected_patients[i].patient_id,
                   server->connected_patients[i].last_name,
                   server->connected_patients[i].first_name,
                   server->connected_patients[i].ip_address,
                   ctime(&server->connected_patients[i].connection_time));
        }
    }
    printf("================================\n");
    
    pthread_mutex_unlock(&server->patients_mutex);
}

// Envoyer une réponse d'erreur
void send_error_response(Socket* client_socket, const char* error_message) {
    CBPResponse response;
    response.success = 0;
    response.patient_id = 0;
    strncpy(response.message, error_message, sizeof(response.message) - 1);
    response.message[sizeof(response.message) - 1] = '\0';
    
    char response_buffer[CBP_MAX_MESSAGE_SIZE];
    int response_size = serialize_response(&response, response_buffer, sizeof(response_buffer));
    
    CBPMessage response_msg;
    response_msg.command = CBP_ERROR;
    response_msg.data_length = response_size;
    memcpy(response_msg.data, response_buffer, response_size);
    
    char final_buffer[CBP_MAX_MESSAGE_SIZE];
    int final_size = serialize_cbp_message(&response_msg, final_buffer, sizeof(final_buffer));
    
    send_data(client_socket, final_buffer, final_size);
}

// Envoyer une réponse de succès
void send_success_response(Socket* client_socket, int patient_id, const char* message) {
    CBPResponse response;
    response.success = 1;
    response.patient_id = patient_id;
    strncpy(response.message, message, sizeof(response.message) - 1);
    response.message[sizeof(response.message) - 1] = '\0';
    
    char response_buffer[CBP_MAX_MESSAGE_SIZE];
    int response_size = serialize_response(&response, response_buffer, sizeof(response_buffer));
    
    CBPMessage response_msg;
    response_msg.command = CBP_LOGIN;
    response_msg.data_length = response_size;
    memcpy(response_msg.data, response_buffer, response_size);
    
    char final_buffer[CBP_MAX_MESSAGE_SIZE];
    int final_size = serialize_cbp_message(&response_msg, final_buffer, sizeof(final_buffer));
    
    send_data(client_socket, final_buffer, final_size);
}

