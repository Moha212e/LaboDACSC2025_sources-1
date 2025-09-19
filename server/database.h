#ifndef DATABASE_H
#define DATABASE_H

#include <mysql.h>

// Structure pour représenter une spécialité
typedef struct {
    int id;
    char name[30];
} Specialty;

// Structure pour représenter un médecin
typedef struct {
    int id;
    int specialty_id;
    char last_name[30];
    char first_name[30];
} Doctor;

// Structure pour représenter un patient
typedef struct {
    int id;
    char last_name[30];
    char first_name[30];
    char birth_date[20];
} Patient;

// Structure pour représenter une consultation
typedef struct {
    int id;
    int doctor_id;
    int patient_id;
    char date[20];
    char hour[10];
    char reason[100];
} Consultation;

// Structure pour une consultation avec détails complets
typedef struct {
    int id;
    char specialty[30];
    char doctor[60];
    char date[20];
    char hour[10];
} ConsultationDetails;

// Fonctions de connexion à la base de données
MYSQL* connect_database(const char* host, const char* user, const char* password, const char* database);
void disconnect_database(MYSQL* connection);

// Fonctions de gestion des patients
int authenticate_patient(MYSQL* connection, const char* last_name, const char* first_name, int patient_id);
int create_patient(MYSQL* connection, const char* last_name, const char* first_name, const char* birth_date);
int get_patient_id(MYSQL* connection, const char* last_name, const char* first_name);

// Fonctions de récupération des données
Specialty* get_specialties(MYSQL* connection, int* count);
Doctor* get_doctors(MYSQL* connection, int* count);
Doctor* get_doctors_by_specialty(MYSQL* connection, int specialty_id, int* count);
ConsultationDetails* search_consultations(MYSQL* connection, int specialty_id, int doctor_id, 
                                        const char* start_date, const char* end_date, int* count);

// Fonctions de réservation
int book_consultation(MYSQL* connection, int consultation_id, int patient_id, const char* reason);

// Fonctions utilitaires
void free_specialties(Specialty* specialties);
void free_doctors(Doctor* doctors);
void free_consultations(ConsultationDetails* consultations);

#endif // DATABASE_H
