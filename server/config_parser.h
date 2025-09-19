#ifndef CONFIG_PARSER_H
#define CONFIG_PARSER_H

typedef struct {
    int port_reservation;
    int thread_pool_size;
    char db_host[256];
    char db_user[256];
    char db_password[256];
    char db_name[256];
    int port_admin;
    int connection_timeout;
} ServerConfig;

// Charger la configuration depuis un fichier
int load_config(const char* config_file, ServerConfig* config);

// Afficher la configuration
void print_config(const ServerConfig* config);

#endif // CONFIG_PARSER_H
