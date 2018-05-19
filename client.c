#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <arpa/inet.h>
#include <unistd.h>

#define STDIN_FILENO 0
#define BUFLEN 256

void error(char *msg)
{
    perror(msg);
    exit(0);
}

int main(int argc, char *argv[])
{
    int sockfd, n;
    int fdmax, rc;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    int nr_card;
    int pin;
    int suma;
    int logare = 0;
    int log2 = 0;
    char flog[20];
    int pid = getpid();
    sprintf(flog, "client-%d.log", pid);
    char text[100000];
    sprintf(text, "Fisierul clientului %d\n", pid);

    fd_set read_fds;
    fd_set tmp_fds;

    char buffer[BUFLEN];
    if (argc < 3) {
       fprintf(stderr,"Usage %s server_address server_port\n", argv[0]);
       exit(0);
    }  

    FD_ZERO(&read_fds);
    FD_ZERO(&tmp_fds);
    
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");
    
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(atoi(argv[2]));
    inet_aton(argv[1], &serv_addr.sin_addr);
    
    
    if (connect(sockfd,(struct sockaddr*) &serv_addr,sizeof(serv_addr)) < 0) 
        error("ERROR connecting");    
    
    // Adaugam tastatura
    FD_SET(STDIN_FILENO, &read_fds);
    FD_SET(sockfd, &read_fds);

    fdmax = sockfd;

    while(1){
        tmp_fds = read_fds;
        rc = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
        //Parcurg socketi
        for (int i = 0; i <= fdmax; i++) {
            int valid = 0;
            if (FD_ISSET(i, &tmp_fds)) {
                //Citesc de la tastatura
                if (i == STDIN_FILENO){
                    char command[9];
                    char buff[100];
                    scanf("%s", command);
                    sprintf(buff, "%s", command);
                    if (strcmp(command, "login") == 0) {
                        valid = 1;
                        //Exista deja un utilizator logat
                        if (logare == 1) {
                            printf("IBANK> -2 : Sesiune deja deschisa\n");
                            strcat(text, "IBANK> -2 : Sesiune deja deschisa\n");
                        } else {
                            //Formez buffer pe care il trimit catre server
                            scanf("%d %d", &nr_card, &pin);
                            char Cnr_card[7];
                            char Cpin[5];
                            sprintf(Cnr_card, "%d", nr_card);
                            sprintf(Cpin, "%d", pin);
                            strcat(buff, " ");
                            strcat(buff, Cnr_card);
                            strcat(buff, " ");
                            strcat(buff, Cpin);
                            n = send(sockfd, buff, strlen(buff), 0);
                            strcat(text,buff);
                            strcat(text, "\n");
                        }
                    }
                    if (strcmp(command, "logout") == 0) {
                        valid = 1;
                        //Daca am un utilizator logat si trimit comanda de logout catre server
                        if (logare == 1) {
                            logare = 0;
                            strcat(text, "logout\n");
                            n = send(sockfd, buff, strlen(buff), 0);
                        } else {
                            printf("IBANK> -1 : Clientul nu este autentificat\n");
                            strcat(text, "IBANK> -1 : Clientul nu este autentificat\n");
                        }
                    }
                    if (strcmp(command, "listsold") == 0) {
                        valid = 1;
                        //Daca am un utilizator logat si trimit comanda de listsold catre server
                        if (logare == 1) {
                            printf("Sold\n");
                            strcat(text, "listsold\n");
                            n = send(sockfd, buff, strlen(buff), 0);
                        } else {
                            printf("IBANK> -1 : Clientul nu este autentificat\n");
                            strcat(text, "IBANK> -1 : Clientul nu este autentificat\n");
                        }
                    }
                    if (strcmp(command, "transfer") == 0) {
                        valid = 1;
                        //Daca am un utilizator logat si trimit comanda de transfer catre server
                        if (logare == 1) {
                            //Formez buffer pe care il trimit serverului
                            scanf("%d %d", &nr_card, &suma);
                            char Cnr_card[7];
                            char Csuma[18];
                            sprintf(Cnr_card, "%d", nr_card);
                            sprintf(Csuma, "%d", suma);
                            strcat(buff, " ");
                            strcat(buff, Cnr_card);
                            strcat(buff, " ");
                            strcat(buff, Csuma);
                            strcat(text,buff);
                            strcat(text, "\n");
                            n = send(sockfd, buff, strlen(buff), 0);
                        } else {
                            printf("IBANK> -1 : Clientul nu este autentificat\n");
                            strcat(text, "IBANK> -1 : Clientul nu este autentificat\n");
                        }
                    }
                    if (strcmp(command, "unlock") == 0) {
                        valid = 1;
                        strcat(text, "unlock\n");
                        printf("Unlock\n");
                    }
                    //La comanda quit scriu in fisier toate comenzile dupa care inchid clientul
                    if (strcmp(command, "quit") == 0) {
                        valid = 1;
                        strcat(text, "quit\n");
                        FILE *out = fopen(flog, "w");
                        fputs(text, out);
                        fclose(out);
                        n = send(sockfd, buff, strlen(buff), 0);
                        return 0;
                    }
                    //In cazul in care nu am primit o comanda valida
                    if (valid == 0) {
                        n = send(sockfd, buff, strlen(buff), 0);
                    }
                }
                //Primesc mesaj de la server
                if (i == sockfd){
                    char buff[100];
                    n = recv(sockfd, buff, sizeof(buff), 0);
                    printf("%s", buff);
                    //Daca primesc quit de la server
                    if (buff[0] == 'S') {
                        strcat(text, buff);
                        FILE *out = fopen(flog, "w");
                        fputs(text, out);
                        fclose(out);
                        return 0;
                    }
                    //Daca s-a realizat o logare cu succes
                    strcat(text, buff);
                    if (buff[7] == 'W') {
                        logare = 1;
                    }
                    fflush(stdin);
                }
            }
        }
                
    }
    return 0;
}


