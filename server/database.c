#include "database.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Fonction pour gérer les erreurs MySQL
void finish_with_error(MYSQL *con) {
    fprintf(stderr, "Erreur MySQL: %s\n", mysql_error(con));
    mysql_close(con);
}

// Connexion à la base de données
MYSQL* connect_database(const char* host, const char* user, const char* password, const char* database) {
    MYSQL* connection = mysql_init(NULL);
    if (!connection) {
        printf("Erreur: Impossible d'initialiser MySQL\n");
        return NULL;
    }
    
    if (!mysql_real_connect(connection, host, user, password, database, 0, NULL, 0)) {
        printf("Erreur de connexion MySQL: %s\n", mysql_error(connection));
        mysql_close(connection);
        return NULL;
    }
    
    printf("Connexion à la base de données réussie\n");
    return connection;
}

// Déconnexion de la base de données
void disconnect_database(MYSQL* connection) {
    if (connection) {
        mysql_close(connection);
    }
}

// Authentification d'un patient
int authenticate_patient(MYSQL* connection, const char* last_name, const char* first_name, int patient_id) {
    char query[512];
    sprintf(query, "SELECT id FROM patients WHERE last_name='%s' AND first_name='%s' AND id=%d",
            last_name, first_name, patient_id);
    
    if (mysql_query(connection, query)) {
        finish_with_error(connection);
        return 0;
    }
    
    MYSQL_RES* result = mysql_store_result(connection);
    if (!result) {
        finish_with_error(connection);
        return 0;
    }
    
    int exists = (mysql_num_rows(result) > 0);
    mysql_free_result(result);
    
    return exists;
}

// Création d'un nouveau patient
int create_patient(MYSQL* connection, const char* last_name, const char* first_name, const char* birth_date) {
    char query[512];
    sprintf(query, "INSERT INTO patients (last_name, first_name, birth_date) VALUES ('%s', '%s', '%s')",
            last_name, first_name, birth_date);
    
    if (mysql_query(connection, query)) {
        finish_with_error(connection);
        return -1;
    }
    
    return (int)mysql_insert_id(connection);
}

// Obtenir l'ID d'un patient par nom et prénom
int get_patient_id(MYSQL* connection, const char* last_name, const char* first_name) {
    char query[512];
    sprintf(query, "SELECT id FROM patients WHERE last_name='%s' AND first_name='%s'",
            last_name, first_name);
    
    if (mysql_query(connection, query)) {
        finish_with_error(connection);
        return -1;
    }
    
    MYSQL_RES* result = mysql_store_result(connection);
    if (!result) {
        finish_with_error(connection);
        return -1;
    }
    
    MYSQL_ROW row = mysql_fetch_row(result);
    int patient_id = -1;
    if (row) {
        patient_id = atoi(row[0]);
    }
    
    mysql_free_result(result);
    return patient_id;
}

// Récupérer toutes les spécialités
Specialty* get_specialties(MYSQL* connection, int* count) {
    if (mysql_query(connection, "SELECT id, name FROM specialties ORDER BY name")) {
        finish_with_error(connection);
        return NULL;
    }
    
    MYSQL_RES* result = mysql_store_result(connection);
    if (!result) {
        finish_with_error(connection);
        return NULL;
    }
    
    int num_rows = mysql_num_rows(result);
    Specialty* specialties = (Specialty*)malloc(num_rows * sizeof(Specialty));
    if (!specialties) {
        mysql_free_result(result);
        return NULL;
    }
    
    MYSQL_ROW row;
    int i = 0;
    while ((row = mysql_fetch_row(result)) && i < num_rows) {
        specialties[i].id = atoi(row[0]);
        strncpy(specialties[i].name, row[1], sizeof(specialties[i].name) - 1);
        specialties[i].name[sizeof(specialties[i].name) - 1] = '\0';
        i++;
    }
    
    *count = num_rows;
    mysql_free_result(result);
    return specialties;
}

// Récupérer tous les médecins
Doctor* get_doctors(MYSQL* connection, int* count) {
    if (mysql_query(connection, "SELECT id, specialty_id, last_name, first_name FROM doctors ORDER BY last_name, first_name")) {
        finish_with_error(connection);
        return NULL;
    }
    
    MYSQL_RES* result = mysql_store_result(connection);
    if (!result) {
        finish_with_error(connection);
        return NULL;
    }
    
    int num_rows = mysql_num_rows(result);
    Doctor* doctors = (Doctor*)malloc(num_rows * sizeof(Doctor));
    if (!doctors) {
        mysql_free_result(result);
        return NULL;
    }
    
    MYSQL_ROW row;
    int i = 0;
    while ((row = mysql_fetch_row(result)) && i < num_rows) {
        doctors[i].id = atoi(row[0]);
        doctors[i].specialty_id = atoi(row[1]);
        strncpy(doctors[i].last_name, row[2], sizeof(doctors[i].last_name) - 1);
        doctors[i].last_name[sizeof(doctors[i].last_name) - 1] = '\0';
        strncpy(doctors[i].first_name, row[3], sizeof(doctors[i].first_name) - 1);
        doctors[i].first_name[sizeof(doctors[i].first_name) - 1] = '\0';
        i++;
    }
    
    *count = num_rows;
    mysql_free_result(result);
    return doctors;
}

