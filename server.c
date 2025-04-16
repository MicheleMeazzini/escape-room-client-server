#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>

#define BUFFER_SIZE 1024
#define NUM_ROOMS 1
#define NUM_LOCATIONS 4
#define NUM_OBJECTS 2
#define MAX_CREDS_LEN 20
#define MAX_NAME_LEN 50
#define MAX_DESC_LEN 500
#define MAX_SOL_LEN 100
#define TIME 600

int NUM_USERS = 0;
int NUM_SESSIONS = 0;
int utenti_online = 0;

// Struttura per gli utenti registrati
struct User {
    int id;
    bool online;
    char username[MAX_CREDS_LEN];
    char password[MAX_CREDS_LEN];

    struct User* next;
};
struct User* user_head = NULL;

struct Enigma {
    char nome[MAX_NAME_LEN];
    char desc[MAX_DESC_LEN];
    char sol[MAX_SOL_LEN];
};

struct Object {
    char nome[MAX_NAME_LEN];
    char desc[MAX_DESC_LEN];
    char use_desc_p1[MAX_DESC_LEN];
    char use_desc_p2[MAX_DESC_LEN];
    char use2_obj[MAX_NAME_LEN];
    int owner; //0 non preso, 1 preso da Player 1, 2 preso da Player 2, 3 preso da entrambi
    int token;
    int usato; //0 non usato, 1 usato da Player 1, 2 usato da Player 2, 3 usato da entrambi
    bool bloccato; //Serve per avere un filo logico nel gameplay
};

struct Location {
    char nome[MAX_NAME_LEN];
    char desc[MAX_DESC_LEN];
    int looking; //0 nessuno sta guardando, 1 Player 1 sta guardando, 2... , 3 entrambi
    struct Enigma enigma;
    struct Object objects[NUM_OBJECTS];
};

struct Room {
    int id;
    char nome[MAX_NAME_LEN];
    char desc[MAX_DESC_LEN];
    struct Location locations[NUM_LOCATIONS];
};

// Struttura per le sessioni attive
struct Session {
    int id;
    int socket_p1;
    int socket_p2;
    int tokens;
    time_t start_time;
    time_t end_time;
    int room_num;
    struct Room room;

    struct Session* next;
};
struct Session* session_head = NULL;

struct Session* find_session_by_socket(int socket){
    struct Session* current = session_head;

    while (current != NULL) {
        if (current->socket_p1 == socket || current->socket_p2 == socket) {
            return current; 
        }
        current = current->next;
    }

    return NULL;
}

void end_handler(int socket);

int find_player_role_by_socket(int socket){
    struct Session* current = find_session_by_socket(socket);
    if(current->socket_p1 == socket)
        return 1;
    else if(current->socket_p2 == socket)
        return 2;
    else    
        return 0;

    return 0;
}

