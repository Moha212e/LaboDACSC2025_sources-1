#include "cbp_protocol.h"
#include <arpa/inet.h>
#include <stdint.h>

// Sérialiser un message CBP
int serialize_cbp_message(const CBPMessage* msg, char* buffer, int buffer_size) {
    if (!msg || !buffer || buffer_size < 4) {
        return -1;
    }
    
    int pos = 0;
    
    // Commande (4 bytes)
    *((int*)(buffer + pos)) = htonl((uint32_t)msg->command);
    pos += 4;
    
    // Longueur des données (4 bytes)
    *((int*)(buffer + pos)) = htonl((uint32_t)msg->data_length);
    pos += 4;
    
    // Données
    if (msg->data_length > 0 && pos + msg->data_length <= buffer_size) {
        memcpy(buffer + pos, msg->data, msg->data_length);
        pos += msg->data_length;
    }
    
    return pos;
}

// Désérialiser un message CBP
int deserialize_cbp_message(const char* buffer, int buffer_size, CBPMessage* msg) {
    if (!buffer || !msg || buffer_size < 8) {
        return -1;
    }
    
    int pos = 0;
    
    // Commande
    msg->command = (CBPCommand)ntohl(*((uint32_t*)(buffer + pos)));
    pos += 4;
    
    // Longueur des données
    msg->data_length = ntohl(*((uint32_t*)(buffer + pos)));
    pos += 4;
    
    // Vérifier la taille
    if (pos + msg->data_length > buffer_size || msg->data_length > CBP_MAX_MESSAGE_SIZE) {
        return -1;
    }
    
    // Données
    if (msg->data_length > 0) {
        memcpy(msg->data, buffer + pos, msg->data_length);
        msg->data[msg->data_length] = '\0';
    }
    
    return pos + msg->data_length;
}

// Sérialiser les données de login
int serialize_login_data(const CBPLoginData* data, char* buffer, int buffer_size) {
    if (!data || !buffer) return -1;
    
    int pos = 0;
    
    // Nom
    int len = strlen(data->last_name);
    if (pos + 4 + len > buffer_size) return -1;
    *((int*)(buffer + pos)) = htonl(len);
    pos += 4;
    memcpy(buffer + pos, data->last_name, len);
    pos += len;
    
    // Prénom
    len = strlen(data->first_name);
    if (pos + 4 + len > buffer_size) return -1;
    *((int*)(buffer + pos)) = htonl(len);
    pos += 4;
    memcpy(buffer + pos, data->first_name, len);
    pos += len;
    
    // ID patient
    if (pos + 4 > buffer_size) return -1;
    *((int*)(buffer + pos)) = htonl(data->patient_id);
    pos += 4;
    
    // Nouveau patient
    if (pos + 4 > buffer_size) return -1;
    *((int*)(buffer + pos)) = htonl(data->is_new_patient);
    pos += 4;
    
    return pos;
}

// Désérialiser les données de login
int deserialize_login_data(const char* buffer, int buffer_size, CBPLoginData* data) {
    if (!buffer || !data) return -1;
    
    int pos = 0;
    
    // Nom
    if (pos + 4 > buffer_size) return -1;
    int len = ntohl(*((int*)(buffer + pos)));
    pos += 4;
    if (pos + len > buffer_size || len >= sizeof(data->last_name)) return -1;
    memcpy(data->last_name, buffer + pos, len);
    data->last_name[len] = '\0';
    pos += len;
    
    // Prénom
    if (pos + 4 > buffer_size) return -1;
    len = ntohl(*((int*)(buffer + pos)));
    pos += 4;
    if (pos + len > buffer_size || len >= sizeof(data->first_name)) return -1;
    memcpy(data->first_name, buffer + pos, len);
    data->first_name[len] = '\0';
    pos += len;
    
    // ID patient
    if (pos + 4 > buffer_size) return -1;
    data->patient_id = ntohl(*((int*)(buffer + pos)));
    pos += 4;
    
    // Nouveau patient
    if (pos + 4 > buffer_size) return -1;
    data->is_new_patient = ntohl(*((int*)(buffer + pos)));
    pos += 4;
    
    return pos;
}

// Sérialiser les données de recherche
int serialize_search_data(const CBPSearchData* data, char* buffer, int buffer_size) {
    if (!data || !buffer) return -1;
    
    int pos = 0;
    
    // Spécialité ID
    if (pos + 4 > buffer_size) return -1;
    *((int*)(buffer + pos)) = htonl(data->specialty_id);
    pos += 4;
    
    // Médecin ID
    if (pos + 4 > buffer_size) return -1;
    *((int*)(buffer + pos)) = htonl(data->doctor_id);
    pos += 4;
    
    // Date début
    int len = strlen(data->start_date);
    if (pos + 4 + len > buffer_size) return -1;
    *((int*)(buffer + pos)) = htonl(len);
    pos += 4;
    memcpy(buffer + pos, data->start_date, len);
    pos += len;
    
    // Date fin
    len = strlen(data->end_date);
    if (pos + 4 + len > buffer_size) return -1;
    *((int*)(buffer + pos)) = htonl(len);
    pos += 4;
    memcpy(buffer + pos, data->end_date, len);
    pos += len;
    
    return pos;
}

