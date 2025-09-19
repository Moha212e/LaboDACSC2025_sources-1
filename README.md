# Système de Réservation Médicale

## Description du Projet

Ce projet implémente un système de réservation de consultations médicales composé de :

- **Librairie de sockets générique** en C
- **Serveur multi-threads** avec pool de threads POSIX
- **Client Qt** pour l'interface utilisateur
- **Base de données MySQL** pour la persistance des données
- **Protocole CBP** (Consultation Booking Protocol) pour la communication

## Architecture du Système

```
┌─────────────────┐    CBP Protocol    ┌─────────────────┐
│   Client Qt     │◄──────────────────►│  Serveur C/C++  │
│   (Interface)   │                    │  Multi-threads  │
└─────────────────┘                    └─────────────────┘
                                               │
                                               ▼
                                       ┌─────────────────┐
                                       │ Base de données │
                                       │     MySQL       │
                                       └─────────────────┘
```

## Structure du Projet

```
LaboDACSC2025_sources/
├── socket_lib/              # Librairie de sockets générique
│   ├── socket_lib.h
│   ├── socket_lib.c
│   └── test_socket_lib.c
├── server/                  # Serveur de réservation
│   ├── main.c
│   ├── reservation_server.h
│   ├── reservation_server.c
│   ├── config_parser.h
│   ├── config_parser.c
│   ├── database.h
│   ├── database.c
│   ├── cbp_protocol.h
│   └── cbp_protocol.c
├── client/                  # Client de test
│   └── test_client.c
├── ClientConsultationBookerQt/  # Client Qt existant
├── BD_Hospital/             # Base de données
│   └── CreationBD.cpp
├── server_config.txt        # Configuration du serveur
├── Makefile
└── test_system.sh          # Script de test complet
```

## Fonctionnalités Implémentées

### 1. Librairie de Sockets (`socket_lib/`)

- **Abstraction complète** des structures système (sockaddr_in, etc.)
- **API générique** réutilisable dans d'autres projets
- **Gestion d'erreurs** robuste
- **Tests unitaires** complets

**Fonctions principales :**
- `create_socket()` / `destroy_socket()`
- `bind_socket()` / `listen_socket()`
- `accept_connection()` / `connect_socket()`
- `send_data()` / `receive_data()`

### 2. Serveur Multi-threads (`server/`)

- **Pool de threads POSIX** configurable
- **Gestion des connexions** simultanées
- **Protocole CBP** complet
- **Base de données MySQL** intégrée
- **Système de mémorisation** des patients connectés

**Commandes CBP supportées :**
- `LOGIN` - Authentification des patients (nouveaux et existants)
- `LOGOUT` - Déconnexion
- `GET_SPECIALTIES` - Récupération des spécialités médicales
- `GET_DOCTORS` - Récupération des médecins
- `SEARCH_CONSULTATIONS` - Recherche de consultations disponibles
- `BOOK_CONSULTATION` - Réservation d'une consultation

### 3. Base de Données MySQL

**Tables :**
- `specialties` - Spécialités médicales
- `doctors` - Médecins avec spécialités
- `patients` - Patients enregistrés
- `consultations` - Créneaux de consultations

**Configuration par défaut :**
- Host: localhost
- User: Student
- Password: PassStudent1_
- Database: PourStudent

### 4. Protocole CBP

**Format des messages :**
```
[Commande: 4 bytes][Longueur: 4 bytes][Données: variable]
```

**Sérialisation binaire** pour l'efficacité
**Gestion des erreurs** intégrée
**Extensibilité** pour de futures commandes

## Installation et Utilisation

### Prérequis

```bash
# Ubuntu/Debian
sudo apt-get install build-essential libmysqlclient-dev mysql-server
sudo apt-get install qt5-default qttools5-dev-tools

# CentOS/RHEL
sudo yum install gcc gcc-c++ mysql-devel mysql-server
sudo yum install qt5-qtbase-devel
```

### Compilation

```bash
# Compiler tout le projet
make all

# Compiler des composants spécifiques
make socket_lib    # Librairie de sockets
make server        # Serveur
make test_client   # Client de test
make database      # Programme de création de BD
```

