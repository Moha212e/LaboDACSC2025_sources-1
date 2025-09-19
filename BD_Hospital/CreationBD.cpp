#include <stdio.h>
#include <stdlib.h>
#include <mysql.h>
#include <time.h>
#include <string.h>

typedef struct {
  int  id;
  char name[50];
} SPECIALTY;

typedef struct {
  int  id;
  int  specialty_id;
  char last_name[50];
  char first_name[50];
} DOCTOR;

typedef struct {
  int  id;
  char last_name[50];
  char first_name[50];
  char birth_date[20];
} PATIENT;

typedef struct {
  int  id;
  int  doctor_id;
  int  patient_id; // -1 => NULL
  char date[20];
  char hour[10]; // stocké en TIME dans la BD
  char reason[255];
} CONSULTATION;

typedef struct {
  int  id;
  int  consultation_id;
  char description[255];
} REPORT;

SPECIALTY specialties[] = {
  {-1, "Cardiologie"},
  {-1, "Dermatologie"},
  {-1, "Neurologie"},
  {-1, "Ophtalmologie"},
  {-1, "Pédiatrie"},
  {-1, "Gynécologie"},
  {-1, "Orthopédie"},
  {-1, "Psychiatrie"},
  {-1, "Radiologie"},
  {-1, "Chirurgie générale"},
  {-1, "Médecine interne"},
  {-1, "Endocrinologie"},
  {-1, "Gastro-entérologie"},
  {-1, "Pneumologie"},
  {-1, "Urologie"}
};
int nbSpecialties = 15;

DOCTOR doctors[] = {
  {-1, 1, "Dupont", "Alice"},
  {-1, 2, "Lemoine", "Bernard"},
  {-1, 3, "Martin", "Claire"},
  {-1, 2, "Maboul", "Paul"},
  {-1, 1, "Coptere", "Elie"},
  {-1, 3, "Merad", "Gad"},
  {-1, 4, "Benali", "Mohammed"},
  {-1, 5, "Shala", "Donika"},
  {-1, 5, "Azrou", "Nassim"},
  {-1, 4, "Kova", "Zafina"},
  {-1, 6, "Moreau", "Isabelle"},
  {-1, 7, "Rousseau", "Pierre"},
  {-1, 8, "Dubois", "Marie"},
  {-1, 9, "Leroy", "François"},
  {-1, 10, "Petit", "Catherine"},
  {-1, 11, "Simon", "Antoine"},
  {-1, 12, "Laurent", "Sylvie"},
  {-1, 13, "Garcia", "Miguel"},
  {-1, 14, "Thomas", "Julie"},
  {-1, 15, "Robert", "Philippe"},
  {-1, 1, "Chen", "Wei"},
  {-1, 2, "Johnson", "Sarah"},
  {-1, 3, "Brown", "Michael"},
  {-1, 4, "Wilson", "Emma"},
  {-1, 5, "Davis", "James"}
};
int nbDoctors = 25;

PATIENT patients[] = {
  {-1, "Durand", "Jean", "1980-05-12"},
  {-1, "Petit", "Sophie", "1992-11-30"},
  {-1, "Benali", "Mohammed", "1987-01-09"},
  {-1, "Shala", "Donika", "1995-03-21"},
  {-1, "Azrou", "Nassim", "2001-07-14"},
  {-1, "Kova", "Zafina", "1998-09-02"},
  {-1, "Lefebvre", "Camille", "1990-04-15"},
  {-1, "Moreau", "Thomas", "1985-08-22"},
  {-1, "Rousseau", "Amélie", "1993-12-03"},
  {-1, "Dubois", "Nicolas", "1978-06-18"},
  {-1, "Leroy", "Chloé", "2000-02-28"},
  {-1, "Simon", "Lucas", "1982-10-11"},
  {-1, "Laurent", "Emma", "1996-07-25"},
  {-1, "Garcia", "Carlos", "1989-03-14"},
  {-1, "Thomas", "Léa", "1994-09-07"},
  {-1, "Robert", "Alexandre", "1983-11-30"},
  {-1, "Chen", "Li", "1991-05-20"},
  {-1, "Johnson", "Emily", "1986-01-12"},
  {-1, "Brown", "David", "1997-08-05"},
  {-1, "Wilson", "Jessica", "1984-12-16"},
  {-1, "Davis", "Christopher", "1999-04-09"},
  {-1, "Miller", "Sarah", "1981-07-23"},
  {-1, "Garcia", "Maria", "1992-10-31"},
  {-1, "Martinez", "Jose", "1988-06-14"},
  {-1, "Anderson", "Jennifer", "1995-03-27"}
};
int nbPatients = 25;

