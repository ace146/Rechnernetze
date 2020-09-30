/**
 * @file client.c
 * @author Ömer Kirdas, Vadim Budagov
 * Created on: 24.05.2019
 * @brief Client, der mit dem Server kommuniziert. Der Client kann eine Text-Datei
 * 			beim Server anlegen(Put), sie wieder anfordern(Get) und die Verbindung beenden.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <time.h>
#include <string.h>
#include <arpa/inet.h>

#define PORT "8080" 
#define bufferSize 255
#define MAXDATASIZE 1000 
int sockfd, numbytes;

/**
 * @brief Prüft, ob Argument valide ist.
 * @return 1 wenn alles in ordnung, sonst 0
 * @param str, der zu überprüfende Argument
 */
int isValidArgument(char* str) {
	return (str != NULL) && (strlen(str) > 0);
}

/**
 * @brief sockaddr zu IPv4 wenn sa_family == AF_INET, sonst IPv6
 * @param *sa, sockaddr
 */
void *get_in_addr(struct sockaddr *sa) {
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*) sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*) sa)->sin6_addr);
}

/**
 * @brief liefert den Portnummer
 * @return unsignet short, Portnummer
 */
unsigned short get_port(struct sockaddr *sa) {
	if (sa->sa_family == AF_INET) {
		return (((struct sockaddr_in*) sa)->sin_port);
	}

	return (((struct sockaddr_in6*) sa)->sin6_port);
}

/**
 * @brief Packt den Inhalt vom Fime in den Buffer
 * @return size, Die Größe des Inhalts
 * @param fimename, Name der Datei
 * @param content, Inhalt der Datei
 */
int getFileContent(char* filename, char* content) {
	FILE* fp = fopen(filename, "r+");
	int size = 0;
	char buffer[MAXDATASIZE];
	size_t i;

	for (i = 0; i < MAXDATASIZE; ++i) {
		int c = getc(fp);
		if (c == 0) {
			fprintf(stderr, "No more chars in this File!");
			fflush(stderr);
			break;
		}

		size++;
		if (c == EOF) {
			buffer[i] = 0x00;
			break;
		}
		buffer[i] = c;
	}

	fclose(fp);
	memcpy(content, buffer, size);
	return size;
}

/**
 * @brief sendet die Datei über den Socket zum Server
 * @param filename, die zu versendende Datei
 */
void sendFile(char* filename) {
	int sentbytes = 0;
	printf("Sending %s to Server\n", filename);
	fflush(stdout);
	//send filename to server    
	sentbytes = write(sockfd, filename, bufferSize);
	char filecontent[MAXDATASIZE] = { 0 };
	int size = getFileContent(filename, filecontent);
	//sent filecontent to server
	sentbytes = write(sockfd, filecontent, size);

	printf("%i from %i bytes of data were sent to the Server\n", sentbytes,
			size);
}
/**
 * @brief Printet Client-Infos aus
 * @param client, dessen Infos geprintet werden
 */
void printClientInfos(char* clients){
	int i=0;
	while(clients[i] != '\0'){
		  printf("CLIENTFD: %c\n",  clients[i]);
		  i = i+2;

		  printf("Hostname: ");
		  while(clients[i] != ' '){
		      printf("%c", clients[i]);
			  i = i+1;
		  }
		  printf("\n");
		  i = i+1;
		
		  printf("ClientPort: ");
		  while(clients[i] != ' ' && clients[i] !='\0'){
		      printf("%c", clients[i]);
			  i = i+1;
		  }
		  i = i+1;
		  printf("\n\n\n");
		}
}
/**
 * @brief MAIN, Befor Client gestartet wird,
 * 			wird ein Argument (hostname des Servers) erwartet.
 */
