#include "config_parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int load_config(const char* config_file, ServerConfig* config) {
    FILE* file = fopen(config_file, "r");
    if (!file) {
        printf("Erreur: Impossible d'ouvrir le fichier de configuration %s\n", config_file);
        return -1;
    }
    
    // Valeurs par défaut
    config->port_reservation = 8080;
    config->thread_pool_size = 10;
    strcpy(config->db_host, "localhost");
    strcpy(config->db_user, "Student");
    strcpy(config->db_password, "PassStudent1_");
    strcpy(config->db_name, "PourStudent");
    config->port_admin = 8081;
    config->connection_timeout = 300;
    
    char line[512];
    char key[256];
    char value[256];
    
    while (fgets(line, sizeof(line), file)) {
        // Ignorer les commentaires et lignes vides
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r') {
            continue;
        }
        
        // Parser la ligne clé=valeur
        if (sscanf(line, "%255[^=]=%255s", key, value) == 2) {
            // Supprimer les espaces en fin de clé et valeur
            char* end_key = key + strlen(key) - 1;
            while (end_key > key && (*end_key == ' ' || *end_key == '\t')) {
                *end_key = '\0';
                end_key--;
            }
            
            char* end_value = value + strlen(value) - 1;
            while (end_value > value && (*end_value == ' ' || *end_value == '\t' || *end_value == '\r' || *end_value == '\n')) {
                *end_value = '\0';
                end_value--;
            }
            
            // Assigner les valeurs selon la clé
            if (strcmp(key, "PORT_RESERVATION") == 0) {
                config->port_reservation = atoi(value);
            } else if (strcmp(key, "THREAD_POOL_SIZE") == 0) {
                config->thread_pool_size = atoi(value);
            } else if (strcmp(key, "DB_HOST") == 0) {
                strncpy(config->db_host, value, sizeof(config->db_host) - 1);
                config->db_host[sizeof(config->db_host) - 1] = '\0';
            } else if (strcmp(key, "DB_USER") == 0) {
                strncpy(config->db_user, value, sizeof(config->db_user) - 1);
                config->db_user[sizeof(config->db_user) - 1] = '\0';
            } else if (strcmp(key, "DB_PASSWORD") == 0) {
                strncpy(config->db_password, value, sizeof(config->db_password) - 1);
                config->db_password[sizeof(config->db_password) - 1] = '\0';
            } else if (strcmp(key, "DB_NAME") == 0) {
                strncpy(config->db_name, value, sizeof(config->db_name) - 1);
                config->db_name[sizeof(config->db_name) - 1] = '\0';
            } else if (strcmp(key, "PORT_ADMIN") == 0) {
                config->port_admin = atoi(value);
            } else if (strcmp(key, "CONNECTION_TIMEOUT") == 0) {
                config->connection_timeout = atoi(value);
            }
        }
    }
    
    fclose(file);
    return 0;
}

void print_config(const ServerConfig* config) {
    printf("=== Configuration du serveur ===\n");
    printf("Port réservation: %d\n", config->port_reservation);
    printf("Taille du pool de threads: %d\n", config->thread_pool_size);
    printf("Base de données:\n");
    printf("  Host: %s\n", config->db_host);
    printf("  User: %s\n", config->db_user);
    printf("  Database: %s\n", config->db_name);
    printf("Port admin: %d\n", config->port_admin);
    printf("Timeout connexion: %d secondes\n", config->connection_timeout);
    printf("===============================\n");
}