CONSULTATION consultations[] = {
  {-1, 1,  1, "2025-10-01", "09:00", "Check-up"},
  {-1, 2,  2, "2025-10-02", "14:30", "Premier rendez-vous"},
  {-1, 3,  3, "2025-10-03", "11:15", "Douleurs persistantes"},
  {-1, 4, -1, "2025-10-04", "09:00", ""},
  {-1, 4, -1, "2025-10-04", "09:30", ""},
  {-1, 4, -1, "2025-10-04", "10:00", ""},
  {-1, 4, -1, "2025-10-04", "10:30", ""},
  {-1, 5, -1, "2025-10-07", "13:00", ""},
  {-1, 5, -1, "2025-10-07", "13:30", ""},
  {-1, 5, -1, "2025-10-07", "14:00", ""},
  {-1, 5, -1, "2025-10-07", "14:30", ""},
  {-1, 6,  4, "2025-10-05", "10:00", "Suivi tension"},
  {-1, 7,  5, "2025-10-06", "10:15", "Rougeurs peau"},
  {-1, 8,  6, "2025-10-06", "11:45", "Migraine chronique"},
  {-1, 9,  7, "2025-10-06", "09:30", "Vision trouble"},
  {-1, 10, 8, "2025-10-08", "08:30", "Consultation gynécologique"},
  {-1, 11, 9, "2025-10-08", "10:00", "Fracture du poignet"},
  {-1, 12, 10, "2025-10-08", "14:00", "Anxiété généralisée"},
  {-1, 13, 11, "2025-10-09", "09:15", "Radiographie thorax"},
  {-1, 14, 12, "2025-10-09", "11:00", "Appendicite suspectée"},
  {-1, 15, 13, "2025-10-09", "15:30", "Diabète type 2"},
  {-1, 16, 14, "2025-10-10", "08:45", "Douleurs abdominales"},
  {-1, 17, 15, "2025-10-10", "10:30", "Asthme chronique"},
  {-1, 18, 16, "2025-10-10", "14:15", "Problèmes urinaires"},
  {-1, 19, 17, "2025-10-11", "09:00", "Contrôle cardiaque"},
  {-1, 20, 18, "2025-10-11", "11:30", "Eczéma sévère"},
  {-1, 21, 19, "2025-10-11", "15:00", "Troubles de la mémoire"},
  {-1, 22, 20, "2025-10-12", "08:00", "Examen ophtalmologique"},
  {-1, 23, 21, "2025-10-12", "10:45", "Vaccination enfant"},
  {-1, 24, 22, "2025-10-12", "13:30", "Consultation de routine"},
  {-1, 25, 23, "2025-10-13", "09:30", "Arythmie cardiaque"},
  {-1, 1, 24, "2025-10-13", "11:15", "Psoriasis"},
  {-1, 2, 25, "2025-10-13", "14:45", "Épilepsie"},
  {-1, 3, 1, "2025-10-14", "08:30", "Cataracte"},
  {-1, 4, 2, "2025-10-14", "10:00", "Fièvre enfant"},
  {-1, 5, 3, "2025-10-14", "15:30", "Grossesse"},
  {-1, 6, 4, "2025-10-15", "09:45", "Entorse cheville"},
  {-1, 7, 5, "2025-10-15", "11:30", "Dépression"},
  {-1, 8, 6, "2025-10-15", "14:00", "Scanner cérébral"},
  {-1, 9, 7, "2025-10-16", "08:15", "Cholécystite"},
  {-1, 10, 8, "2025-10-16", "10:30", "Hypothyroïdie"},
  {-1, 11, 9, "2025-10-16", "13:00", "Reflux gastro-œsophagien"},
  {-1, 12, 10, "2025-10-17", "09:00", "Bronchite chronique"},
  {-1, 13, 11, "2025-10-17", "11:45", "Calculs rénaux"},
  {-1, 14, 12, "2025-10-17", "15:15", "Infection urinaire"}
};
int nbConsultations = 45;

