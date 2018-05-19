#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdbool.h>

#define BUFLEN 256

//Structura pentru a memora datele userilor
struct Client{
	char nume[13];   //13 cu terminator de sir
	char prenume[13];
	int nr_card;  //6 cifre
	int pin;		//4 cifre
	char parola[9];
	double sold;	//cu dubla precizie
	//Pentru a retine daca un user este logat
	bool logat;
	//Pentru a retine daca un user este blocat
	bool blocat;
	//Pentru a retine numarul de pin-uri introduse gresit
	int nr_try_login;
	//Pentru a retine socketul pe care este logat
	int socket;
};

void error(char *msg)
{
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[])
{
    int sockfd, newsockfd, portno, clilen;
    char buffer[BUFLEN];
    struct sockaddr_in serv_addr, cli_addr;
    int n, i, j;
    FILE *in;
	int N;
	struct Client *cl;
	char command[9];
	in = fopen(argv[2], "r");
	fscanf(in, "%d", &N);
	cl = malloc (N* sizeof(struct Client));
	for (int i = 0; i < N; i++) {
		fscanf(in, "%s %s %d %d %s %lf", cl[i].nume, cl[i].prenume, 
			&cl[i].nr_card, &cl[i].pin, cl[i].parola, &cl[i].sold);
		cl[i].logat = false;
		cl[i].blocat = false;
		cl[i].nr_try_login = 0;
		cl[i].socket = -1;
	}
	fclose(in);

    fd_set read_fds;	//multimea de citire folosita in select()
    fd_set tmp_fds;	//multime folosita temporar 
    int fdmax;		//valoare maxima file descriptor din multimea read_fds

    if (argc < 2) {
        fprintf(stderr,"Usage : %s port\n", argv[0]);
        exit(1);
    }

    //golim multimea de descriptori de citire (read_fds) si multimea tmp_fds 
    FD_ZERO(&read_fds);
    FD_ZERO(&tmp_fds);
    
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
    	error("ERROR opening socket");
     
    portno = atoi(argv[1]);

    memset((char *) &serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;	// foloseste adresa IP a masinii
    serv_addr.sin_port = htons(portno);
     
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr)) < 0) 
        error("ERROR on binding");
    
    listen(sockfd, N);

    //adaugam noul file descriptor (socketul pe care se asculta conexiuni) in multimea read_fds
    FD_SET(sockfd, &read_fds);

    // Adaugam tastatura
    FD_SET(STDIN_FILENO, &read_fds);
    FD_SET(sockfd, &read_fds);

    fdmax = sockfd;

    // main loop
	while (1) {
		tmp_fds = read_fds; 
		if (select(fdmax + 1, &tmp_fds, NULL, NULL, NULL) == -1) 
			error("ERROR in select");
	
		for(i = 0; i <= fdmax; i++) {
			if (FD_ISSET(i, &tmp_fds)) {
				//Citesc de la tastatura (pentru comanda quit)
				if (i == STDIN_FILENO) {
					char buff[100];
					char buffer[100];
					scanf("%s", buff);
					sprintf(buffer, "Serverul se inchide\n");
					if (strcmp(buff, "quit") == 0) {
						//Rescriu fisierul userilor cu datele actualizate
						FILE *out = fopen(argv[2], "w");
						fprintf(out, "%d\n", N);
						for (int i = 0; i < N; i++) {
							fprintf(out, "%s %s %d %d %s %.2f\n", cl[i].nume, cl[i].prenume, 
								cl[i].nr_card, cl[i].pin, cl[i].parola, cl[i].sold);
	}
                        fclose(out);
						//Trimit clientilor mesajul ca serverul se va inchide
						for (int l = 0; l <= fdmax; l++) {
							if (FD_ISSET(l, &read_fds) && l != sockfd && l != 0) {
								printf("Broadcast message to be sent %d msg %s\n", l, buffer);
								n = send(l, buffer, sizeof(buffer), 0);
							}
						}
						//Astept pentru a se putea trimite mesajul
						sleep(2);
						exit(1);
					}
				}
				
				if (i == sockfd) {
					// a venit ceva pe socketul inactiv(cel cu listen) = o noua conexiune
					// actiunea serverului: accept()
					clilen = sizeof(cli_addr);
					if ((newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen)) == -1) {
						error("ERROR in accept");
					} 
					else {
						//adaug noul socket intors de accept() la multimea descriptorilor de citire
						FD_SET(newsockfd, &read_fds);
						if (newsockfd > fdmax) { 
							fdmax = newsockfd;
						}
					}
					printf("Noua conexiune de la %s, port %d, socket_client %d\n ", inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port), newsockfd);
				}
					
				else {
					// am primit date pe unul din socketii cu care vorbesc cu clientii
					//actiunea serverului: recv()
					memset(buffer, 0, BUFLEN);
					if ((n = recv(i, buffer, sizeof(buffer), 0)) <= 0) {
						if (n == 0) {
							//conexiunea s-a inchis
							printf("selectserver: socket %d hung up\n", i);
						} else {
							error("ERROR in recv");
						}
						close(i); 
						FD_CLR(i, &read_fds); // scoatem din multimea de citire socketul pe care 
					} 
					
					else { //recv intoarce >0
						printf ("Am primit de la clientul de pe socketul %d, mesajul: %s\n", i, buffer);
						if (strcmp(buffer, "quit") == 0){
							close(i); 
							FD_CLR(i, &read_fds);
						}
						int invalid = 0;
						if (strncmp(buffer, "login", 5) == 0){
							invalid = 1;
							char *token;
							int nr_card;
							int pin;
							int find_card = 0;
							int find_pin = 0;
							int index_client = 0;
							token = strtok(buffer, " ");
							token = strtok(NULL, " ");
							nr_card = atoi(token);
							token = strtok(NULL, " ");
							pin = atoi(token);
							//Caut userul cu numarul de card pe care l-am primit
							for (int s = 0; s < N; s++) {
								if (cl[s].nr_card == nr_card) {
									find_card = 1;
									if (cl[s].pin == pin) {
										find_pin = 1;
										index_client = s;
									}
								}
								if (find_card == 1 && find_pin == 1) {
									break;
								}
							}
							//Verific daca userul nu este deja logat in alt client
							if (cl[index_client].logat == 1) {
								memset(buffer, 0, BUFLEN);
								strcpy(buffer, "IBANK> -2 : Sesiune deja deschisa\n");
							}
							else {
								//Verific daca userul este blocat
								if (cl[index_client].blocat == 1) {
									memset(buffer, 0, BUFLEN);
									strcpy(buffer, "IBANK> -5 : Card blocat\n");

								} else {
									//Verific daca am gasit userul
									if (find_card == 0) {
										memset(buffer, 0, BUFLEN);
										strcpy(buffer, "IBANK> -4 : Numar card inexistent\n");
									}
									else {
										//Verific daca pinul este corect
										if (find_pin == 0) {
											memset(buffer, 0, BUFLEN);
											strcpy(buffer, "IBANK> -3 : Pin gresit\n");
											//Daca pinul nu este corect incrementez numarul de pinul introduse gresit
											cl[index_client].nr_try_login++;
										}
										else {
											memset(buffer, 0, BUFLEN);
											sprintf(buffer, "IBANK> Welcome %s %s\n", cl[index_client].nume, cl[index_client].prenume);
											//Actualizez variabila care imi spune daca un user este logat
											cl[index_client].logat = 1;
											cl[index_client].nr_try_login = 0;
											//Tin minte pe ce socket este logat userul
											cl[index_client].socket = i;
										}
									}
									//Daca pinul este introdus gresit de 3 ori la rand, blochez cardul
									if (cl[index_client].nr_try_login == 3) {
										cl[index_client].blocat = 1;
										cl[index_client].nr_try_login = 0;
										memset(buffer, 0, BUFLEN);
										strcpy(buffer, "IBANK> -5 : Card blocat\n");
									}
								}
							}
						}
						if (strcmp(buffer, "logout") == 0) {
							invalid = 1;
							//Caut userul logat pe socketul curent
							for (int s = 0; s < N; s++) {
								if (cl[s].socket == i) {
									cl[s].logat = 0;
									cl[s].socket = -1;
									break;
								}
							}
							memset(buffer, 0, BUFLEN);
							strcpy(buffer, "IBANK> Clientul a fost deconectat\n");
						}
						if (strcmp(buffer, "listsold") == 0) {
							invalid = 1;
							//Caut userul logat pe socketul curent
							for (int s = 0; s < N; s++) {
								if (cl[s].socket == i) {
									memset(buffer, 0, BUFLEN);
									sprintf(buffer, "IBANK> %.2f\n", cl[s].sold);
									break;
								}
							}
						}
						if (strncmp(buffer, "transfer", 8) == 0) {
							invalid = 1;
							int sold_sender = 0;
							int index_sender = 0;
							int card_receiver = 0;
							int index_receiver = -1;
							double sold_transfer = 0;
							char *token;
							token = strtok(buffer, " ");
							token = strtok(NULL, " ");
							card_receiver = atoi(token);
							token = strtok(NULL, " ");
							sold_transfer = atoi(token);
							//Caut userul care vrea sa transfere bani si userul catre care vrea sa trimita banii
							for (int s = 0; s < N; s++) {
								if (cl[s].socket == i) {
									index_sender = s;
								}
								if (cl[s].nr_card == card_receiver) {
									index_receiver = s;
								}
							}
							memset(buffer, 0, BUFLEN);
							//Verific daca a gasit userul catre care vrea sa trimita bani userul curent
							if (index_receiver == -1) {
								strcpy(buffer, "IBANK> -4 : Numar card inexistent\n");
							} else {
								sold_sender = cl[index_sender].sold;
								//Verific daca userul care vrea sa trimita are suficienti bani
								if (sold_transfer > sold_sender) {
									strcpy(buffer, "IBANK> -8 : Fonduri insuficiente\n");
								} else {
									sprintf(buffer, "IBANK Transfer %.2f catre %s %s? [y/n]\n", sold_transfer, cl[index_receiver].nume, cl[index_receiver].prenume);
									n = send(i, buffer, sizeof(buffer), 0);
									//Astept confirmarea de la user (de la socketul pe care este logat userul)
									if ((n = recv(i, buffer, sizeof(buffer), 0)) <= 0) {
										if (n == 0) {
											//conexiunea s-a inchis
											printf("selectserver: socket %d hung up\n", i);
										} else {
											error("ERROR in recv");
										}
										close(i); 
										FD_CLR(i, &read_fds); // scoatem din multimea de citire socketul pe care 
									} else {
										//Verific daca userul confirma transferul
										if (buffer[0] == 'y') {	
											cl[index_receiver].sold += sold_transfer;
											cl[index_sender].sold -= sold_transfer;
											strcpy(buffer, "IBANK> Transfer realizat cu success\n");
										}
										else {
											memset(buffer, 0, BUFLEN);
											strcpy(buffer, "IBANK> -9 : Operatie anulata\n");
										}
									}
								}
							}
						}
						n = send(i, buffer, sizeof(buffer), 0);
					}
				} 
			}
		}
    }
    close(sockfd);
    return 0; 
}