// Désérialiser les données de recherche
int deserialize_search_data(const char* buffer, int buffer_size, CBPSearchData* data) {
    if (!buffer || !data) return -1;
    
    int pos = 0;
    
    // Spécialité ID
    if (pos + 4 > buffer_size) return -1;
    data->specialty_id = ntohl(*((int*)(buffer + pos)));
    pos += 4;
    
    // Médecin ID
    if (pos + 4 > buffer_size) return -1;
    data->doctor_id = ntohl(*((int*)(buffer + pos)));
    pos += 4;
    
    // Date début
    if (pos + 4 > buffer_size) return -1;
    int len = ntohl(*((int*)(buffer + pos)));
    pos += 4;
    if (pos + len > buffer_size || len >= sizeof(data->start_date)) return -1;
    memcpy(data->start_date, buffer + pos, len);
    data->start_date[len] = '\0';
    pos += len;
    
    // Date fin
    if (pos + 4 > buffer_size) return -1;
    len = ntohl(*((int*)(buffer + pos)));
    pos += 4;
    if (pos + len > buffer_size || len >= sizeof(data->end_date)) return -1;
    memcpy(data->end_date, buffer + pos, len);
    data->end_date[len] = '\0';
    pos += len;
    
    return pos;
}

// Sérialiser les données de réservation
int serialize_book_data(const CBPBookData* data, char* buffer, int buffer_size) {
    if (!data || !buffer) return -1;
    
    int pos = 0;
    
    // ID consultation
    if (pos + 4 > buffer_size) return -1;
    *((int*)(buffer + pos)) = htonl(data->consultation_id);
    pos += 4;
    
    // Raison
    int len = strlen(data->reason);
    if (pos + 4 + len > buffer_size) return -1;
    *((int*)(buffer + pos)) = htonl(len);
    pos += 4;
    memcpy(buffer + pos, data->reason, len);
    pos += len;
    
    return pos;
}

// Désérialiser les données de réservation
int deserialize_book_data(const char* buffer, int buffer_size, CBPBookData* data) {
    if (!buffer || !data) return -1;
    
    int pos = 0;
    
    // ID consultation
    if (pos + 4 > buffer_size) return -1;
    data->consultation_id = ntohl(*((int*)(buffer + pos)));
    pos += 4;
    
    // Raison
    if (pos + 4 > buffer_size) return -1;
    int len = ntohl(*((int*)(buffer + pos)));
    pos += 4;
    if (pos + len > buffer_size || len >= sizeof(data->reason)) return -1;
    memcpy(data->reason, buffer + pos, len);
    data->reason[len] = '\0';
    pos += len;
    
    return pos;
}

// Sérialiser une réponse
int serialize_response(const CBPResponse* response, char* buffer, int buffer_size) {
    if (!response || !buffer) return -1;
    
    int pos = 0;
    
    // Succès
    if (pos + 4 > buffer_size) return -1;
    *((int*)(buffer + pos)) = htonl(response->success);
    pos += 4;
    
    // ID patient
    if (pos + 4 > buffer_size) return -1;
    *((int*)(buffer + pos)) = htonl(response->patient_id);
    pos += 4;
    
    // Message
    int len = strlen(response->message);
    if (pos + 4 + len > buffer_size) return -1;
    *((int*)(buffer + pos)) = htonl(len);
    pos += 4;
    memcpy(buffer + pos, response->message, len);
    pos += len;
    
    return pos;
}

// Désérialiser une réponse
int deserialize_response(const char* buffer, int buffer_size, CBPResponse* response) {
    if (!buffer || !response) return -1;
    
    int pos = 0;
    
    // Succès
    if (pos + 4 > buffer_size) return -1;
    response->success = ntohl(*((int*)(buffer + pos)));
    pos += 4;
    
    // ID patient
    if (pos + 4 > buffer_size) return -1;
    response->patient_id = ntohl(*((int*)(buffer + pos)));
    pos += 4;
    
    // Message
    if (pos + 4 > buffer_size) return -1;
    int len = ntohl(*((int*)(buffer + pos)));
    pos += 4;
    if (pos + len > buffer_size || len >= sizeof(response->message)) return -1;
    memcpy(response->message, buffer + pos, len);
    response->message[len] = '\0';
    pos += len;
    
    return pos;
}