void init_room(struct Room* room, int room_num) {
    if(room_num == 1){
        room->id = 1;
        strcpy(room->nome, "Cucina");
        strcpy(room->desc, "In Cucina è presente un ++tavolo++ accanto al ++frigorifero++, poco più avanti si trova la ++dispensa++ proprio accanto alla ++cassaforte++\n");

        struct Enigma enigma_vuoto;
        strcpy(enigma_vuoto.nome, "");   
        strcpy(enigma_vuoto.desc, "");   
        strcpy(enigma_vuoto.sol, "");    

        strcpy(room->locations[0].nome, "tavolo");
        strcpy(room->locations[0].desc, "Un tavolo di legno robusto, sopra di esso si trova uno **straccio** e una vecchia **foto**.\n");
        room->locations[0].looking = 0;
        room->locations[0].enigma = enigma_vuoto;

        strcpy(room->locations[0].objects[0].nome, "straccio");
        strcpy(room->locations[0].objects[0].desc, "Un vecchio straccio usurato.\n");
        strcpy(room->locations[0].objects[0].use_desc_p1, "Hai ottenuto uno straccio bagnato per respirare meglio!\n");
        strcpy(room->locations[0].objects[0].use_desc_p2, "Hai ottenuto uno straccio bagnato per respirare meglio!\n");
        strcpy(room->locations[0].objects[0].use2_obj, "bottiglia\0");
        room->locations[0].objects[0].owner = 0;
        room->locations[0].objects[0].token = 1;
        room->locations[0].objects[0].bloccato = false;
        room->locations[0].objects[0].usato = false;

        strcpy(room->locations[0].objects[1].nome, "foto");
        strcpy(room->locations[0].objects[1].desc, "La foto raffigura il matrimonio dei nostri genitori\n");
        strcpy(room->locations[0].objects[1].use_desc_p1, "Era un bellissimo venerdì 14 Luglio...\n");
        strcpy(room->locations[0].objects[1].use_desc_p2, "E' un pò scolorita, del resto è del 1998...\n");
        strcpy(room->locations[0].objects[1].use2_obj, "");
        room->locations[0].objects[1].owner = 0;
        room->locations[0].objects[1].token = 0;
        room->locations[0].objects[1].bloccato = true;
        room->locations[0].objects[1].usato = false;

        strcpy(room->locations[1].nome, "dispensa");
        strcpy(room->locations[1].desc, "Una dispensa piena di **piatti**, si intravede una **chiave** al suo interno.\n");
        room->locations[1].looking = 0;
        room->locations[1].enigma = enigma_vuoto;

        strcpy(room->locations[1].objects[0].nome, "chiave");
        strcpy(room->locations[1].objects[0].desc, "La chiave della porta, possiamo usarla per scappare.\n");
        strcpy(room->locations[1].objects[0].use_desc_p1, "La porta non si apre! La serratura è bloccata, dobbiamo romperla in qualche modo.\n");
        strcpy(room->locations[1].objects[0].use_desc_p2, "La porta non si apre! La serratura è bloccata, dobbiamo romperla in qualche modo.\n");
        strcpy(room->locations[1].objects[0].use2_obj, "");
        room->locations[1].objects[0].owner = 0;
        room->locations[1].objects[0].token = 2;
        room->locations[1].objects[0].bloccato = true;
        room->locations[1].objects[0].usato = false;

        strcpy(room->locations[1].objects[1].nome, "piatti");
        strcpy(room->locations[1].objects[1].desc, "Un set di piatti di porcellana.\n");
        strcpy(room->locations[1].objects[1].use_desc_p1, "A cosa possono servirci dei piatti? Dobbiamo scappare!\n");
        strcpy(room->locations[1].objects[1].use_desc_p2, "A cosa possono servirci dei piatti? Dobbiamo scappare!\n");
        strcpy(room->locations[1].objects[1].use2_obj, "");
        room->locations[1].objects[1].owner = 0;
        room->locations[1].objects[1].token = 0;
        room->locations[1].objects[1].bloccato = false;
        room->locations[1].objects[1].usato = false;

        struct Enigma enigma_cassaforte;
        strcpy(enigma_cassaforte.nome, "cassaforte");
        strcpy(enigma_cassaforte.desc, "La cassaforte è bloccata da un codice! Forse è una data importante per i miei genitori.\nInserisci codice:\n");
        strcpy(enigma_cassaforte.sol, "140798");

        strcpy(room->locations[2].nome, "cassaforte");
        strcpy(room->locations[2].desc, "Nella cassaforte c'è una vecchia **pistola** e un mucchio di **soldi**\n");
        room->locations[2].looking = 0;
        room->locations[2].enigma = enigma_cassaforte;

        strcpy(room->locations[2].objects[0].nome, "pistola");
        strcpy(room->locations[2].objects[0].desc, "Una pistola pesante con qualche graffio.\n");
        strcpy(room->locations[2].objects[0].use_desc_p1, "Lo sparo ha rotto la serratura, siamo salvi!\n");
        strcpy(room->locations[2].objects[0].use_desc_p2, "Lo sparo ha rotto la serratura, siamo salvi!\n");
        strcpy(room->locations[2].objects[0].use2_obj, "");
        room->locations[2].objects[0].owner = 0;
        room->locations[2].objects[0].token = 2;
        room->locations[2].objects[0].bloccato = true;
        room->locations[2].objects[0].usato = false;

        strcpy(room->locations[2].objects[1].nome, "soldi");
        strcpy(room->locations[2].objects[1].desc, "E' un bel mucchio di soldi.\n");
        strcpy(room->locations[2].objects[1].use_desc_p1, "Non ci servono adesso, dobbiamo scappare!.\n");
        strcpy(room->locations[2].objects[1].use_desc_p2, "Non ci servono adesso, dobbiamo scappare!.\n");
        strcpy(room->locations[2].objects[1].use2_obj, "");
        room->locations[2].objects[1].owner = 0;
        room->locations[2].objects[1].token = 0;
        room->locations[2].objects[1].bloccato = true;
        room->locations[2].objects[1].usato = false;

        strcpy(room->locations[3].nome, "frigorifero");
        strcpy(room->locations[3].desc, "Un frigorifero bianco e silenzioso, con una **bottiglia** e un mucchio di **verdure** al suo interno.\n");
        room->locations[3].looking = 0;
        room->locations[3].enigma = enigma_vuoto;

        strcpy(room->locations[3].objects[0].nome, "bottiglia");
        strcpy(room->locations[3].objects[0].desc, "Una bottiglia d'acqua ancora piena.\n");
        strcpy(room->locations[3].objects[0].use_desc_p1, "Hai ottenuto uno straccio bagnato per respirare meglio!\n");
        strcpy(room->locations[3].objects[0].use_desc_p2, "Hai ottenuto uno straccio bagnato per respirare meglio!\n");
        strcpy(room->locations[3].objects[0].use2_obj, "straccio");
        room->locations[3].objects[0].owner = 0;
        room->locations[3].objects[0].token = 1;
        room->locations[3].objects[0].bloccato = false;
        room->locations[3].objects[0].usato = false;

        strcpy(room->locations[3].objects[1].nome, "verdure");
        strcpy(room->locations[3].objects[1].desc, "Zucchine, spinaci e molte altre verdure.\n");
        strcpy(room->locations[3].objects[1].use_desc_p1, "Non c'è tempo di mangiare adesso, dobbiamo scappare!\n");
        strcpy(room->locations[3].objects[1].use_desc_p2, "Non c'è tempo di mangiare adesso, dobbiamo scappare!\n");
        strcpy(room->locations[3].objects[1].use2_obj, "");
        room->locations[3].objects[1].owner = 0;
        room->locations[3].objects[1].token = 0;
        room->locations[3].objects[1].bloccato = false;
        room->locations[3].objects[1].usato = false;
    } else {
        printf("Stanza non esistente\n");
    }
}

