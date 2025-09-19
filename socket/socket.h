/**
 * Librairie de Sockets Générique et Abstraite
 * 
 * Cette librairie fournit une interface simplifiée pour la communication
 * réseau en masquant les détails d'implémentation des sockets système.
 * 
 * Caractéristiques :
 * - Générique : Aucune référence métier spécifique
 * - Abstraite : Masque les structures système (sockaddr_in, etc.)
 * - Protocole : Utilise '\n' comme délimiteur de fin de message
 * - Thread-safe : Peut être utilisée dans un environnement multi-threads
 */

#ifndef SOCKET_H
#define SOCKET_H

// ============================================================================
// INCLUDES SYSTÈME
// ============================================================================
#include <sys/socket.h>
#include <netinet/in.h>

// ============================================================================
// CONSTANTES
// ============================================================================
#define TAILLE_MAX 1024        // Taille maximale des messages

// ============================================================================
// FONCTIONS SERVEUR
// ============================================================================

/**
 * Crée et configure un socket serveur en écoute
 * @param port Port d'écoute du serveur
 * @return Descripteur de socket ou -1 en cas d'erreur
 */
int ServerSocket(int port);

/**
 * Accepte une connexion entrante sur le socket serveur
 * @param socketEcoute Socket serveur en écoute
 * @param ipClient Buffer pour stocker l'IP du client (peut être NULL)
 * @return Descripteur de socket client ou -1 en cas d'erreur
 */
int AcceptConnection(int socketEcoute, char* ipClient);

// ============================================================================
// FONCTIONS CLIENT
// ============================================================================

/**
 * Établit une connexion vers un serveur
 * @param ipServeur Adresse IP du serveur
 * @param port Port du serveur
 * @return Descripteur de socket ou -1 en cas d'erreur
 */
int ClientSocket(const char* ipServeur, int port);

// ============================================================================
// FONCTIONS DE COMMUNICATION
// ============================================================================

/**
 * Envoie des données sur un socket
 * @param sSocket Descripteur de socket
 * @param data Données à envoyer
 * @param taille Taille des données
 * @return Nombre d'octets envoyés ou -1 en cas d'erreur
 */
int Send(int sSocket, const char *data, int taille);

/**
 * Reçoit des données depuis un socket
 * @param sSocket Descripteur de socket
 * @param data Buffer de réception
 * @return Taille des données reçues ou -1 en cas d'erreur
 */
int Receive(int sSocket, char *data);

// ============================================================================
// FONCTIONS UTILITAIRES
// ============================================================================

/**
 * Ferme proprement un socket
 * @param sSocket Descripteur de socket à fermer
 * @return 0 en cas de succès, -1 en cas d'erreur
 */
int closeSocket(int sSocket);

#endif // SOCKET_H