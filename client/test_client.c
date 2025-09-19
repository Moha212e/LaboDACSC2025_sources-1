#include "../socket_lib/socket_lib.h"
#include "../server/cbp_protocol.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void test_login_new_patient(Socket* client) {
    printf("\n=== Test LOGIN nouveau patient ===\n");
    
    // Créer les données de login
    CBPLoginData login_data;
    strcpy(login_data.last_name, "Test");
    strcpy(login_data.first_name, "Patient");
    login_data.patient_id = 0;
    login_data.is_new_patient = 1;
    
    // Sérialiser les données
    char data_buffer[256];
    int data_size = serialize_login_data(&login_data, data_buffer, sizeof(data_buffer));
    
    // Créer le message CBP
    CBPMessage message;
    message.command = CBP_LOGIN;
    message.data_length = data_size;
    memcpy(message.data, data_buffer, data_size);
    
    // Sérialiser le message
    char message_buffer[512];
    int message_size = serialize_cbp_message(&message, message_buffer, sizeof(message_buffer));
    
    // Envoyer le message
    printf("Envoi de la commande LOGIN...\n");
    int sent = send_data(client, message_buffer, message_size);
    printf("Bytes envoyés: %d\n", sent);
    
    // Recevoir la réponse
    char response_buffer[512];
    int received = receive_data(client, response_buffer, sizeof(response_buffer));
    printf("Bytes reçus: %d\n", received);
    
    if (received > 0) {
        CBPMessage response_msg;
        if (deserialize_cbp_message(response_buffer, received, &response_msg) > 0) {
            printf("Réponse reçue: %s\n", cbp_command_to_string(response_msg.command));
            
            CBPResponse response;
            if (deserialize_response(response_msg.data, response_msg.data_length, &response) > 0) {
                printf("Succès: %d\n", response.success);
                printf("ID Patient: %d\n", response.patient_id);
                printf("Message: %s\n", response.message);
            }
        }
    }
}

void test_get_specialties(Socket* client) {
    printf("\n=== Test GET_SPECIALTIES ===\n");
    
    // Créer le message
    CBPMessage message;
    message.command = CBP_GET_SPECIALTIES;
    message.data_length = 0;
    
    // Sérialiser le message
    char message_buffer[512];
    int message_size = serialize_cbp_message(&message, message_buffer, sizeof(message_buffer));
    
    // Envoyer le message
    printf("Envoi de la commande GET_SPECIALTIES...\n");
    int sent = send_data(client, message_buffer, message_size);
    printf("Bytes envoyés: %d\n", sent);
    
    // Recevoir la réponse
    char response_buffer[1024];
    int received = receive_data(client, response_buffer, sizeof(response_buffer));
    printf("Bytes reçus: %d\n", received);
    
    if (received > 0) {
        CBPMessage response_msg;
        if (deserialize_cbp_message(response_buffer, received, &response_msg) > 0) {
            printf("Réponse reçue: %s\n", cbp_command_to_string(response_msg.command));
            printf("Données reçues (%d bytes)\n", response_msg.data_length);
        }
    }
}

void test_search_consultations(Socket* client) {
    printf("\n=== Test SEARCH_CONSULTATIONS ===\n");
    
    // Créer les données de recherche
    CBPSearchData search_data;
    search_data.specialty_id = 0; // Toutes les spécialités
    search_data.doctor_id = 0;    // Tous les médecins
    strcpy(search_data.start_date, "2025-10-01");
    strcpy(search_data.end_date, "2025-12-31");
    
    // Sérialiser les données
    char data_buffer[256];
    int data_size = serialize_search_data(&search_data, data_buffer, sizeof(data_buffer));
    
    // Créer le message CBP
    CBPMessage message;
    message.command = CBP_SEARCH_CONSULTATIONS;
    message.data_length = data_size;
    memcpy(message.data, data_buffer, data_size);
    
    // Sérialiser le message
    char message_buffer[512];
    int message_size = serialize_cbp_message(&message, message_buffer, sizeof(message_buffer));
    
    // Envoyer le message
    printf("Envoi de la commande SEARCH_CONSULTATIONS...\n");
    int sent = send_data(client, message_buffer, message_size);
    printf("Bytes envoyés: %d\n", sent);
    
    // Recevoir la réponse
    char response_buffer[2048];
    int received = receive_data(client, response_buffer, sizeof(response_buffer));
    printf("Bytes reçus: %d\n", received);
    
    if (received > 0) {
        CBPMessage response_msg;
        if (deserialize_cbp_message(response_buffer, received, &response_msg) > 0) {
            printf("Réponse reçue: %s\n", cbp_command_to_string(response_msg.command));
            printf("Données reçues (%d bytes)\n", response_msg.data_length);
        }
    }
}

int main() {
    printf("=== Client de test pour le serveur de réservation ===\n");
    
    // Créer le socket client
    Socket* client = create_socket();
    if (!client) {
        printf("Erreur: Impossible de créer le socket client\n");
        return 1;
    }
    
    // Se connecter au serveur
    printf("Connexion au serveur sur localhost:8080...\n");
    if (connect_socket(client, "127.0.0.1", 8080) != 0) {
        printf("Erreur: Impossible de se connecter au serveur\n");
        printf("Assurez-vous que le serveur est démarré\n");
        destroy_socket(client);
        return 1;
    }
    
    printf("Connexion réussie!\n");
    
    // Exécuter les tests
    test_login_new_patient(client);
    test_get_specialties(client);
    test_search_consultations(client);
    
    // Fermer la connexion
    printf("\nFermeture de la connexion...\n");
    close_socket(client);
    destroy_socket(client);
    
    printf("Test terminé\n");
    return 0;
}