int check_credentials(const char* username, const char* password) {
    struct User* current = user_head;

    while (current != NULL) {
        if (strcmp(current->username, username) == 0 && strcmp(current->password, password) == 0) {
            if(current->online)
                return -1;
            else {
                current->online = true;
                return current->id;
            }
        }
        current = current->next;
    }
    return 0;
}

int add_user(const char* username, const char* password, int socket) {
    struct User* new_user = (struct User*)malloc(sizeof(struct User));
    if (new_user == NULL) {
        perror("Errore nella malloc");
        exit(1);
    }

    NUM_USERS++;
    new_user->id = NUM_USERS;
    new_user->online = false;
    strcpy(new_user->username, username);
    strcpy(new_user->password, password);
    new_user->next = NULL;

    if (user_head == NULL) {
        user_head = new_user;
    } else {
        struct User* current = user_head;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = new_user;
    }
    return new_user->id;
}

struct Session* create_session(int socket_p1, int room){
    struct Session* new_session = (struct Session*)malloc(sizeof(struct Session));
    if (new_session == NULL) {
        perror("Errore nella malloc");
        exit(1);
    }

    new_session->id = NUM_SESSIONS;
    new_session->socket_p1 = socket_p1;
    new_session->socket_p2 = -1;
    new_session->tokens = -1;
    new_session->room_num = room;
    new_session->next = NULL;

    init_room(&new_session->room, room);

    if (session_head == NULL) {
        session_head = new_session;
    } else {
        struct Session* current = session_head;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = new_session;
    }

    NUM_SESSIONS++;
    return new_session;
}
 
void send_remaining_time(int socket){
    struct Session* current = find_session_by_socket(socket);
    if(current == NULL){
        return;
    }
    time_t current_time = time(NULL);  // Ottieni il tempo corrente
    double seconds_remaining = difftime(current->end_time, current_time);
    char mex[BUFFER_SIZE];
    int socket_p1 = current->socket_p1;
    int socket_p2 = current->socket_p2;

    if (current->socket_p2 == -1)
        return;

    if (seconds_remaining > 0) {
        int minutes = (int)seconds_remaining / 60;
        int seconds = (int)seconds_remaining % 60;
        sprintf(mex, "Tempo rimanente: %d minuti e %d secondi - Tokens raccolti: %d - Tokens rimasti: %d\n", minutes, seconds, current->tokens, 8 - current->tokens);
        send(socket, mex, BUFFER_SIZE, 0);
    } else {
        sprintf(mex, "Tempo esaurito, le fiamme non vi hanno lasciato via di scampo.\nSCONFITTA\n");
        send(socket_p1, mex, BUFFER_SIZE, 0);
        send(socket_p2, mex, BUFFER_SIZE, 0);
        end_handler(socket);
    }
}

// Funzione per aggiungere un player2 a una sessione esistente
void join_session(struct Session* session, int socket_p2) {
    if (session->socket_p2 == -1) {
        session->socket_p2 = socket_p2;
        session->tokens = 0;
        session->start_time = time(NULL);
        session->end_time = session->start_time + TIME;
        printf("Sessione iniziata: %s", ctime(&(session->start_time)));
        printf("Sessione terminerà alle: %s", ctime(&(session->end_time)));
        printf("Player2 con socket %d si è unito alla sessione %d\n", socket_p2, session->id);
    } else {
        printf("Errore: la sessione è già piena!\n");
    }
}

int search_session(int socket, int room){
    struct Session* current = session_head;

    // Se la lista è vuota, crea una nuova sessione
    if (current == NULL) {
        printf("Lista delle sessioni vuota. Creazione di una nuova sessione...\n");
        session_head = create_session(socket, room);
        return 0;
    }

    // Cerca una sessione con un player2_id == -1
    struct Session* last = NULL;
    while (current != NULL) {
        printf("Sessione corrente: %d\n", current->id);
        printf("Socket p1: %d\n", current->socket_p1);
        printf("Socket p2: %d\n", current->socket_p2);
        printf("Room number: %d\n", current->room_num);
        // Se trovi una sessione con player1 già presente e player2 == -1
        if (current->socket_p1 > 0 && current->socket_p2 == -1 && current->room_num == room) {
            join_session(current, socket);
            return current->socket_p1;
        }

    last = current;  // Memorizza l'ultimo nodo della lista
        current = current->next;
    }

    // Se nessuna sessione ha trovato posto per player2, crea una nuova sessione
    printf("Creazione di una nuova sessione...\n\n");
    last->next = create_session(socket, room);
    return 0;
}

void end_handler(int socket){
    struct Session* to_delete = find_session_by_socket(socket);
    if(to_delete == NULL){
        send(socket, "Non puoi usare questo comando adesso.\n", BUFFER_SIZE, 0);
        return;
    }
    int role = find_player_role_by_socket(socket);
    int socket_p1 = to_delete->socket_p1;
    int socket_p2 = to_delete->socket_p2;   
    char mex[BUFFER_SIZE];
    sprintf(mex, "Player %d ha terminato la partita.\n", role);

    send(socket_p1, mex, BUFFER_SIZE, 0);
    if(socket_p2 > 0)
        send(socket_p2, mex, BUFFER_SIZE, 0);
    printf(mex);

    if (session_head == NULL || to_delete == NULL) return;

    // Se la sessione da eliminare è la testa della lista
    if (session_head == to_delete) {
        session_head = to_delete->next; // Sposta la testa al prossimo elemento
    } else {
        // Cerca la sessione precedente a quella da eliminare
        struct Session* current = session_head;
        while (current->next != NULL && current->next != to_delete) {
            current = current->next;
        }
    // Se la sessione da eliminare è stata trovata nella lista
        if (current->next == to_delete) {
            current->next = to_delete->next; // Salta l'elemento da eliminare
        }
    }

    // Dealloca la sessione
    free(to_delete);
}

