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

#define SERVER_IP "127.0.0.1"
#define PORT 4242
#define BUFFER_SIZE 1024
#define NUM_TENTATIVI 5

bool register_handler(int sd) {
    char username[20], password[20];
    char buffer[BUFFER_SIZE];
    int nbytes, i;
    printf("Benvenuto! Registrati con Username e Password.\n");

    // Ricezione richiesta di username
    nbytes = recv(sd, buffer, BUFFER_SIZE, 0);
    buffer[nbytes] = '\0';
    printf("%s", buffer);
   
    // Invio dello username
    for(i = NUM_TENTATIVI; i > 0; i--){
        if(fgets(username, sizeof(username), stdin) == NULL)
            printf("Errore in fgets()");
        username[strcspn(username, "\n")] = 0;
        if(strcmp(username, "") != 0){
            send(sd, username, strlen(username), 0);
            break;
        } else {
            if(i == 1){
                printf("Registrazione non riuscita, chiusura client.\n");
                exit(1);
            }
            printf("Username non valido, hai altri %d tentativi.\nInserisci username: ", i-1);
        }
    }

    // Ricezione richiesta di password
    nbytes = recv(sd, buffer, BUFFER_SIZE, 0);
    buffer[nbytes] = '\0';
    printf("%s", buffer);

    // Invio della password
    for(i = NUM_TENTATIVI; i > 0; i--){
        if(fgets(password, sizeof(password), stdin) == NULL)
            printf("Errore in fgets()");
        password[strcspn(password, "\n")] = 0;
        if(strcmp(password, "") != 0){
            send(sd, password, strlen(password), 0);
            break;
        } else {
            if(i == 1){
                printf("Registrazione non riuscita, chiusura client.\n");
                exit(1);
            }
            printf("Password non valida, hai altri %d tentativi.\nInserisci password: ", i-1);
        }
    }

    // Ricezione esito registrazione
    nbytes = recv(sd, buffer, BUFFER_SIZE, 0);
    buffer[nbytes] = '\0';

    // Esito registrazione
    if (strstr(buffer, "Register success") != NULL) {
        printf("Registrazione effettuata con successo\n\n");
        return 1; 
    } else {
        printf("Errore nella registrazione, chiusura del client\n");
        return 0; 
    }
}

bool login_handler(int sd) {
    char username[20], password[20];
    char buffer[BUFFER_SIZE];
    int nbytes, i;
    printf("Effettua il login.\n");
    printf("Inserisci username: ");

    // Invio dello username
    for(i = NUM_TENTATIVI; i > 0; i--){
        if(fgets(username, sizeof(username), stdin) == NULL)
            printf("Errore in fgets()");
        username[strcspn(username, "\n")] = 0;
        if(strcmp(username, "") != 0){
            send(sd, username, strlen(username), 0);
            break;
        } else {
            if(i == 1){
                printf("Login non riuscita, chiusura client.\n");
                exit(1);
            }
            printf("Username non valido, hai altri %d tentativi.\nInserisci username: ", i-1);
        }
    }

    // Ricezione richiesta di password
    nbytes = recv(sd, buffer, BUFFER_SIZE, 0);
    buffer[nbytes] = '\0';
    printf("%s", buffer);

    // Invio della password
    for(i = NUM_TENTATIVI; i > 0; i--){
        if(fgets(password, sizeof(password), stdin) == NULL)
            printf("Errore in fgets()");
        password[strcspn(password, "\n")] = 0;
        if(strcmp(password, "") != 0){
            send(sd, password, strlen(password), 0);
            break;
        } else {
            if(i == 1){
                printf("Login non riuscita, chiusura client.\n");
                exit(1);
            }
            printf("Password non valida, hai altri %d tentativi.\nInserisci password: ", i-1);
        }
    }

    // Ricezione esito login
    nbytes = recv(sd, buffer, BUFFER_SIZE, 0);
    buffer[nbytes] = '\0';

    // Esito login
    if (strstr(buffer, "Login success") != NULL) {
        printf("\n************************************ LISTA COMANDI ***************************************\n");
        printf("| > start <room> --> Inizia la partita nella stanza <room>.                              |\n");
        printf("| Stanze disponibili:                                                                    |\n");
        printf("|     1 - Cucina in fiamme                                                               |\n");
        printf("| > look --> Fornisce una descrizione dell'ambiente in cui ti trovi                      |\n");
        printf("| > look <loc | obj> --> Osserva meglio la location <loc> o l'oggetto <obj>              |\n");
        printf("| > take <obj> --> Prendi l'oggetto <obj>                                                |\n");
        printf("| > use <obj1> (<obj2>) --> Usa l'oggetto <obj1> in combinazione con <obj2> se presente  |\n");
        printf("| > objs --> Mostra l'elenco degli oggetti raccolti                                      |\n");
        printf("| > say <mess> --> Invia il messaggio <mess> all'altro giocatore                         |\n");
        printf("| > end --> Termina la partita per entrambi i giocatori                                  |\n");
        printf("******************************************************************************************\n\n");
        return true; 
    } else {
        printf("%s", buffer);
        return false;
    }
}

