
/**
 * Implémentation de la librairie de sockets générique et abstraite
 * 
 * Ce fichier implémente toutes les fonctions déclarées dans socket.h
 * en masquant les détails d'implémentation des sockets système.
 */

// ============================================================================
// INCLUDES
// ============================================================================
#include "socket.h"
#include <unistd.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
// ============================================================================
// FONCTIONS DE COMMUNICATION
// ============================================================================

/**
 * Envoie des données sur un socket avec délimiteur de fin de message
 * @param sSocket Descripteur de socket
 * @param data Données à envoyer
 * @param taille Taille des données
 * @return Nombre d'octets envoyés ou -1 en cas d'erreur
 */
int Send(int sSocket, const char *data, int taille) {
    // Vérification des paramètres d'entrée
    if (sSocket < 0 || data == NULL || taille <= 0 || taille > TAILLE_MAX) {
        return -1;
    }
    
    // Préparation du message avec délimiteur de fin
    char message[TAILLE_MAX + 1];
    strncpy(message, data, taille);
    message[taille] = '\n'; // Délimiteur de fin de message
    
    // Envoi en plusieurs parties si nécessaire
    int totalEnvoye = 0;
    int tailleComplete = taille + 1; // +1 pour le délimiteur '\n'
    
    while (totalEnvoye < tailleComplete) {
        int octetsEnvoyes = send(sSocket, message + totalEnvoye, 
                                tailleComplete - totalEnvoye, 0);
        if (octetsEnvoyes <= 0) {
            // Erreur de transmission ou connexion fermée
            return -1;
        }
        totalEnvoye += octetsEnvoyes;
    }
    
    return totalEnvoye;
}

/**
 * Reçoit des données depuis un socket avec parsing du délimiteur
 * @param sSocket Descripteur de socket
 * @param data Buffer de réception
 * @return Taille des données reçues ou -1 en cas d'erreur
 */
int Receive(int sSocket, char *data) {
    // Vérification des paramètres d'entrée
    if (sSocket < 0 || data == NULL) {
        return -1;
    }
    
    char buffer[TAILLE_MAX];
    int totalRecu = 0;
    
    // Réception des données jusqu'au délimiteur de fin
    while (totalRecu < TAILLE_MAX) {
        int octetsRecus = recv(sSocket, buffer + totalRecu, 
                              TAILLE_MAX - totalRecu, 0);
        if (octetsRecus <= 0) {
            // Erreur de réception ou connexion fermée
            return -1;
        }
        
        totalRecu += octetsRecus;
        
        // Recherche du délimiteur de fin de message '\n'
        for (int i = 0; i < totalRecu; i++) {
            if (buffer[i] == '\n') {
                // Message complet trouvé
                buffer[i] = '\0'; // Remplacer '\n' par '\0'
                strcpy(data, buffer);
                return i; // Retourner la taille du message (sans le '\n')
            }
        }
    }
    
    // Message trop long pour le buffer
    return -1;
}
// ============================================================================
// FONCTIONS SERVEUR
// ============================================================================

/**
 * Crée et configure un socket serveur en écoute
 * @param port Port d'écoute du serveur
 * @return Descripteur de socket ou -1 en cas d'erreur
 */
int ServerSocket(int port) {
    // Vérification du port
    if (port <= 0 || port > 65535) {
        return -1;
    }
    
    // Création du socket TCP
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        return -1;
    }
    
    // Configuration de l'adresse serveur
    struct sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY; // Écoute sur toutes les interfaces
    serverAddress.sin_port = htons(port);       // Conversion en format réseau
    
    // Association du socket à l'adresse (bind)
    if (bind(serverSocket, (struct sockaddr *)&serverAddress, 
             sizeof(serverAddress)) < 0) {
        closeSocket(serverSocket);
        return -1;
    }
    
    // Mise en écoute du socket (file d'attente de 5 connexions)
    if (listen(serverSocket, 5) < 0) {
        closeSocket(serverSocket);
        return -1;
    }
    
    return serverSocket;
}
/**
 * Accepte une connexion entrante sur le socket serveur
 * @param socketEcoute Socket serveur en écoute
 * @param ipClient Buffer pour stocker l'IP du client (peut être NULL)
 * @return Descripteur de socket client ou -1 en cas d'erreur
 */
int AcceptConnection(int socketEcoute, char* ipClient) {
    // Vérification du socket d'écoute
    if (socketEcoute < 0) {
        return -1;
    }
    
    // Structure pour recevoir les informations du client
    struct sockaddr_in clientAddress;
    socklen_t addressSize = sizeof(clientAddress);
    
    // Attente et acceptation d'une connexion
    int clientSocket = accept(socketEcoute, (struct sockaddr *)&clientAddress, 
                             &addressSize);
    if (clientSocket < 0) {
        return -1;
    }
    
    // Récupération de l'adresse IP du client si demandée
    if (ipClient != NULL) {
        snprintf(ipClient, INET_ADDRSTRLEN, "%s", 
                inet_ntoa(clientAddress.sin_addr));
    }
    
    return clientSocket;
}
// ============================================================================
// FONCTIONS CLIENT
// ============================================================================

/**
 * Établit une connexion vers un serveur
 * @param ipServeur Adresse IP du serveur
 * @param port Port du serveur
 * @return Descripteur de socket ou -1 en cas d'erreur
 */
int ClientSocket(const char* ipServeur, int port) {
    // Vérification des paramètres d'entrée
    if (ipServeur == NULL || port <= 0 || port > 65535) {
        return -1;
    }
    
    // Création du socket TCP
    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket < 0) {
        return -1;
    }
    
    // Configuration de l'adresse du serveur
    struct sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = inet_addr(ipServeur);
    serverAddress.sin_port = htons(port);
    
    // Tentative de connexion au serveur
    if (connect(clientSocket, (struct sockaddr *)&serverAddress, 
                sizeof(serverAddress)) < 0) {
        closeSocket(clientSocket);
        return -1;
    }
    
    return clientSocket;
}
// ============================================================================
// FONCTIONS UTILITAIRES
// ============================================================================

/**
 * Ferme proprement un socket
 * @param sSocket Descripteur de socket à fermer
 * @return 0 en cas de succès, -1 en cas d'erreur
 */
int closeSocket(int sSocket) {
    if (sSocket < 0) {
        return -1;
    }
    
    if (close(sSocket) < 0) {
        return -1;
    }
    
    return 0;
}