bool register_handler(int sd) {
    char username[MAX_CREDS_LEN], password[MAX_CREDS_LEN];
    int nbytes, new_id;

    // Richiedi l'username al client
    send(sd, "Inserisci username: ", BUFFER_SIZE, 0);
    nbytes = recv(sd, username, sizeof(username), 0);
    if (nbytes <= 0) {
        printf("No username recieved");
        return false; // Errore o disconnessione
    }
    username[nbytes] = '\0'; // Rimuove il newline
    printf("Username: %s\n", username);

    // Richiedi la password al client
    send(sd, "Inserisci password: ", BUFFER_SIZE, 0);
    nbytes = recv(sd, password, sizeof(password), 0);
    if (nbytes <= 0) {
        printf("No password recieved");
        return false; // Errore o disconnessione
    }
    password[nbytes] = '\0'; // Rimuove il newline
    printf("Password: %s\n", password);

    // Inserisco utente
    new_id = add_user(username, password, sd);
    if (new_id > 0) {
        printf("Utente registrato\n");
        send(sd, "Register success", BUFFER_SIZE, 0);
        return true;
    } else {
        send(sd, "Register fail", BUFFER_SIZE, 0);
        return false;
    }
}

int login_handler(int sd) {
    char username[MAX_CREDS_LEN], password[MAX_CREDS_LEN];
    int nbytes, id, i;
    char mex[BUFFER_SIZE];

    for(i = 5; i > 0; i--){
        // Richiedi l'username al client
        nbytes = recv(sd, username, sizeof(username), 0);
        if (nbytes <= 0) {
            printf("No username recieved\n");
            return false; // Errore o disconnessione
        }
        username[nbytes] = '\0'; // Rimuove il newline
        printf("Username: %s\n", username);

        // Richiedi la password al client
        send(sd, "Inserisci password: ", BUFFER_SIZE, 0);
        nbytes = recv(sd, password, sizeof(password), 0);
        if (nbytes <= 0) {
            printf("No password recieved");
            return false; // Errore o disconnessione
        }
        password[nbytes] = '\0'; // Rimuove il newline
        printf("Password: %s\n", password);

        // Verifica le credenziali
        id = check_credentials(username, password);
        printf("ID utente: %d\n", id);
        if (id > 0) {
            send(sd, "Login success", BUFFER_SIZE, 0);
            utenti_online++;
            printf("Utenti online: %d\n", utenti_online);
            return id;
        } else if (id == -1) {
            sprintf(mex, "Utente già loggato, hai altri %d tentativi.\n\n", i-1);
            send(sd, mex, BUFFER_SIZE, 0);
        } else {
            sprintf(mex, "Utente non trovato, hai altri %d tentativi.\n\n", i-1);
            send(sd, mex, BUFFER_SIZE, 0);
        }
    }
    return 0;
}

void start_room_handler(int socket, int room){
    struct Session* current = find_session_by_socket(socket); 
    if(current != NULL){
        return;
    }

    if(room > NUM_ROOMS || room <= 0){
        send(socket, "Stanza non esistente. Stanze disponibili:\n  1 - Cucina in fiamme\n", BUFFER_SIZE, 0);
        return;
    }

    int ret = search_session(socket, room);
    if(ret == 0){
        send(socket, "Sessione creata, sei il Player 1. \nIn attesa di Player 2...", BUFFER_SIZE, 0);
        
    } else {
        char mex[BUFFER_SIZE];
        sprintf(mex, "Trovata sessione del socket: %d, sei il Player 2, avvio la partita...\n\nSei in cucina con Player 1 quando scoppia un incendio e le fiamme vi avvolgono rapidamente, trovate un modo per respirare nel fumo e uscite in fretta!\n", ret);
        send(ret, "Player 2 connesso, avvio la partita...\n\nSei in cucina con Player 2 quando scoppia un incendio e le fiamme vi avvolgono rapidamente, trovate un modo per respirare nel fumo e uscite in fretta!\n", BUFFER_SIZE, 0);
        send(socket, mex, BUFFER_SIZE, 0);
        send_remaining_time(ret);
    }
}

