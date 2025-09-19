/**
 * Serveur de Réservation Multi-threads
 * 
 * Ce serveur implémente le protocole CBP (Consultation Booking Protocol)
 * pour la gestion des réservations de consultations médicales.
 * 
 * Fonctionnalités :
 * - Pool de threads pour gérer les connexions clients
 * - Interface avec base de données MySQL
 * - Protocole de communication sécurisé
 * - Configuration via fichier externe
 */

// ============================================================================
// INCLUDES
// ============================================================================
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>
#include <pthread.h>
#include <string>
#include <vector>
#include <queue>
#include <fstream>
#include <iostream>
#include <mysql.h>
#include "../util/name.h"
#include "../socket/socket.h"

using namespace std;

// ============================================================================
// CONSTANTES ET CONFIGURATION
// ============================================================================
const int BUFFER_SIZE = 1024;           // Taille du buffer de réception
const int QUERY_SIZE = 512;             // Taille maximale des requêtes SQL
const int MAX_PATIENT_NAME_LENGTH = 50; // Longueur maximale des noms

// Longueurs des commandes du protocole CBP
const int LOGIN_NEW_LENGTH = 10;         // "LOGIN_NEW;" = 10 caractères
const int LOGIN_EXIST_LENGTH = 12;       // "LOGIN_EXIST;" = 12 caractères
const int SEARCH_LENGTH = 7;             // "SEARCH;" = 7 caractères
const int GET_DOCTORS_LENGTH = 12;       // "GET_DOCTORS;" = 12 caractères
const int BOOK_CONSULTATION_LENGTH = 18; // "BOOK_CONSULTATION;" = 18 caractères

// ============================================================================
// STRUCTURES DE DONNÉES
// ============================================================================

/**
 * Configuration du serveur
 */
struct ServerConfig {
    int portReservation = 0;        // Port d'écoute du serveur
    int nbThreads = 4;              // Nombre de threads dans le pool
    string dbHost;             // Hôte de la base de données
    string dbUser;             // Utilisateur de la base de données
    string dbPass;             // Mot de passe de la base de données
    string dbName;             // Nom de la base de données
};

/**
 * Tâche client à traiter par un thread
 */
struct ClientTask {
    int socket;                     // Socket de communication avec le client
    char ip[INET_ADDRSTRLEN];       // Adresse IP du client
};

// ============================================================================
// VARIABLES GLOBALES
// ============================================================================
static ServerConfig config;                    // Configuration du serveur
static bool stop = false;                     // Flag d'arrêt du serveur
static queue<ClientTask> tasks;          // File de tâches à traiter
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;  // Mutex pour la file
static pthread_cond_t condition = PTHREAD_COND_INITIALIZER; // Condition pour les threads

// ============================================================================
// FONCTIONS UTILITAIRES
// ============================================================================

/**
 * Supprime les espaces en début et fin de chaîne
 * @param s Chaîne à nettoyer
 */
static void trim(string &s) {
    // Supprimer les espaces en fin de chaîne
    while (!s.empty() && (s.back() == '\n' || s.back() == '\r' || s.back() == ' ' || s.back() == '\t')) {
        s.pop_back();
    }
    
    // Supprimer les espaces en début de chaîne
    size_t i = 0;
    while (i < s.size() && (s[i] == ' ' || s[i] == '\t')) {
        ++i;
    }
    s.erase(0, i);
}

// ============================================================================
// GESTION DE LA CONFIGURATION
// ============================================================================

/**
 * Charge la configuration du serveur depuis un fichier
 * @param path Chemin vers le fichier de configuration
 * @param cfg Structure de configuration à remplir
 * @return true si le chargement a réussi, false sinon
 */