REPORT reports[] = {
  {-1, 1, "RAS"},
  {-1, 2, "Diagnostic initial, à confirmer"},
  {-1, 3, "Prescription anti-douleur"},
  {-1, 10, "Examen gynécologique normal, suivi dans 6 mois"},
  {-1, 11, "Fracture du radius, plâtre recommandé pour 6 semaines"},
  {-1, 12, "Anxiété confirmée, traitement par thérapie cognitive"},
  {-1, 13, "Radiographie normale, pas de pathologie pulmonaire"},
  {-1, 14, "Appendicite confirmée, intervention chirurgicale nécessaire"},
  {-1, 15, "Diabète type 2 diagnostiqué, régime et metformine prescrits"},
  {-1, 16, "Coloscopie recommandée pour douleurs abdominales"},
  {-1, 17, "Asthme confirmé, inhalateur prescrit"},
  {-1, 18, "Infection urinaire, antibiotiques prescrits"},
  {-1, 19, "Électrocardiogramme normal, suivi dans 1 an"},
  {-1, 20, "Eczéma atopique, crème corticoïde prescrite"},
  {-1, 21, "Tests cognitifs normaux, suivi neurologique"},
  {-1, 22, "Vision normale, pas de correction nécessaire"},
  {-1, 23, "Vaccination effectuée selon calendrier"},
  {-1, 24, "Consultation de routine, état général satisfaisant"},
  {-1, 25, "Arythmie bénigne, surveillance recommandée"},
  {-1, 26, "Psoriasis confirmé, traitement local prescrit"},
  {-1, 27, "Épilepsie stabilisée, ajustement médicamenteux"},
  {-1, 28, "Cataracte débutante, suivi ophtalmologique"},
  {-1, 29, "Infection virale, repos et paracétamol"},
  {-1, 30, "Grossesse normale, échographie de contrôle"},
  {-1, 31, "Entorse bénigne, repos et anti-inflammatoires"},
  {-1, 32, "Dépression modérée, antidépresseurs prescrits"},
  {-1, 33, "Scanner normal, pas de lésion cérébrale"},
  {-1, 34, "Cholécystite aiguë, intervention chirurgicale urgente"},
  {-1, 35, "Hypothyroïdie confirmée, traitement hormonal"},
  {-1, 36, "Reflux gastro-œsophagien, inhibiteurs de pompe prescrits"},
  {-1, 37, "Bronchite chronique, bronchodilatateurs prescrits"},
  {-1, 38, "Calculs rénaux, traitement médical et hydratation"},
  {-1, 39, "Infection urinaire récidivante, antibiothérapie prolongée"}
};
int nbReports = 33;

void finish_with_error(MYSQL *con) {
  fprintf(stderr, "%s\n", mysql_error(con));
  mysql_close(con);
  exit(1);
}