void gameplay_handler(int socket){
    struct Session* current = find_session_by_socket(socket);  
    int role = find_player_role_by_socket(socket);
    int other_socket = (role == 1) ? current->socket_p2 : current->socket_p1;
    int room = current->room_num;
    int tokens = current->tokens;

    if(room == 1){
        if(tokens == 0){
            send(socket, "...dobbiamo trovare un modo per respirare!\n", BUFFER_SIZE, 0);
        }
        if(tokens == 2){
            int usato = current->room.locations[0].objects[0].usato; // chi ha usato lo straccio
            if(role == usato)
                send(socket, "...forse dovrei dire a mio fratello cosa usare per respirare meglio\n", BUFFER_SIZE, 0); 
            else 
                send(socket, "...devo trovare un modo per respirare!\n", BUFFER_SIZE, 0);    
        }
        if(tokens == 4){
            send(socket, "...cerchiamo la chiave e scappiamo in fretta!\n", BUFFER_SIZE, 0);  
            current->room.locations[1].objects[0].bloccato = false; // sblocca la chiave
        }  
        if(tokens == 6){
            send(socket, "...dobbiamo trovare un modo per rompere la serratura della porta!\n", BUFFER_SIZE, 0);
            current->room.locations[1].objects[0].usato = 3; // la chiave viene usata solo da un Player  
            current->room.locations[0].objects[1].bloccato = false; // sblocca la foto
            current->room.locations[2].objects[0].bloccato = false; // sblocca la pistola
            current->room.locations[2].objects[1].bloccato = false; // sblocca i soldi
        }
        if(tokens == 8){
            send(socket, "...siamo salvi!\nVITTORIA\n", BUFFER_SIZE, 0);
            send(other_socket, "Il colpo di pistola ha rotto la serratura, siamo salvi!\nVITTORIA\n", BUFFER_SIZE, 0);
            printf("Sessione: %d, VITTORIA\n", current->id);
            end_handler(socket);
        }
    }
    return;
}

bool enigma_handler(int socket, struct Enigma enigma){
    char buf[BUFFER_SIZE];

    printf("Gestisco enigma %s: %s\n", enigma.nome, enigma.sol);

    send(socket, enigma.desc, BUFFER_SIZE, 0);
    recv(socket, buf, BUFFER_SIZE, 0);
    printf("Ricevuto: %s\n", buf);
    if(strcmp(buf, enigma.sol) == 0){
        printf("Codice corretto, location sbloccata.\n");
        send(socket, "Codice corretto, ora puoi prendere gli oggetti!", BUFFER_SIZE, 0);
        return true;
    }
    printf("Codice sbagliato, location ancora bloccata.\n");
    send(socket, "Codice sbagliato.\n", BUFFER_SIZE, 0);
    return false;
}

void look_loc_obj_handler(int socket, const char* loc_obj){
    struct Session* current = find_session_by_socket(socket);
    if(current == NULL || current->tokens < 0){ // se inverto il controllo ottengo segmentation fault
        send(socket, "Non puoi usare questo comando adesso.\n", BUFFER_SIZE, 0);
        return;
    }
    int role = find_player_role_by_socket(socket);
    int i, j;

    for(i = 0; i < NUM_LOCATIONS; i++){
        if(strcmp(current->room.locations[i].nome, loc_obj) == 0){
            printf("Trovata la location: %s\n", loc_obj);
            if(strcmp(current->room.locations[i].enigma.nome, "") != 0){
                printf("Trovato enigma per la location: %s\n", loc_obj);

                if(!enigma_handler(socket, current->room.locations[i].enigma))
                return;
            }
            if(current->room.locations[i].looking == 0)
                current->room.locations[i].looking = role;
            else if(current->room.locations[i].looking > 0 && current->room.locations[i].looking != role)
                current->room.locations[i].looking = 3;
            send(socket, current->room.locations[i].desc, BUFFER_SIZE, 0);
            return;
        }
        //Se il player sta guardando dentro alla location cerco anche tra i suoi oggetti
        if(current->room.locations[i].looking == role || current->room.locations[i].looking == 3){
            for(j = 0; j < NUM_OBJECTS; j++){
                if(strcmp(current->room.locations[i].objects[j].nome, loc_obj) == 0){
                    printf("Trovato l'oggetto: %s\n", loc_obj);
                    send(socket, current->room.locations[i].objects[j].desc, BUFFER_SIZE, 0);
                    return;
                }
            }
        }
    }
    printf("Location/oggetto non trovato: %s\n", loc_obj);
    send(socket, "Location/Oggetto non trovato, se cerchi un oggetto assicurati di star guardando la location in cui si trova.\n", BUFFER_SIZE, 0);
}

void look_handler(int socket){
    struct Session* current = find_session_by_socket(socket);
    if(current == NULL || current->tokens < 0){
        send(socket, "Non puoi usare questo comando adesso.\n", BUFFER_SIZE, 0);
        return;
    }
    struct Room room = current->room;

    send(socket, room.desc, BUFFER_SIZE, 0);
}

void take_handler(int socket, const char* obj){
    struct Session* current = find_session_by_socket(socket);
    if(current == NULL){
        send(socket, "Non puoi usare questo comando adesso.\n", BUFFER_SIZE, 0);
        return;
    }
    int role = find_player_role_by_socket(socket);
    int i, j;

    for(i = 0; i < NUM_LOCATIONS; i++){
        //Se il player sta guardando dentro alla location cerco anche tra i suoi oggetti
        if(current->room.locations[i].looking == role || current->room.locations[i].looking == 3){
            for(j = 0; j < NUM_OBJECTS; j++){
                if(strcmp(current->room.locations[i].objects[j].nome, obj) == 0){
                    printf("Trovato l'oggetto: %s\n", obj);
                    if(current->room.locations[i].objects[j].owner == 0)
                        current->room.locations[i].objects[j].owner = role;
                    else if(current->room.locations[i].objects[j].owner == role || current->room.locations[i].objects[j].owner == 3){
                        send(socket, "Possiedi già questo oggetto.\n", BUFFER_SIZE, 0);
                        return;
                    }
                    else if(current->room.locations[i].objects[j].owner != role)
                        current->room.locations[i].objects[j].owner = 3;
                    char mex[] = "Hai preso l'oggetto ";
                    strcat(mex, obj);
                    strcat(mex, "\n");
                    send(socket, mex, BUFFER_SIZE, 0);
                    return;
                    
                }
            }
        }
    }
    printf("Oggetto %s non trovato\n", obj);
    send(socket, "Oggetto non trovato\n", BUFFER_SIZE, 0);
}