static bool loadConfig(const char *path, ServerConfig &cfg) {
    ifstream configFile(path);
    if (!configFile) {
        printf("ERREUR: Impossible d'ouvrir le fichier de configuration: %s\n", path);
        return false;
    }
    
    string line;
    while (getline(configFile, line)) {
        trim(line);
        
        // Ignorer les lignes vides et les commentaires
        if (line.empty() || line[0] == '#') {
            continue;
        }
        
        // Rechercher le séparateur '='
        auto pos = line.find('=');
        if (pos == string::npos) {
            continue;
        }
        
        // Extraire la clé et la valeur
        string key = line.substr(0, pos);
        string value = line.substr(pos + 1);
        trim(key);
        trim(value);
        
        // Parser les paramètres de configuration
        if (key == "PORT_RESERVATION") {
            cfg.portReservation = atoi(value.c_str());
        }
        else if (key == "NB_THREADS") {
            cfg.nbThreads = atoi(value.c_str());
        }
        else if (key == "DB_HOST") {
            cfg.dbHost = value;
        }
        else if (key == "DB_USER") {
            cfg.dbUser = value;
        }
        else if (key == "DB_PASS") {
            cfg.dbPass = value;
        }
        else if (key == "DB_NAME") {
            cfg.dbName = value;
        }
    }
    
    // Vérifier que le port est configuré
    return cfg.portReservation > 0;
}

// ============================================================================
// GESTION DE LA BASE DE DONNÉES
// ============================================================================

/**
 * Établit une connexion à la base de données MySQL
 * @return Pointeur vers la connexion MySQL ou NULL en cas d'échec
 */
static MYSQL *openDb() {
    MYSQL *connection = mysql_init(NULL);
    if (!connection) {
        printf("ERREUR: Impossible d'initialiser la connexion MySQL\n");
        return NULL;
    }
    
    if (!mysql_real_connect(connection, 
                           config.dbHost.c_str(), 
                           config.dbUser.c_str(), 
                           config.dbPass.c_str(), 
                           config.dbName.c_str(), 
                           0, NULL, 0)) {
        printf("ERREUR: Impossible de se connecter à la base de données: %s\n", mysql_error(connection));
        mysql_close(connection);
        return NULL;
    }
    
    return connection;
}

// ============================================================================
// GESTION DES PATIENTS
// ============================================================================

/**
 * Crée un nouveau patient dans la base de données
 * @param connection Connexion à la base de données
 * @param lastName Nom de famille du patient
 * @param firstName Prénom du patient
 * @return ID du patient créé ou -1 en cas d'échec
 */
static int createNewPatient(MYSQL *connection, const string &lastName, const string &firstName) {
    // Validation des entrées
    if (lastName.empty() || firstName.empty() || 
        lastName.length() > MAX_PATIENT_NAME_LENGTH || 
        firstName.length() > MAX_PATIENT_NAME_LENGTH) {
        printf("ERREUR: Données patient invalides (nom: %s, prénom: %s)\n", 
               lastName.c_str(), firstName.c_str());
        return -1;
    }

    // Échappement des caractères spéciaux pour éviter les injections SQL
    string escapedLastName = lastName;
    string escapedFirstName = firstName;

    // Remplacer les apostrophes par des apostrophes doubles
    size_t pos = 0;
    while ((pos = escapedLastName.find("'", pos)) != string::npos) {
        escapedLastName.replace(pos, 1, "''");
        pos += 2;
    }
    pos = 0;
    while ((pos = escapedFirstName.find("'", pos)) != string::npos) {
        escapedFirstName.replace(pos, 1, "''");
        pos += 2;
    }

    // Construction et exécution de la requête SQL
    char query[QUERY_SIZE];
    snprintf(query, sizeof(query), 
             "INSERT INTO patients (last_name, first_name, birth_date) VALUES ('%s','%s','2000-01-01')",
             escapedLastName.c_str(), escapedFirstName.c_str());
    
    if (mysql_query(connection, query) != 0) {
        printf("ERREUR: Échec de l'insertion du patient: %s\n", mysql_error(connection));
        return -1;
    }
    
    return (int)mysql_insert_id(connection);
}

/**
 * Vérifie l'existence d'un patient dans la base de données
 * @param connection Connexion à la base de données
 * @param patientId ID du patient à vérifier
 * @param lastName Nom de famille du patient
 * @param firstName Prénom du patient
 * @return true si le patient existe, false sinon
 */