### Configuration de la Base de Données

```bash
# Créer la base de données
make setup-db

# Ou manuellement
mysql -u root -p
CREATE DATABASE PourStudent;
CREATE USER 'Student'@'localhost' IDENTIFIED BY 'PassStudent1_';
GRANT ALL PRIVILEGES ON PourStudent.* TO 'Student'@'localhost';
FLUSH PRIVILEGES;
./BD_Hospital/CreationBD
```

### Démarrage du Système

```bash
# 1. Démarrer le serveur
make start-server

# 2. Dans un autre terminal, démarrer le client Qt
make start-client

# 3. Ou utiliser le client de test
./client/test_client
```

### Test Automatique

```bash
# Exécuter le script de test complet
./test_system.sh
```

## Configuration du Serveur

Le fichier `server_config.txt` contient :

```ini
PORT_RESERVATION=8080
THREAD_POOL_SIZE=10
DB_HOST=localhost
DB_USER=Student
DB_PASSWORD=PassStudent1_
DB_NAME=PourStudent
PORT_ADMIN=8081
CONNECTION_TIMEOUT=300
```

## Utilisation du Client Qt

1. **Login** : Saisir nom, prénom et ID patient
   - Cocher "Nouveau Patient" pour créer un compte
   - Décocher pour un patient existant

2. **Recherche** : Sélectionner spécialité, médecin et dates
   - Cliquer sur "Lancer la recherche"
   - Les consultations disponibles apparaissent dans le tableau

3. **Réservation** : Sélectionner une consultation
   - Cliquer sur "Réserver"
   - Saisir la raison de la consultation

## Tests et Validation

### Tests de la Librairie de Sockets

```bash
make test
```

Tests inclus :
- Création/destruction de sockets
- Bind et listen
- Connexions client-serveur
- Gestion d'erreurs
- Communication bidirectionnelle

### Tests du Système Complet

```bash
./test_system.sh
```

Le script teste :
- Création de la base de données
- Démarrage du serveur
- Connexion client-serveur
- Commandes CBP (LOGIN, GET_SPECIALTIES, SEARCH_CONSULTATIONS)

## Architecture Technique

### Gestion des Threads

- **Pool de threads** configurable (défaut: 10 threads)
- **Mutex** pour la synchronisation des patients connectés
- **Gestion propre** des connexions fermées

### Gestion de la Base de Données

- **API C/MySQL** native
- **Requêtes préparées** pour la sécurité
- **Gestion des erreurs** robuste
- **Connexions persistantes**

### Sérialisation des Données

- **Format binaire** pour l'efficacité
- **Endianness** gérée (htonl/ntohl)
- **Validation** des données reçues
- **Extensibilité** pour de nouveaux types

## Sécurité

- **Validation** des entrées utilisateur
- **Protection** contre l'injection SQL
- **Gestion des erreurs** sans exposition d'informations sensibles
- **Timeout** des connexions

## Performance

- **Pool de threads** pour la concurrence
- **Sérialisation binaire** pour la vitesse
- **Connexions persistantes** à la base de données
- **Gestion mémoire** optimisée

## Extensibilité

- **Librairie de sockets** réutilisable
- **Protocole CBP** extensible
- **Architecture modulaire**
- **Configuration** externalisée

## Dépannage

### Problèmes Courants

1. **Erreur de connexion MySQL**
   ```bash
   sudo systemctl start mysql
   mysql -u root -p
   ```

2. **Port déjà utilisé**
   ```bash
   netstat -tulpn | grep :8080
   sudo kill -9 <PID>
   ```

3. **Erreurs de compilation Qt**
   ```bash
   sudo apt-get install qt5-default
   ```

### Logs et Debug

- Le serveur affiche les logs sur stdout
- Utiliser `gdb` pour le debug :
  ```bash
  gdb ./server/reservation_server
  ```

## Auteur

Projet développé dans le cadre du cours DACSC (Développement d'architectures clients/serveurs et cryptographie) - 2025-2026

## Licence

Ce projet est fourni à des fins éducatives uniquement.