void objs_handler(int socket){
    struct Session* current = find_session_by_socket(socket);
    if(current == NULL || current->tokens < 0){
        send(socket, "Non puoi usare questo comando adesso.\n", BUFFER_SIZE, 0);
        return;
    }
    int role = find_player_role_by_socket(socket);
    int i, j;
    char mex[BUFFER_SIZE] = "Oggetti raccolti:\n";

    for(i = 0; i < NUM_LOCATIONS; i++){
        for(j = 0; j < NUM_OBJECTS; j++){
            printf("Oggetto: %s. Owner: %d. Usato: %d\n", current->room.locations[i].objects[j].nome, current->room.locations[i].objects[j].owner, current->room.locations[i].objects[j].usato);
            if(current->room.locations[i].objects[j].owner == role || current->room.locations[i].objects[j].owner == 3){
                int remaining_bytes = BUFFER_SIZE - strlen(mex);
                strncat(mex, current->room.locations[i].objects[j].nome, remaining_bytes);
                strncat(mex, "\n", remaining_bytes-2);
            }
        }
    }
    send(socket, mex, BUFFER_SIZE, 0);
}

void use_handler(int socket, const char* obj){
    struct Session* current = find_session_by_socket(socket);
    if(current == NULL || current->tokens < 0){
        send(socket, "Non puoi usare questo comando adesso.\n", BUFFER_SIZE, 0);
        return;
    }
    int role = find_player_role_by_socket(socket);
    int other = (role == 1) ? 2 : 1;
    int i, j;

    for(i = 0; i < NUM_LOCATIONS; i++){
        for(j = 0; j < NUM_OBJECTS; j++){
            if(strcmp(current->room.locations[i].objects[j].nome, obj) == 0 && (current->room.locations[i].objects[j].owner == role || current->room.locations[i].objects[j].owner == 3)){
                if (current->room.locations[i].objects[j].usato == role || current->room.locations[i].objects[j].usato == 3) {
                    send(socket, "Oggetto già utilizzato.\n", BUFFER_SIZE, 0);
                    printf("Player %d ha usato l'oggetto %s (già usato). Tokens totali: %d\n", role, obj, current->tokens);
                    gameplay_handler(socket);
                    return;
                }
                if (current->room.locations[i].objects[j].token == 1) {
                    send(socket, "Questo oggetto può essermi utile se lo combino con un altro.\n", BUFFER_SIZE, 0);
                    gameplay_handler(socket);
                    return;
                }
                if (current->room.locations[i].objects[j].bloccato == true) {
                    send(socket, "Non devo usare questo oggetto adesso.\n", BUFFER_SIZE, 0);
                    gameplay_handler(socket);
                    return;
                }
                if(role == 1)
                    send(socket, current->room.locations[i].objects[j].use_desc_p1, BUFFER_SIZE, 0);
                else    
                    send(socket, current->room.locations[i].objects[j].use_desc_p2, BUFFER_SIZE, 0);
                if(current->room.locations[i].objects[j].usato == 0 || current->room.locations[i].objects[j].usato == other){
                    current->tokens += current->room.locations[i].objects[j].token;
                    if(current->room.locations[i].objects[j].usato == 0)
                        current->room.locations[i].objects[j].usato = role;
                    else
                        current->room.locations[i].objects[j].usato = 3;
                    printf("Player %d ha usato l'oggetto %s. Tokens totali: %d\n", role, obj, current->tokens);
                }   
                gameplay_handler(socket);
                return;
            }
        }
    }
    send(socket, "Oggetto non posseduto o inesistente, >objs per visualizzare gli oggetti che possiedi.\n", BUFFER_SIZE, 0);
    gameplay_handler(socket);
}