static bool verifyExistingPatient(MYSQL *connection, int patientId, const string &lastName, const string &firstName) {
    char query[QUERY_SIZE];
    snprintf(query, sizeof(query), 
             "SELECT id FROM patients WHERE id=%d AND last_name='%s' AND first_name='%s'",
             patientId, lastName.c_str(), firstName.c_str());

    if (mysql_query(connection, query) != 0) {
        printf("ERREUR: Échec de la vérification du patient: %s\n", mysql_error(connection));
        return false;
    }

    MYSQL_RES *result = mysql_store_result(connection);
    if (!result) {
        printf("ERREUR: Impossible de stocker le résultat de la requête\n");
        return false;
    }

    bool patientExists = (mysql_num_rows(result) > 0);
    mysql_free_result(result);
    return patientExists;
}

// ============================================================================
// COMMUNICATION RÉSEAU
// ============================================================================

/**
 * Envoie une réponse au client
 * @param clientSocket Socket de communication avec le client
 * @param response Message de réponse à envoyer
 */
static void sendResponse(int clientSocket, const string &response) {
    int result = Send(clientSocket, response.c_str(), response.length());
    if (result < 0) {
        printf("ERREUR: Impossible d'envoyer la réponse au client\n");
    }
}

// ============================================================================
// GESTION DES REQUÊTES CLIENT
// ============================================================================

/**
 * Gère la connexion d'un nouveau patient
 * @param connection Connexion à la base de données
 * @param clientSocket Socket de communication avec le client
 * @param lastName Nom de famille du patient
 * @param firstName Prénom du patient
 */
static void handleLoginNew(MYSQL *connection, int clientSocket, const string &lastName, const string &firstName) {
    printf("Traitement LOGIN_NEW pour %s %s\n", lastName.c_str(), firstName.c_str());

    int patientId = createNewPatient(connection, lastName, firstName);
    if (patientId > 0) {
        // Succès : envoyer l'ID du nouveau patient
        string response = string(LOGIN_OK) + to_string(patientId);
        sendResponse(clientSocket, response);
        printf("Nouveau patient créé avec ID: %d\n", patientId);

        // Vérification optionnelle de la création
        char query[QUERY_SIZE];
        snprintf(query, sizeof(query), 
                 "SELECT id, last_name, first_name FROM patients WHERE id=%d", patientId);
        if (mysql_query(connection, query) == 0) {
            MYSQL_RES *result = mysql_store_result(connection);
            if (result && mysql_num_rows(result) > 0) {
                MYSQL_ROW row = mysql_fetch_row(result);
                printf("Vérification: Patient ID=%s, Nom=%s, Prénom=%s\n", 
                       row[0], row[1], row[2]);
            }
            mysql_free_result(result);
        }
    } else {
        // Échec : envoyer message d'erreur
        sendResponse(clientSocket, string(LOGIN_FAIL) + INSERT);
        printf("ERREUR: Échec de la création du patient %s %s\n", 
               lastName.c_str(), firstName.c_str());
    }
}

/**
 * Gère la connexion d'un patient existant
 * @param connection Connexion à la base de données
 * @param clientSocket Socket de communication avec le client
 * @param patientId ID du patient à vérifier
 * @param lastName Nom de famille du patient
 * @param firstName Prénom du patient
 */
static void handleLoginExist(MYSQL *connection, int clientSocket, int patientId, const string &lastName, const string &firstName) {
    printf("Traitement LOGIN_EXIST pour ID=%d, %s %s\n", patientId, lastName.c_str(), firstName.c_str());

    if (verifyExistingPatient(connection, patientId, lastName, firstName)) {
        // Succès : patient trouvé et vérifié
        string response = string(LOGIN_OK) + to_string(patientId);
        sendResponse(clientSocket, response);
        printf("Patient existant vérifié avec succès (ID: %d)\n", patientId);
    } else {
        // Échec : patient non trouvé ou données incorrectes
        sendResponse(clientSocket, string(LOGIN_FAIL) + NOT_FOUND);
        printf("ERREUR: Patient non trouvé ou données incorrectes (ID: %d, %s %s)\n", 
               patientId, lastName.c_str(), firstName.c_str());
    }
}