int main(int argc, char* argv[]){
    int fdmax, ret, i;
    fd_set  master, read_fds;
    char buffer[BUFFER_SIZE];
    struct sockaddr_in sv_addr;
    in_port_t porta = htons(PORT);

    /* Inizializzazione del client */
    printf("! Avvio e inizializzazione del client in corso...\n");
    
    fdmax = socket(AF_INET, SOCK_STREAM, 0);
    if(fdmax < 0) {
        perror("ERR: Errore creazione socket");
        exit(1);
    }

    memset(&sv_addr, 0, sizeof(sv_addr));
    sv_addr.sin_family = AF_INET;
    sv_addr.sin_port = porta;
    inet_pton(AF_INET, SERVER_IP, &sv_addr.sin_addr);

    for(i = 0; i < NUM_TENTATIVI; i++){
        printf("Tentativo di connessione col server...\n");
        ret = connect(fdmax, (struct sockaddr*)&sv_addr, sizeof(sv_addr));
        if(ret == 0) {
            break;
        }
        sleep(3);
        if(i == 4){
            printf("Server non trovato, chiusura del client.\n");
            close(fdmax);
            exit(1);
        }
    }
    
    printf("OK: Client inizializzato e connesso al server\n");
    
    /* Inizializzazione dei set di file descriptor */
    FD_ZERO(&master);
    FD_ZERO(&read_fds);

    FD_SET(fdmax, &master);        // Aggiunta del socket del server al set master
    FD_SET(STDIN_FILENO, &master); // Aggiunta di STDIN (tastiera) al set master

    // Ricevo il messaggio per capire se sono player 1 o player 2
    printf("Attendi altri clients...\n");

    // Registra l'utente
    if (!register_handler(fdmax)) {
        close(fdmax);
        FD_CLR(fdmax, &master);
    }

    for (i = 0; i < NUM_TENTATIVI; i++){
        // Login utente
        ret = login_handler(fdmax);
        if (ret){
            break;
        }
        if (!ret && i == 4) {
            printf("Tentativi esauriti, chiusura client.\n");
            close(fdmax);
            exit(1);
        }   
    }

    for(;;) {
        read_fds = master; // Copia del set master in read_fds

        /* Attesa degli eventi su socket o input da tastiera */
        ret = select(fdmax + 1, &read_fds, NULL, NULL, NULL);
        if (ret < 0) {
            perror("ERR: Errore nella select");
            exit(1);
        }

        /* Gestione degli eventi */
        for(i = 0; i <= fdmax; i++) {
            if(FD_ISSET(i, &read_fds)) {

                /* Controllo se è arrivato input da tastiera */
                if(i == STDIN_FILENO) {
                    memset(buffer, 0, BUFFER_SIZE);
                    fgets(buffer, BUFFER_SIZE, stdin);

                    /* Rimuovo il newline dall'input */
                    strtok(buffer, "\n");

                    /* Invia il messaggio al server */
                    ret = send(fdmax, buffer, BUFFER_SIZE, 0);
                    if(ret < 0){
                        perror("ERR: Errore durante l'invio del messaggio");
                        exit(1);
                    }
                }

                /* Controllo se ci sono messaggi dal server */
                else if(i == fdmax) {
                    memset(buffer, 0, BUFFER_SIZE);
                    ret = recv(fdmax, buffer, BUFFER_SIZE, 0);

                    if(ret == 0) {
                        printf("Connessione chiusa dal server\n");
                        exit(0); // Server ha chiuso la connessione
                    }

                    if(ret < 0) {
                        perror("ERR: Errore in ricezione del messaggio");
                        exit(1);
                    }

                    printf("%s\n", buffer);
                }
            }
        } 
    }

    return 0;
}
