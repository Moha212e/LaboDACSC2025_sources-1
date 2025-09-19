#include "reservation_server.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[]) {
    const char* config_file = "server_config.txt";
    
    // Vérifier les arguments
    if (argc > 1) {
        config_file = argv[1];
    }
    
    printf("=== Serveur de Réservation Médicale ===\n");
    printf("Fichier de configuration: %s\n", config_file);
    
    // Créer le serveur
    ReservationServer* server = create_reservation_server(config_file);
    if (!server) {
        printf("Erreur: Impossible de créer le serveur\n");
        return 1;
    }
    
    // Démarrer le serveur
    printf("Démarrage du serveur...\n");
    if (start_reservation_server(server) != 0) {
        printf("Erreur: Impossible de démarrer le serveur\n");
        destroy_reservation_server(server);
        return 1;
    }
    
    // Le serveur s'arrête ici (dans la boucle principale)
    // Nettoyer les ressources
    destroy_reservation_server(server);
    
    printf("Serveur arrêté proprement\n");
    return 0;
}