/**
 * Gère la recherche de consultations disponibles
 * @param connection Connexion à la base de données
 * @param clientSocket Socket de communication avec le client
 * @param specialty Spécialité recherchée (ou "--- TOUTES ---")
 * @param doctor Médecin recherché (ou "--- TOUS ---")
 * @param startDate Date de début de recherche
 * @param endDate Date de fin de recherche
 */
static void handleSearch(MYSQL *connection, int clientSocket, const string &specialty, const string &doctor, const string &startDate, const string &endDate) {
    printf("Traitement SEARCH: specialty=%s, doctor=%s, startDate=%s, endDate=%s\n",
           specialty.c_str(), doctor.c_str(), startDate.c_str(), endDate.c_str());

    // Construction de la requête SQL de base
    string query = "SELECT c.id, s.name, CONCAT(d.first_name, ' ', d.last_name), c.date, c.hour ";
    query += "FROM consultations c ";
    query += "JOIN doctors d ON c.doctor_id = d.id ";
    query += "JOIN specialties s ON d.specialty_id = s.id ";
    query += "WHERE c.patient_id IS NULL "; // Seulement les créneaux libres

    // Ajout des filtres selon les critères
    if (specialty != TOUTES) {
        query += "AND s.name = '" + specialty + "' ";
    }
    if (doctor != TOUS) {
        query += "AND CONCAT(d.first_name, ' ', d.last_name) = '" + doctor + "' ";
    }
    query += "AND c.date BETWEEN '" + startDate + "' AND '" + endDate + "' ";
    query += "ORDER BY c.date, c.hour";

    printf("Requête SQL: %s\n", query.c_str());

    // Exécution de la requête
    if (mysql_query(connection, query.c_str()) != 0) {
        sendResponse(clientSocket, string(SEARCH_FAIL) + DB);
        printf("ERREUR: Échec de la requête SQL: %s\n", mysql_error(connection));
        return;
    }

    // Récupération des résultats
    MYSQL_RES *result = mysql_store_result(connection);
    if (!result) {
        sendResponse(clientSocket, string(SEARCH_FAIL) + DB);
        printf("ERREUR: Impossible de stocker le résultat de la requête\n");
        return;
    }

    // Construction de la réponse
    string response = SEARCH_OK;
    int numRows = mysql_num_rows(result);
    printf("Nombre de consultations trouvées: %d\n", numRows);

    if (numRows == 0) {
        sendResponse(clientSocket, response);
        mysql_free_result(result);
        return;
    }

    // Formatage des résultats : ID;SPECIALTY;DOCTOR;DATE;HOUR
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result))) {
        if (!response.empty() && response.back() != ';') {
            response += "|";
        }
        response += string(row[0]) + ";" + string(row[1]) + ";" + 
                   string(row[2]) + ";" + string(row[3]) + ";" + string(row[4]);
    }

    mysql_free_result(result);
    sendResponse(clientSocket, response);
    printf("Réponse envoyée: %s\n", response.c_str());
}

/**
 * Gère la récupération de la liste des spécialités
 * @param connection Connexion à la base de données
 * @param clientSocket Socket de communication avec le client
 */
static void handleGetSpecialties(MYSQL *connection, int clientSocket) {
    printf("Traitement GET_SPECIALTIES\n");

    string query = "SELECT name FROM specialties ORDER BY name";

    if (mysql_query(connection, query.c_str()) != 0) {
        sendResponse(clientSocket, "SPECIALTIES_FAIL;DB");
        printf("ERREUR: Échec de la requête spécialités: %s\n", mysql_error(connection));
        return;
    }

    MYSQL_RES *result = mysql_store_result(connection);
    if (!result) {
        sendResponse(clientSocket, "SPECIALTIES_FAIL;DB");
        printf("ERREUR: Impossible de stocker le résultat spécialités\n");
        return;
    }

    // Construction de la réponse : SPECIALTIES_OK;SPEC1|SPEC2|SPEC3
    string response = SPECIALTIES_OK;
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result))) {
        if (!response.empty() && response.back() != ';') {
            response += "|";
        }
        response += string(row[0]);
    }

    mysql_free_result(result);
    sendResponse(clientSocket, response);
    printf("Spécialités envoyées: %s\n", response.c_str());
}