// Sérialiser les spécialités
int serialize_specialties(const CBPSpecialty* specialties, int count, char* buffer, int buffer_size) {
    if (!specialties || !buffer) return -1;
    
    int pos = 0;
    
    // Nombre de spécialités
    if (pos + 4 > buffer_size) return -1;
    *((int*)(buffer + pos)) = htonl(count);
    pos += 4;
    
    for (int i = 0; i < count; i++) {
        // ID
        if (pos + 4 > buffer_size) return -1;
        *((int*)(buffer + pos)) = htonl(specialties[i].id);
        pos += 4;
        
        // Nom
        int len = strlen(specialties[i].name);
        if (pos + 4 + len > buffer_size) return -1;
        *((int*)(buffer + pos)) = htonl(len);
        pos += 4;
        memcpy(buffer + pos, specialties[i].name, len);
        pos += len;
    }
    
    return pos;
}

// Sérialiser les médecins
int serialize_doctors(const CBPDoctor* doctors, int count, char* buffer, int buffer_size) {
    if (!doctors || !buffer) return -1;
    
    int pos = 0;
    
    // Nombre de médecins
    if (pos + 4 > buffer_size) return -1;
    *((int*)(buffer + pos)) = htonl(count);
    pos += 4;
    
    for (int i = 0; i < count; i++) {
        // ID
        if (pos + 4 > buffer_size) return -1;
        *((int*)(buffer + pos)) = htonl(doctors[i].id);
        pos += 4;
        
        // Nom
        int len = strlen(doctors[i].last_name);
        if (pos + 4 + len > buffer_size) return -1;
        *((int*)(buffer + pos)) = htonl(len);
        pos += 4;
        memcpy(buffer + pos, doctors[i].last_name, len);
        pos += len;
        
        // Prénom
        len = strlen(doctors[i].first_name);
        if (pos + 4 + len > buffer_size) return -1;
        *((int*)(buffer + pos)) = htonl(len);
        pos += 4;
        memcpy(buffer + pos, doctors[i].first_name, len);
        pos += len;
    }
    
    return pos;
}

// Sérialiser les consultations
int serialize_consultations(const CBPConsultation* consultations, int count, char* buffer, int buffer_size) {
    if (!consultations || !buffer) return -1;
    
    int pos = 0;
    
    // Nombre de consultations
    if (pos + 4 > buffer_size) return -1;
    *((int*)(buffer + pos)) = htonl(count);
    pos += 4;
    
    for (int i = 0; i < count; i++) {
        // ID
        if (pos + 4 > buffer_size) return -1;
        *((int*)(buffer + pos)) = htonl(consultations[i].id);
        pos += 4;
        
        // Spécialité
        int len = strlen(consultations[i].specialty);
        if (pos + 4 + len > buffer_size) return -1;
        *((int*)(buffer + pos)) = htonl(len);
        pos += 4;
        memcpy(buffer + pos, consultations[i].specialty, len);
        pos += len;
        
        // Médecin
        len = strlen(consultations[i].doctor);
        if (pos + 4 + len > buffer_size) return -1;
        *((int*)(buffer + pos)) = htonl(len);
        pos += 4;
        memcpy(buffer + pos, consultations[i].doctor, len);
        pos += len;
        
        // Date
        len = strlen(consultations[i].date);
        if (pos + 4 + len > buffer_size) return -1;
        *((int*)(buffer + pos)) = htonl(len);
        pos += 4;
        memcpy(buffer + pos, consultations[i].date, len);
        pos += len;
        
        // Heure
        len = strlen(consultations[i].hour);
        if (pos + 4 + len > buffer_size) return -1;
        *((int*)(buffer + pos)) = htonl(len);
        pos += 4;
        memcpy(buffer + pos, consultations[i].hour, len);
        pos += len;
    }
    
    return pos;
}

// Afficher un message CBP
void print_cbp_message(const CBPMessage* msg) {
    if (!msg) return;
    
    printf("Message CBP:\n");
    printf("  Commande: %s (%d)\n", cbp_command_to_string(msg->command), msg->command);
    printf("  Longueur données: %d\n", msg->data_length);
    if (msg->data_length > 0) {
        printf("  Données: %.*s\n", msg->data_length, msg->data);
    }
}

// Convertir une commande en string
const char* cbp_command_to_string(CBPCommand command) {
    switch (command) {
        case CBP_LOGIN: return "LOGIN";
        case CBP_LOGOUT: return "LOGOUT";
        case CBP_GET_SPECIALTIES: return "GET_SPECIALTIES";
        case CBP_GET_DOCTORS: return "GET_DOCTORS";
        case CBP_SEARCH_CONSULTATIONS: return "SEARCH_CONSULTATIONS";
        case CBP_BOOK_CONSULTATION: return "BOOK_CONSULTATION";
        case CBP_ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}
