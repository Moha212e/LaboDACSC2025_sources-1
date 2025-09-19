#ifndef CBP_PROTOCOL_H
#define CBP_PROTOCOL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Constantes du protocole CBP
#define CBP_MAX_MESSAGE_SIZE 1024
#define CBP_MAX_FIELD_SIZE 256

// Types de commandes CBP
typedef enum {
    CBP_LOGIN = 1,
    CBP_LOGOUT = 2,
    CBP_GET_SPECIALTIES = 3,
    CBP_GET_DOCTORS = 4,
    CBP_SEARCH_CONSULTATIONS = 5,
    CBP_BOOK_CONSULTATION = 6,
    CBP_ERROR = 99
} CBPCommand;

// Structure pour un message CBP
typedef struct {
    CBPCommand command;
    char data[CBP_MAX_MESSAGE_SIZE];
    int data_length;
} CBPMessage;

// Structures pour les données des commandes
typedef struct {
    char last_name[30];
    char first_name[30];
    int patient_id;
    int is_new_patient;
} CBPLoginData;

typedef struct {
    int specialty_id;
    int doctor_id;
    char start_date[20];
    char end_date[20];
} CBPSearchData;

typedef struct {
    int consultation_id;
    char reason[100];
} CBPBookData;

// Structures pour les réponses
typedef struct {
    int success;
    int patient_id;
    char message[256];
} CBPResponse;

typedef struct {
    int id;
    char name[30];
} CBPSpecialty;

typedef struct {
    int id;
    char last_name[30];
    char first_name[30];
} CBPDoctor;

typedef struct {
    int id;
    char specialty[30];
    char doctor[60];
    char date[20];
    char hour[10];
} CBPConsultation;

// Fonctions de sérialisation/désérialisation
int serialize_cbp_message(const CBPMessage* msg, char* buffer, int buffer_size);
int deserialize_cbp_message(const char* buffer, int buffer_size, CBPMessage* msg);

// Fonctions de sérialisation des données spécifiques
int serialize_login_data(const CBPLoginData* data, char* buffer, int buffer_size);
int deserialize_login_data(const char* buffer, int buffer_size, CBPLoginData* data);

int serialize_search_data(const CBPSearchData* data, char* buffer, int buffer_size);
int deserialize_search_data(const char* buffer, int buffer_size, CBPSearchData* data);

int serialize_book_data(const CBPBookData* data, char* buffer, int buffer_size);
int deserialize_book_data(const char* buffer, int buffer_size, CBPBookData* data);

// Fonctions de sérialisation des réponses
int serialize_response(const CBPResponse* response, char* buffer, int buffer_size);
int deserialize_response(const char* buffer, int buffer_size, CBPResponse* response);

int serialize_specialties(const CBPSpecialty* specialties, int count, char* buffer, int buffer_size);
int serialize_doctors(const CBPDoctor* doctors, int count, char* buffer, int buffer_size);
int serialize_consultations(const CBPConsultation* consultations, int count, char* buffer, int buffer_size);

// Fonctions utilitaires
void print_cbp_message(const CBPMessage* msg);
const char* cbp_command_to_string(CBPCommand command);

#endif // CBP_PROTOCOL_H