/**
 * Gère la récupération de la liste des médecins
 * @param connection Connexion à la base de données
 * @param clientSocket Socket de communication avec le client
 * @param specialty Spécialité pour filtrer les médecins (ou "--- TOUS ---")
 */
static void handleGetDoctors(MYSQL *connection, int clientSocket, const string &specialty) {
    printf("Traitement GET_DOCTORS pour spécialité: %s\n", specialty.c_str());

    // Construction de la requête SQL
    string query = "SELECT CONCAT(d.first_name, ' ', d.last_name) ";
    query += "FROM doctors d ";
    query += "JOIN specialties s ON d.specialty_id = s.id ";
    
    // Ajout du filtre de spécialité si nécessaire
    if (specialty != TOUS) {
        query += "WHERE s.name = '" + specialty + "' ";
    }
    query += "ORDER BY d.last_name, d.first_name";

    printf("Requête SQL GET_DOCTORS: %s\n", query.c_str());

    // Exécution de la requête
    if (mysql_query(connection, query.c_str()) != 0) {
        sendResponse(clientSocket, "DOCTORS_FAIL;DB");
        printf("ERREUR: Échec de la requête médecins: %s\n", mysql_error(connection));
        return;
    }

    // Récupération des résultats
    MYSQL_RES *result = mysql_store_result(connection);
    if (!result) {
        sendResponse(clientSocket, "DOCTORS_FAIL;DB");
        printf("ERREUR: Impossible de stocker le résultat médecins\n");
        return;
    }

    // Construction de la réponse : DOCTORS_OK;DOC1|DOC2|DOC3
    string response = DOCTORS_OK;
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result))) {
        if (!response.empty() && response.back() != ';') {
            response += "|";
        }
        response += string(row[0]);
    }

    mysql_free_result(result);
    sendResponse(clientSocket, response);
    printf("Médecins envoyés: %s\n", response.c_str());
}

/**
 * Gère la réservation d'une consultation
 * @param connection Connexion à la base de données
 * @param clientSocket Socket de communication avec le client
 * @param consultationId ID de la consultation à réserver
 * @param patientId ID du patient qui réserve
 * @param reason Raison de la consultation
 */