void use_2_handler(int socket, const char* obj1, const char* obj2){
    struct Session* current = find_session_by_socket(socket);
    if(current == NULL || current->tokens < 0){
        send(socket, "Non puoi usare questo comando adesso.\n", BUFFER_SIZE, 0);
        return;
    }
    int role = find_player_role_by_socket(socket);
    int other = (role == 1) ? 2 : 1;
    int i, j;
    int loc_obj1, idx_obj1;
    char _obj2[MAX_NAME_LEN];

    for(i = 0; i < NUM_LOCATIONS; i++){
        for(j = 0; j < NUM_OBJECTS; j++){
            if(strcmp(current->room.locations[i].objects[j].nome, obj1) == 0 && (current->room.locations[i].objects[j].owner == role || current->room.locations[i].objects[j].owner == 3)){
                if(current->room.locations[i].objects[j].usato == role || current->room.locations[i].objects[j].usato == 3){
                    send(socket, "Oggetto 1 già utilizzato.\n", BUFFER_SIZE, 0);
                    gameplay_handler(socket);
                    return;
                }
                strcpy(_obj2, current->room.locations[i].objects[j].use2_obj);
                loc_obj1 = i;
                idx_obj1 = j;
            }
        }
    }
    if(strcmp(obj2, _obj2) != 0){
        send(socket, "Uno o più oggetti sconosciuti o non combinabili.\n", BUFFER_SIZE, 0);
        gameplay_handler(socket);
        return;
    }

    for(i = 0; i < NUM_LOCATIONS; i++){
        for(j = 0; j < NUM_OBJECTS; j++){
            if(strcmp(current->room.locations[i].objects[j].nome, obj2) == 0 && (current->room.locations[i].objects[j].owner == role || current->room.locations[i].objects[j].owner == 3)){
                if(current->room.locations[i].objects[j].usato == role || current->room.locations[i].objects[j].usato == 3){
                    send(socket, "Oggetto 2 già utilizzato.\n", BUFFER_SIZE, 0);
                    gameplay_handler(socket);
                    return;
                }
                if(role == 1)
                    send(socket, current->room.locations[i].objects[j].use_desc_p1, BUFFER_SIZE, 0);
                else    
                    send(socket, current->room.locations[i].objects[j].use_desc_p2, BUFFER_SIZE, 0);

                current->tokens += current->room.locations[i].objects[j].token;
                current->tokens += current->room.locations[loc_obj1].objects[idx_obj1].token;

                if (current->room.locations[i].objects[j].usato == 0)
                    current->room.locations[i].objects[j].usato = role;
                else if (current->room.locations[i].objects[j].usato == other)
                    current->room.locations[i].objects[j].usato = 3;

                if (current->room.locations[loc_obj1].objects[idx_obj1].usato == 0)
                    current->room.locations[loc_obj1].objects[idx_obj1].usato = role;
                else if (current->room.locations[loc_obj1].objects[idx_obj1].usato == other)
                    current->room.locations[loc_obj1].objects[idx_obj1].usato = 3;

                printf("Player %d ha usato l'oggetto %s insieme a %s. Tokens totali: %d\n", role, obj1, obj2, current->tokens);
                gameplay_handler(socket);
                return;
            }
        }
    }
}

void say_handler(int socket, const char* mex){
    struct Session* current = find_session_by_socket(socket);
    if(current == NULL || current->tokens < 0){
        send(socket, "Non puoi usare questo comando adesso.\n", BUFFER_SIZE, 0);
        return;
    }
    int role = find_player_role_by_socket(socket);
    int dest_socket = (role == 1) ? current->socket_p2 : current->socket_p1;
    char buf[BUFFER_SIZE];

    sprintf(buf, "Player %d dice: %s\n", role, mex);

    send(dest_socket, buf, BUFFER_SIZE, 0);
    send(socket, "Messaggio inviato.\n", BUFFER_SIZE, 0);
}

void gestione_comandi(int socket, const char* comando){
    int room;          
    char loc_obj[MAX_NAME_LEN]; 
    char obj1[MAX_NAME_LEN];      
    char obj2[MAX_NAME_LEN];  
    char mex[BUFFER_SIZE];  

    // Controllo del comando "start room"
    if (strncmp(comando, "start", 5) == 0) {
        if (sscanf(comando, "start %d", &room) == 1) {
            printf("Comando 'start' ricevuto. Stanza: %d\n", room);
            start_room_handler(socket, room);
        } else {
            printf("Errore: il comando 'start' non è corretto.\n");
            send(socket, "Indicare il numero della stanza (es. start 1)\n", BUFFER_SIZE, 0);
        }
    }
    // Controllo del comando "end"
    else if (strncmp(comando, "end", 3) == 0) {
        printf("Comando 'end' ricevuto.\n");
        end_handler(socket);
    }
    // Controllo del comando "look location"
    else if (strncmp(comando, "look", 4) == 0) {
        if (sscanf(comando, "look %s", loc_obj) == 1) {
            printf("Comando 'look' ricevuto. Location/Oggetto: %s\n", loc_obj);
            look_loc_obj_handler(socket, loc_obj);
        } else {
            printf("Comando 'look' ricevuto.\n");
            look_handler(socket);
        }
    }
    // Controllo del comando "objs"
    else if (strncmp(comando, "objs", 4) == 0) {
        printf("Comando 'objs' ricevuto.\n");
        objs_handler(socket);
    }
    // Controllo del comando "take obj"
    else if (strncmp(comando, "take", 4) == 0) {
        if (sscanf(comando, "take %s", obj1) == 1) {
            printf("Comando 'take' ricevuto. Oggetto: %s\n", obj1);
            take_handler(socket, obj1);
        } else {
            printf("Errore: il comando 'take' non è corretto.\n");
            send(socket, "Indicare l'oggetto che si vuole prendere (es. take straccio)\n", BUFFER_SIZE, 0);
        }
    }
    else if (strncmp(comando, "use", 3) == 0) {
        // Proviamo a estrarre due oggetti, se disponibili
        if (sscanf(comando, "use %s %s", obj1, obj2) == 2) {
            printf("Comando 'use' ricevuto con due oggetti: %s e %s\n", obj1, obj2);
            use_2_handler(socket, obj1, obj2);
        }
        // Se solo un oggetto è fornito
        else if (sscanf(comando, "use %s", obj1) == 1) {
            printf("Comando 'use' ricevuto con un oggetto: %s\n", obj1);
            use_handler(socket, obj1);
        } else {
            printf("Errore: il comando 'use' non è corretto.\n");
            send(socket, "Indicare l'oggetto (o gli oggetti) che si vuole utilizzare (es. use foto)\n", BUFFER_SIZE, 0);
        }
    }
    // Controllo del comando "say mex"
    else if (strncmp(comando, "say", 3) == 0) {
        if (sscanf(comando, "say %[^\n]", mex) == 1) {
            printf("Comando 'say' ricevuto. Messaggio: %s\n", mex);
            say_handler(socket, mex);
        } else {
            printf("Errore: il comando 'say' non è corretto.\n");
            send(socket, "Indicare il messaggio da inviare (es. say ciao)\n", BUFFER_SIZE, 0);
        }
    }
    // Comando non riconosciuto
    else {
        send(socket, "Comando non riconosciuto, consulta la lista dei comandi.\n", BUFFER_SIZE, 0);
        printf("Comando non riconosciuto.\n");
    }
    send_remaining_time(socket);

    return;
}

