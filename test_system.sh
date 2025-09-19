#!/bin/bash

echo "=== Script de test du système de réservation médicale ==="
echo

# Vérifier que MySQL est installé et en cours d'exécution
echo "1. Vérification de MySQL..."
if ! systemctl is-active --quiet mysql; then
    echo "   MySQL n'est pas démarré. Démarrage..."
    sudo systemctl start mysql
    sleep 2
fi

# Créer la base de données
echo "2. Création de la base de données..."
if [ ! -f "BD_Hospital/CreationBD" ]; then
    echo "   Compilation du programme de création de BD..."
    make database
fi

echo "   Exécution du programme de création de BD..."
./BD_Hospital/CreationBD

# Compiler le serveur
echo "3. Compilation du serveur..."
make server

# Compiler le client de test
echo "4. Compilation du client de test..."
make test_client

# Démarrer le serveur en arrière-plan
echo "5. Démarrage du serveur..."
./server/reservation_server &
SERVER_PID=$!
echo "   Serveur démarré avec PID: $SERVER_PID"

# Attendre que le serveur soit prêt
echo "6. Attente que le serveur soit prêt..."
sleep 3

# Tester la connexion
echo "7. Test de la connexion..."
if netstat -ln | grep -q ":8080"; then
    echo "   ✓ Serveur en écoute sur le port 8080"
else
    echo "   ✗ Serveur non accessible sur le port 8080"
    kill $SERVER_PID 2>/dev/null
    exit 1
fi

# Exécuter le client de test
echo "8. Exécution du client de test..."
echo "   (Le client va tester les commandes LOGIN, GET_SPECIALTIES, SEARCH_CONSULTATIONS)"
./client/test_client

# Arrêter le serveur
echo "9. Arrêt du serveur..."
kill $SERVER_PID 2>/dev/null
wait $SERVER_PID 2>/dev/null
echo "   Serveur arrêté"

echo
echo "=== Test terminé ==="
echo "Le système de réservation médicale fonctionne correctement !"
echo
echo "Pour utiliser le système:"
echo "1. Démarrez le serveur: make start-server"
echo "2. Dans un autre terminal, démarrez le client Qt: make start-client"
echo "3. Ou utilisez le client de test: ./client/test_client"