static void handleBookConsultation(MYSQL *connection, int clientSocket, int consultationId, int patientId, const string &reason) {
    printf("Traitement BOOK_CONSULTATION pour consultation ID=%d, patient ID=%d\n", consultationId, patientId);

    // Étape 1: Vérifier que la consultation existe et est libre
    char query[QUERY_SIZE];
    snprintf(query, sizeof(query), "SELECT id, patient_id FROM consultations WHERE id=%d", consultationId);

    if (mysql_query(connection, query) != 0) {
        sendResponse(clientSocket, string(BOOK_FAIL) + DB);
        printf("ERREUR: Échec de la vérification de la consultation: %s\n", mysql_error(connection));
        return;
    }

    MYSQL_RES *result = mysql_store_result(connection);
    if (!result) {
        sendResponse(clientSocket, string(BOOK_FAIL) + DB);
        printf("ERREUR: Impossible de stocker le résultat de vérification\n");
        return;
    }

    // Vérifier si la consultation existe
    if (mysql_num_rows(result) == 0) {
        mysql_free_result(result);
        sendResponse(clientSocket, string(BOOK_FAIL) + NOT_FOUND);
        printf("ERREUR: Consultation %d non trouvée\n", consultationId);
        return;
    }

    // Vérifier si la consultation est déjà réservée
    MYSQL_ROW row = mysql_fetch_row(result);
    if (row[1] != NULL) { // patient_id n'est pas NULL = déjà réservée
        mysql_free_result(result);
        sendResponse(clientSocket, string(BOOK_FAIL) + ALREADY_BOOKED);
        printf("ERREUR: Consultation %d déjà réservée par le patient %s\n", consultationId, row[1]);
        return;
    }

    mysql_free_result(result);

    // Étape 2: Effectuer la réservation
    snprintf(query, sizeof(query), 
             "UPDATE consultations SET patient_id=%d, reason='%s' WHERE id=%d",
             patientId, reason.c_str(), consultationId);

    if (mysql_query(connection, query) != 0) {
        sendResponse(clientSocket, string(BOOK_FAIL) + DB);
        printf("ERREUR: Échec de la réservation: %s\n", mysql_error(connection));
        return;
    }

    // Vérifier que la mise à jour a bien eu lieu
    if (mysql_affected_rows(connection) > 0) {
        sendResponse(clientSocket, BOOK_OK);
        printf("SUCCÈS: Consultation %d réservée pour le patient %d (raison: %s)\n", 
               consultationId, patientId, reason.c_str());
    } else {
        sendResponse(clientSocket, string(BOOK_FAIL) + UPDATE_FAILED);
        printf("ERREUR: Aucune ligne mise à jour lors de la réservation\n");
    }
}

// ============================================================================
// GESTION DES THREADS
// ============================================================================

/**
 * Fonction exécutée par chaque thread du pool
 * @param arg Paramètre non utilisé
 * @return NULL
 */