int main(int argc, char* argv[]) {
    fd_set master, read_fds;
    int fdmax, listener, newfd, nbytes, addrlen, i, ret, j;
    struct sockaddr_in sv_addr, cl_addr;
    uint16_t porta;
    char buf[BUFFER_SIZE];
    char comando[6];

    FD_ZERO(&master);
    FD_ZERO(&read_fds);
    printf("\n************************** SERVER **************************\n");
    printf("Seleziona un comando:\n\n");
    printf("1)  start <port> --> Avvia il server sulla porta\n");
    printf("2)  stop --> Ferma il server\n");
    printf("***************************************************************\n\n");
    fflush(stdout);

    FD_SET(STDIN_FILENO, &master);

    for(;;){
        read_fds = master;
        if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1) {
            perror("Errore nella select");
            exit(1);
        }
        for(i = 0; i <= fdmax; i++){
            if(FD_ISSET(i, &read_fds)){
                if(i == STDIN_FILENO){
                    fgets(buf, sizeof(buf), stdin);

                    // So di aspettarmi un comando di max 5 caratteri e un intero su 16 bit
                    ret = sscanf(buf, "%5s %hu", comando, &porta);
                    
                    if (ret == 2 && !strcmp(comando, "start")) {
                        // start <port>, inserisco nell'fd_set il descrittore per un nuovo server
                        
                        printf("Avvio del server di gioco...\n");

                        listener = socket(AF_INET, SOCK_STREAM, 0);
                        if (listener < 0) {
                            perror("Errore nella socket\n");
                            exit(1);
                        }
                        setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));

                        // Configurazione dell'indirizzo del server
                        sv_addr.sin_family = AF_INET;
                        sv_addr.sin_port = htons(porta);
                        sv_addr.sin_addr.s_addr = INADDR_ANY;

                        // Bind del socket
                        if (bind(listener, (struct sockaddr*)&sv_addr, sizeof(sv_addr)) < 0) {
                            perror("Errore nel bind\n");
                            exit(1);
                        }

                        // Ascolta le connessioni
                        if (listen(listener, 10) < 0) {
                            perror("Errore nella listen\n");
                            exit(1);
                        }

                        FD_SET(listener, &master);

                        if(listener > fdmax){
                            fdmax = listener; // fd_max contiene ora il valore massimo dei descrittori di socket master
                        }


                    } else if (ret == 1 && !strcmp(comando, "stop")){
                        if(session_head == NULL){
                            for(j = 1; j <= fdmax; j++){
                                if(FD_ISSET(j, &master)){
                                    FD_CLR(j, &master);
                                    close(j);
                                                    
                                    printf("Socket %d chiuso, continuo...\n", i);
                                }
                            }
                            printf("Arresto del server\n");
                            close(listener);
                        } else {
                            printf("Ci sono partite in corso, impossibile arrestare il server\n");
                        }
                    } else {
                        // Il comando inserito nella console non è supportato.
                        printf("Comando inesistente.\n");
                    }
                }
                else if(i == listener){
                    addrlen = sizeof(cl_addr);
                    newfd = accept(listener, (struct sockaddr*)&cl_addr, (socklen_t*)&addrlen);
                    if (newfd < 0) {
                        perror("Errore nella accept");
                    } else {
                        FD_SET(newfd, &master);
                        if(newfd > fdmax) {
                            fdmax = newfd;
                        }
                        printf("Nuova connessione sul socket %d\n", newfd);
                    }

                    printf("Utenti online: %d\n", utenti_online);

                    // Registra l'utente
                    if (!register_handler(newfd)) {
                        close(newfd);
                        FD_CLR(newfd, &master);
                    }
                    // Login utente
                    if (login_handler(newfd) < 0) {
                        close(newfd);
                        FD_CLR(newfd, &master);
                    }
                } else {
                    nbytes = recv(i, buf, sizeof(buf), 0);
                    if (nbytes <= 0) {
                        // Errore o connessione chiusa dal client
                        if (nbytes == 0) {
                            printf("\nSocket %d ha chiuso la connessione.\n", i);
                            end_handler(i);
                            utenti_online--;
                        } else {
                            perror("\nErrore nella recv");
                        }
                        close(i); // Chiudi il socket
                        FD_CLR(i, &master); // Rimuovi dal set master

                        if (i == fdmax) {
                        // Riduci fdmax al prossimo valore più alto in `master`
                        while (fdmax >= 0 && !FD_ISSET(fdmax, &master)) {
                            fdmax--;
                        }
                    }
                    } else {
                        // Dati ricevuti: gestisci i dati nel buffer 'buf'
                        buf[nbytes] = '\0'; 
                        printf("\nSocket %d: %s (%d bytes)\n", i, buf, nbytes);

                        gestione_comandi(i, buf);
                    }
                }
            }
           
        }
    }

    return 0;
}