// Récupérer les médecins par spécialité
Doctor* get_doctors_by_specialty(MYSQL* connection, int specialty_id, int* count) {
    char query[512];
    sprintf(query, "SELECT id, specialty_id, last_name, first_name FROM doctors WHERE specialty_id=%d ORDER BY last_name, first_name",
            specialty_id);
    
    if (mysql_query(connection, query)) {
        finish_with_error(connection);
        return NULL;
    }
    
    MYSQL_RES* result = mysql_store_result(connection);
    if (!result) {
        finish_with_error(connection);
        return NULL;
    }
    
    int num_rows = mysql_num_rows(result);
    Doctor* doctors = (Doctor*)malloc(num_rows * sizeof(Doctor));
    if (!doctors) {
        mysql_free_result(result);
        return NULL;
    }
    
    MYSQL_ROW row;
    int i = 0;
    while ((row = mysql_fetch_row(result)) && i < num_rows) {
        doctors[i].id = atoi(row[0]);
        doctors[i].specialty_id = atoi(row[1]);
        strncpy(doctors[i].last_name, row[2], sizeof(doctors[i].last_name) - 1);
        doctors[i].last_name[sizeof(doctors[i].last_name) - 1] = '\0';
        strncpy(doctors[i].first_name, row[3], sizeof(doctors[i].first_name) - 1);
        doctors[i].first_name[sizeof(doctors[i].first_name) - 1] = '\0';
        i++;
    }
    
    *count = num_rows;
    mysql_free_result(result);
    return doctors;
}

// Rechercher les consultations disponibles
ConsultationDetails* search_consultations(MYSQL* connection, int specialty_id, int doctor_id, 
                                        const char* start_date, const char* end_date, int* count) {
    char query[1024];
    char where_clause[512] = "";
    
    // Construire la clause WHERE
    strcat(where_clause, "WHERE c.patient_id IS NULL");
    
    if (specialty_id > 0) {
        char temp[64];
        sprintf(temp, " AND d.specialty_id=%d", specialty_id);
        strcat(where_clause, temp);
    }
    
    if (doctor_id > 0) {
        char temp[64];
        sprintf(temp, " AND c.doctor_id=%d", doctor_id);
        strcat(where_clause, temp);
    }
    
    if (start_date && strlen(start_date) > 0) {
        char temp[128];
        sprintf(temp, " AND c.date >= '%s'", start_date);
        strcat(where_clause, temp);
    }
    
    if (end_date && strlen(end_date) > 0) {
        char temp[128];
        sprintf(temp, " AND c.date <= '%s'", end_date);
        strcat(where_clause, temp);
    }
    
    sprintf(query, "SELECT c.id, s.name, CONCAT(d.last_name, ' ', d.first_name), c.date, c.hour "
                   "FROM consultations c "
                   "JOIN doctors d ON c.doctor_id = d.id "
                   "JOIN specialties s ON d.specialty_id = s.id "
                   "%s "
                   "ORDER BY c.date, c.hour", where_clause);
    
    if (mysql_query(connection, query)) {
        finish_with_error(connection);
        return NULL;
    }
    
    MYSQL_RES* result = mysql_store_result(connection);
    if (!result) {
        finish_with_error(connection);
        return NULL;
    }
    
    int num_rows = mysql_num_rows(result);
    ConsultationDetails* consultations = (ConsultationDetails*)malloc(num_rows * sizeof(ConsultationDetails));
    if (!consultations) {
        mysql_free_result(result);
        return NULL;
    }
    
    MYSQL_ROW row;
    int i = 0;
    while ((row = mysql_fetch_row(result)) && i < num_rows) {
        consultations[i].id = atoi(row[0]);
        strncpy(consultations[i].specialty, row[1], sizeof(consultations[i].specialty) - 1);
        consultations[i].specialty[sizeof(consultations[i].specialty) - 1] = '\0';
        strncpy(consultations[i].doctor, row[2], sizeof(consultations[i].doctor) - 1);
        consultations[i].doctor[sizeof(consultations[i].doctor) - 1] = '\0';
        strncpy(consultations[i].date, row[3], sizeof(consultations[i].date) - 1);
        consultations[i].date[sizeof(consultations[i].date) - 1] = '\0';
        strncpy(consultations[i].hour, row[4], sizeof(consultations[i].hour) - 1);
        consultations[i].hour[sizeof(consultations[i].hour) - 1] = '\0';
        i++;
    }
    
    *count = num_rows;
    mysql_free_result(result);
    return consultations;
}

// Réserver une consultation
int book_consultation(MYSQL* connection, int consultation_id, int patient_id, const char* reason) {
    char query[512];
    sprintf(query, "UPDATE consultations SET patient_id=%d, reason='%s' WHERE id=%d AND patient_id IS NULL",
            patient_id, reason, consultation_id);
    
    if (mysql_query(connection, query)) {
        finish_with_error(connection);
        return 0;
    }
    
    return (mysql_affected_rows(connection) > 0);
}

// Libérer la mémoire des spécialités
void free_specialties(Specialty* specialties) {
    if (specialties) {
        free(specialties);
    }
}

// Libérer la mémoire des médecins
void free_doctors(Doctor* doctors) {
    if (doctors) {
        free(doctors);
    }
}

// Libérer la mémoire des consultations
void free_consultations(ConsultationDetails* consultations) {
    if (consultations) {
        free(consultations);
    }
}