static void *workerThread(void *arg) {
    (void)arg; // Éviter l'avertissement du compilateur
    
    while (true) {
        // Attendre une tâche à traiter
        pthread_mutex_lock(&mutex);
        while (tasks.empty() && !stop) {
            pthread_cond_wait(&condition, &mutex);
        }
        
        // Vérifier si on doit s'arrêter
        if (stop && tasks.empty()) {
            pthread_mutex_unlock(&mutex);
            break;
        }
        
        // Récupérer la tâche à traiter
        ClientTask task = tasks.front();
        tasks.pop();
        pthread_mutex_unlock(&mutex);

        // Traitement de la connexion client
        printf("Thread traite la connexion de %s (socket %d)\n", task.ip, task.socket);

        // Connexion à la base de données pour ce thread
        MYSQL *connection = openDb();
        if (!connection) {
            printf("ERREUR: Impossible de se connecter à la base de données\n");
            closeSocket(task.socket);
            continue;
        }

        // Boucle de traitement des messages du client
        char buffer[BUFFER_SIZE];
        bool clientConnected = true;
        
        while (clientConnected) {
            // Réception du message du client
            int bytesReceived = Receive(task.socket, buffer);

            if (bytesReceived <= 0) {
                // Client déconnecté
                printf("Client %s déconnecté (socket %d)\n", task.ip, task.socket);
                clientConnected = false;
            } else {
                // Traitement du message reçu
                string message(buffer);
                printf("Message reçu de %s: %s\n", task.ip, buffer);

                // ================================================================
                // PARSING ET TRAITEMENT DES COMMANDES CBP
                // ================================================================
                
                // Commande: LOGIN_NEW (nouveau patient)
                if (message.find(LOGIN_NEW) == 0) {
                    // Format: LOGIN_NEW;NOM;PRENOM
                    size_t pos1 = message.find(';', LOGIN_NEW_LENGTH);
                    if (pos1 != string::npos) {
                        string lastName = message.substr(LOGIN_NEW_LENGTH, pos1 - LOGIN_NEW_LENGTH);
                        string firstName = message.substr(pos1 + 1);
                        handleLoginNew(connection, task.socket, lastName, firstName);
                    } else {
                        sendResponse(task.socket, string(LOGIN_FAIL) + FORMAT);
                    }
                }
                // Commande: LOGIN_EXIST (patient existant)
                else if (message.find(LOGIN_EXIST) == 0) {
                    // Format: LOGIN_EXIST;ID;NOM;PRENOM
                    size_t pos1 = message.find(';', LOGIN_EXIST_LENGTH);
                    size_t pos2 = message.find(';', pos1 + 1);
                    if (pos1 != string::npos && pos2 != string::npos) {
                        int patientId = atoi(message.substr(LOGIN_EXIST_LENGTH, pos1 - LOGIN_EXIST_LENGTH).c_str());
                        string lastName = message.substr(pos1 + 1, pos2 - pos1 - 1);
                        string firstName = message.substr(pos2 + 1);
                        handleLoginExist(connection, task.socket, patientId, lastName, firstName);
                    } else {
                        sendResponse(task.socket, string(LOGIN_FAIL) + FORMAT);
                    }
                }
                // Commande: SEARCH (recherche de consultations)
                else if (message.find(SEARCH) == 0) {
                    // Format: SEARCH;SPECIALTY;DOCTOR;START_DATE;END_DATE
                    size_t pos1 = message.find(';', SEARCH_LENGTH);
                    size_t pos2 = message.find(';', pos1 + 1);
                    size_t pos3 = message.find(';', pos2 + 1);

                    if (pos1 != string::npos && pos2 != string::npos && pos3 != string::npos) {
                        string specialty = message.substr(SEARCH_LENGTH, pos1 - SEARCH_LENGTH);
                        string doctor = message.substr(pos1 + 1, pos2 - pos1 - 1);
                        string startDate = message.substr(pos2 + 1, pos3 - pos2 - 1);
                        string endDate = message.substr(pos3 + 1);
                        handleSearch(connection, task.socket, specialty, doctor, startDate, endDate);
                    } else {
                        sendResponse(task.socket, string(SEARCH_FAIL) + FORMAT);
                    }
                }
                // Commande: GET_SPECIALTIES (liste des spécialités)
                else if (message.find(GET_SPECIALTIES) == 0) {
                    handleGetSpecialties(connection, task.socket);
                }
                // Commande: GET_DOCTORS (liste des médecins)
                else if (message.find("GET_DOCTORS;") == 0) {
                    // Format: GET_DOCTORS;SPECIALTY
                    string specialty = message.substr(GET_DOCTORS_LENGTH);
                    handleGetDoctors(connection, task.socket, specialty);
                }
                // Commande: BOOK_CONSULTATION (réservation de consultation)
                else if (message.find("BOOK_CONSULTATION;") == 0) {
                    // Format: BOOK_CONSULTATION;CONSULTATION_ID;PATIENT_ID;REASON
                    size_t pos1 = message.find(';', BOOK_CONSULTATION_LENGTH);
                    size_t pos2 = message.find(';', pos1 + 1);

                    if (pos1 != string::npos && pos2 != string::npos) {
                        int consultationId = atoi(message.substr(BOOK_CONSULTATION_LENGTH, pos1 - BOOK_CONSULTATION_LENGTH).c_str());
                        int patientId = atoi(message.substr(pos1 + 1, pos2 - pos1 - 1).c_str());
                        string reason = message.substr(pos2 + 1);
                        handleBookConsultation(connection, task.socket, consultationId, patientId, reason);
                    } else {
                        sendResponse(task.socket, string(BOOK_FAIL) + FORMAT);
                    }
                }
                // Commande inconnue
                else {
                    sendResponse(task.socket, string(LOGIN_FAIL) + UNKNOWN_CMD);
                    printf("ERREUR: Commande inconnue reçue: %s\n", message.c_str());
                }
            }
        }

        // ================================================================
        // NETTOYAGE ET FERMETURE
        // ================================================================

        // Fermer la connexion à la base de données
        mysql_close(connection);
        printf("Connexion base de données fermée pour le client %s\n", task.ip);

        // Fermeture propre du socket client
        closeSocket(task.socket);
        printf("Socket %d fermé pour le client %s\n", task.socket, task.ip);
    }
    
    printf("Thread worker terminé\n");
    return nullptr;
}