int main() {
  MYSQL* connexion = mysql_init(NULL);
  if (!mysql_real_connect(connexion, "localhost", "Student", "PassStudent1_", "PourStudent", 0, NULL, 0)) {
    finish_with_error(connexion);
  }

  mysql_query(connexion, "DROP TABLE IF EXISTS reports;");
  mysql_query(connexion, "DROP TABLE IF EXISTS consultations;");
  mysql_query(connexion, "DROP TABLE IF EXISTS patients;");
  mysql_query(connexion, "DROP TABLE IF EXISTS doctors;");
  mysql_query(connexion, "DROP TABLE IF EXISTS specialties;");

  if (mysql_query(connexion, "CREATE TABLE specialties ("
                             "id INT AUTO_INCREMENT PRIMARY KEY,"
                             "name VARCHAR(50) NOT NULL,"
                             "UNIQUE KEY uk_specialty_name(name)"
                             ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;"))
    finish_with_error(connexion);

  if (mysql_query(connexion, "CREATE TABLE doctors ("
                             "id INT AUTO_INCREMENT PRIMARY KEY, "
                             "specialty_id INT NOT NULL, "
                             "last_name VARCHAR(50) NOT NULL, "
                             "first_name VARCHAR(50) NOT NULL, "
                             "FOREIGN KEY (specialty_id) REFERENCES specialties(id)"
                             ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;"))
    finish_with_error(connexion);

  if (mysql_query(connexion, "CREATE TABLE patients ("
                             "id INT AUTO_INCREMENT PRIMARY KEY, "
                             "last_name VARCHAR(50) NOT NULL, "
                             "first_name VARCHAR(50) NOT NULL, "
                             "birth_date DATE NOT NULL"
                             ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;"))
    finish_with_error(connexion);

  if (mysql_query(connexion, "CREATE TABLE consultations ("
                             "id INT AUTO_INCREMENT PRIMARY KEY, "
                             "doctor_id INT NOT NULL, "
                             "patient_id INT NULL, "
                             "date DATE NOT NULL, "
                             "hour TIME NOT NULL, "
                             "reason VARCHAR(255) NOT NULL DEFAULT '', "
                             "FOREIGN KEY (doctor_id) REFERENCES doctors(id), "
                             "FOREIGN KEY (patient_id) REFERENCES patients(id) ON DELETE SET NULL, "
                             "UNIQUE KEY uk_slot (doctor_id, date, hour)"
                             ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;"))
    finish_with_error(connexion);
  if (mysql_query(connexion, "CREATE TABLE reports ("
                             "id INT AUTO_INCREMENT PRIMARY KEY, "
                             "consultation_id INT NOT NULL, "
                             "description TEXT NOT NULL, "
                             "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP, "
                             "FOREIGN KEY (consultation_id) REFERENCES consultations(id) ON DELETE CASCADE"
                             ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;"))
    finish_with_error(connexion);

  char request[512];
  for (int i = 0; i < nbSpecialties; i++) {
    sprintf(request, "INSERT INTO specialties VALUES (NULL, '%s');", specialties[i].name);
    if (mysql_query(connexion, request)) finish_with_error(connexion);
  }

  for (int i = 0; i < nbDoctors; i++) {
    sprintf(request, "INSERT INTO doctors VALUES (NULL, %d, '%s', '%s');",
            doctors[i].specialty_id, doctors[i].last_name, doctors[i].first_name);
    if (mysql_query(connexion, request)) finish_with_error(connexion);
  }

  for (int i = 0; i < nbPatients; i++) {
    sprintf(request, "INSERT INTO patients VALUES (NULL, '%s', '%s', '%s');",
            patients[i].last_name, patients[i].first_name, patients[i].birth_date);
    if (mysql_query(connexion, request)) finish_with_error(connexion);
  }

  for (int i = 0; i < nbConsultations; i++) {
    if (consultations[i].patient_id == -1) {
      sprintf(request, "INSERT INTO consultations (doctor_id, patient_id, date, hour, reason) "
                       "VALUES (%d, NULL, '%s', '%s', '%s');",
              consultations[i].doctor_id, consultations[i].date, consultations[i].hour, consultations[i].reason);
    } else {
      sprintf(request, "INSERT INTO consultations (doctor_id, patient_id, date, hour, reason) "
                       "VALUES (%d, %d, '%s', '%s', '%s');",
              consultations[i].doctor_id, consultations[i].patient_id, consultations[i].date,
              consultations[i].hour, consultations[i].reason);
    }
    if (mysql_query(connexion, request)) finish_with_error(connexion);
  }

  // Insérer quelques reports liés aux premières consultations (en supposant auto-inc commence à 1)
  for (int i = 0; i < nbReports; i++) {
    sprintf(request, "INSERT INTO reports (consultation_id, description) VALUES (%d, '%s');", reports[i].consultation_id, reports[i].description);
    if (mysql_query(connexion, request)) finish_with_error(connexion);
  }

  mysql_close(connexion);
  return 0;
}