int main(int argc, char *argv[]) {
	char buf[MAXDATASIZE];
	struct addrinfo hints, *servinfo, *p;
	int rv;
	char s[INET6_ADDRSTRLEN];
	unsigned short readport = 0;

    //Prüfe, ob Hostname eingegeben wurde.
	if (argc != 2) {
		fprintf(stderr, "usage: client hostname\n");
		exit(1);
	}

	memset(&hints, 0, sizeof hints); //Kümmert sich darum, dass das struct leer ist.
	hints.ai_family = AF_UNSPEC;     // Arbeite sowohl mit IPv4 als auch mit IPv6
	hints.ai_socktype = SOCK_STREAM; // TCP stream Socket

	printf("argc: %i; argv[1]: %s\n", argc, argv[1]); // Ausgabe zum Testen von Hostname
    
    // AdressInformation in &serviceinfo packen, um später dadurch zu iterieren.
	if ((rv = getaddrinfo(argv[1], PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// Iteriere über die result-Liste und benutze diejenige Adressstruktur,
	// welche als erstes funktioniert. Erzeuge dann den Socket.
	for (p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol))
				== -1) {
			perror("client: socket");
			continue;
		}
        // Verbindet mit dem Server über erzeugte sockfd
		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("client: connect");
			continue;
		}

		break;
	}

	if (p == NULL) {
		fprintf(stderr, "client: failed to connect\n");
		return 2;
	}
    // Networtk to Presentation
	inet_ntop(p->ai_family, get_in_addr((struct sockaddr *) p->ai_addr), s,
			sizeof s);

	readport = get_port((struct sockaddr *) p->ai_addr);
	readport = ntohs(readport); // Network to host short

	printf("Listener-Port: %hu\n", readport);

	printf("client: connecting to %s\n", s);

	freeaddrinfo(servinfo); //Leere den Linked List.

	//****************************************ACTION***********************************

	char* get = "Get";
	char* list = "List";
	char* put = "Put";
	char* quit = "Quit";
	char clientname[bufferSize] = { 0 };
    //gib den hostname aus, auf dem das Programm läuft.
	if (gethostname(clientname, bufferSize) == -1) {
		perror("gethostname: ");
	}
	if (write(sockfd, clientname, bufferSize) == -1) {
		perror("write: ");
	}
    /**
	 * While Loop
	 */
	while (1) {
		char* input = NULL;
		char* command = NULL;
		char* filename = NULL;
		size_t inputsize = 0;

		long int size = 0;
        //Lese die Engaben aus der Konsole aus!
		if (getline(&input, &inputsize, stdin) == -1) {
			perror("Failed to get input");
			fflush(stderr);
			input = NULL;
			return -1;
		}
		inputsize = strlen(input);
		if (input[inputsize - 1] == '\n') {
			input[inputsize - 1] = '\0';
		}

		command = strtok(input, " ");
		printf("Command: %s\n\n\n", command);
//********************PUT**********************************
        //Schike Put-Commando an Server, damit der Server Weiß, worum es sich handelt.
		// danach verschicke auch die Datei.
		if (!strcmp(command, put)) {
			char *hostname;
			char *hostIP;
			char *time;
			char hostInfo[bufferSize];

			fprintf(stdout, "Kommando PUT \n");
			fflush(stdout);
			if (write(sockfd, put, bufferSize) == -1) {
				perror("Failed write for put");
			}

			filename = strtok(NULL, " ");

			printf("FILENAME: %s \n", filename);

			if (isValidArgument(filename)) {
				sendFile(filename);
			} else {
				fprintf(stderr, "client: No Filename \n");
				fflush(stderr);
			}

			read(sockfd, hostInfo, bufferSize);

			hostname = strtok(hostInfo, " ");
			hostIP = strtok(NULL, " ");
			time = strtok(NULL, "\0");

			printf("HOSTNAME: %s \n", hostname);
			printf("HOSTIP: %s \n", hostIP);
			printf("TIME: %s \n", time);

		}
//*******************GET****************************************
        //Schicke Get-Command an Server, und erhalte die angeforderte Datei.
		else if (!strcmp(command, get)) {
			char* timeStamp = NULL;
			char fileContent[bufferSize] = { 0 };
			char* fileSize;
			char infos[bufferSize] = { 0 };
			fprintf(stdout, "Kommando GET \n");
			fflush(stdout);
			if (write(sockfd, get, bufferSize) == -1) {
				perror("Failed write for put");
			}
			int ack[bufferSize];
			read(sockfd, ack, bufferSize);

			filename = strtok(NULL, " ");
			write(sockfd, filename, strlen(filename));

			read(sockfd, infos, bufferSize);
			int vier = 4;
			write(sockfd, &vier, sizeof(int));
			read(sockfd, fileContent, bufferSize);

			fileSize = strtok(infos, " ");
			timeStamp = strtok(NULL, "\0");

			printf("Size: %s\n", fileSize);
			printf("Last Modified: %s\n", timeStamp);
			printf("Datei Inhalt: %s\n", fileContent);
		}
//*******************LIST***************************************
        //Schicke List-Command an Server und erhalte eine Liste mit allen Clients,
		//die mit dem Server verbunden sind.
		else if (!strcmp(command, list)) {
			fprintf(stdout, "Kommando LIST \n");
			fflush(stdout);
			if (write(sockfd, list, bufferSize) == -1) {
				perror("Failed write for put");
			}
			
			char clients[bufferSize] = {0};
			memset(clients,0,bufferSize);
			int nbytes = read(sockfd, clients, bufferSize);
			if (nbytes < 0) {
				perror("Error during the read function...");
			}
			printClientInfos(clients);
			
			memset(clients,0,bufferSize);
		}
//*******************QUIT****************************************
        //Clientseitige Beendigung der Verbindung
		else if (!strcmp(command, quit)) {
			if (write(sockfd, quit, bufferSize) == -1) {
				perror("Failed write for quit");
			}
			break;
		} else {
			puts("fehlgeschlagen\n");
		}

		buf[numbytes] = '\0';
	}

	buf[numbytes] = '\0';

	printf("client: received '%s'\n", buf);

	close(sockfd);
	return 0;
}