// ============================================================================
// FONCTION PRINCIPALE
// ============================================================================

/**
 * Point d'entrée du programme serveur
 * @return Code de sortie (0 = succès, 1 = erreur)
 */
int main() {
    printf("=== SERVEUR DE RÉSERVATION MULTI-THREADS ===\n");
    printf("Démarrage du serveur...\n");
    
    // ================================================================
    // CHARGEMENT DE LA CONFIGURATION
    // ================================================================
    
    // Essayer plusieurs chemins possibles pour le fichier de configuration
    const char *configPaths[] = {
        "conf/serveur.conf",           // Depuis la racine du projet
        "../conf/serveur.conf",        // Depuis le répertoire serveur/
        "serveur.conf"                 // Dans le répertoire courant
    };

    bool configLoaded = false;
    for (int i = 0; i < 3; i++) {
        if (loadConfig(configPaths[i], config)) {
            configLoaded = true;
            printf("Configuration chargée depuis: %s\n", configPaths[i]);
            break;
        }
    }

    if (!configLoaded) {
        fprintf(stderr, "ERREUR: Impossible de charger la configuration serveur.conf\n");
        return 1;
    }
    
    // Validation de la configuration
    if (config.nbThreads <= 0) {
        config.nbThreads = 4;
        printf("ATTENTION: Nombre de threads invalide, utilisation de la valeur par défaut: 4\n");
    }
    
    printf("Configuration chargée: port=%d threads=%d DB=%s@%s (%s)\n", 
           config.portReservation, config.nbThreads, 
           config.dbUser.c_str(), config.dbHost.c_str(), config.dbName.c_str());

    // ================================================================
    // INITIALISATION DU SERVEUR
    // ================================================================

    int serverSocket = ServerSocket(config.portReservation);
    if (serverSocket < 0) {
        perror("ERREUR: Impossible de créer le socket serveur");
        return 1;
    }
    printf("Serveur en écoute sur le port %d\n", config.portReservation);

    // ================================================================
    // CRÉATION DU POOL DE THREADS
    // ================================================================
    
    vector<pthread_t> threads(config.nbThreads);
    for (int i = 0; i < config.nbThreads; ++i) {
        if (pthread_create(&threads[i], nullptr, workerThread, nullptr) != 0) {
            perror("ERREUR: Impossible de créer le thread");
            closeSocket(serverSocket);
            return 1;
        }
    }
    printf("Pool de %d threads créé avec succès\n", config.nbThreads);

    // ================================================================
    // BOUCLE PRINCIPALE - ACCEPTATION DES CONNEXIONS
    // ================================================================
    
    printf("Serveur prêt à accepter les connexions...\n");
    while (!stop) {
        char ipClient[INET_ADDRSTRLEN] = {0};
        int clientSocket = AcceptConnection(serverSocket, ipClient);
        
        if (clientSocket < 0) {
            perror("ERREUR: AcceptConnection");
            continue;
        }
        
        printf("Connexion acceptée de %s (socket %d)\n", ipClient, clientSocket);
        
        // Ajouter la tâche à la file d'attente
        pthread_mutex_lock(&mutex);
        tasks.push({clientSocket, {0}});
        strncpy(tasks.back().ip, ipClient, INET_ADDRSTRLEN - 1);
        pthread_cond_signal(&condition);
        pthread_mutex_unlock(&mutex);
        
        printf("Tâche ajoutée à la file d'attente\n");
    }

    // ================================================================
    // ARRÊT PROPRE DU SERVEUR
    // ================================================================
    
    printf("Arrêt du serveur demandé...\n");
    stop = true;
    pthread_cond_broadcast(&condition);
    
    // Attendre que tous les threads se terminent
    for (auto &thread : threads) {
        pthread_join(thread, nullptr);
    }
    printf("Tous les threads terminés\n");
    
    // Fermer le socket serveur
    closeSocket(serverSocket);
    printf("Serveur arrêté proprement\n");
    
    return 0;
}