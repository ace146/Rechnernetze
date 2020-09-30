/**
 * @file server.c
 * @author Vadim Budagov, Ömer Kirdas
 * Created on: 24.05.2019
 * @brief Server, der in der Lage ist mit mehreren Clients zu kommunizieren.
 * 			Er kann die Datei vom Client abspeichern, an Client zurücksichen.
 * 			Außerdem erhält der Server die Informationen über alle mit ihm verbundenen Clients.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "vector.c" //Hilfsklasse, um die Clients zu referenzieren.

#define MAXDATA 1024
#define bufferSize 255
#define PORT "8080"   
#define MAXCLIENTS 10
/**
 * Eine Struktur, in der Client-Infos gehalten werden
 * @param fd, File Descriptor des Clients
 * @param hosname, hostname des Clients
 * @param port, Portnummer des Clients
 */
typedef struct {
	int fd;
	char* hostname;
	unsigned short port;
} clientinfo;

char filenamePut[bufferSize];
char buf[256];    // Buffer für die Client Daten

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
 * @param *sa, sockaddr
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
int getFileContent(char* filename, char *content) {
	FILE* fp = fopen(filename, "rb");
	int size = 0;
	char buffer[MAXDATA];
	size_t i;

	for (i = 0; i < MAXDATA; i++) {
		int c = getc(fp);
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
 * @brief sendet die Datei über den Socket zum Client
 * @param filename, die zu versendende Datei
 */
int sendFile(char* filename, int sock) {

	char filecontent[MAXDATA] = { 0 };
	int size = getFileContent(filename, filecontent);
	int bytes = write(sock, filecontent, size);
	printf("%i from %i bytes of data were sent\n", bytes, size);
	return bytes;
}

/**
 * @brief Empfangen der Datei vom Client uber den Socket.
 * @param sock, Socket Descriptor über den wir die Client-Datei erhalten.
 * @return nbytes, Anzahl von empfangenen Bytes aus der Datei.
 */
int receiveFileFromClient(int sock) {
	int nbytes;
	memset(buf, 0, MAXDATA);
	memset(filenamePut, 0, MAXDATA);
	printf("In receiveFileFromClientFunktion\n");
	//readfilename
	nbytes = read(sock, filenamePut, bufferSize);
	printf("filenamePut inhalt: %s\n", filenamePut);
	if (nbytes < 0) {
		perror("Error during the read function...");
	}
	nbytes = read(sock, buf, bufferSize);
	printf("Bufferinhalt: %s\n", buf);
	printf("Data received: %i bytes\n", nbytes);
	return nbytes;

}

/**
 * @brief liest byteweise die Client-Datei aus und Speichert diese auf der Platte.
 * @param bytes, Anzahl der Bytes
 * @param filename, Name der Client-Datei
 * @return n, Anzahl der geschriebenen Elementen
 */
int saveClientFile(int bytes, char* filename) {
	//save received data on disk
	FILE* receivedFile;
	receivedFile = fopen(filenamePut, "wb");
	//errorhandling TODO
	//write file on disk
	int n = fwrite(buf, bytes, 1, receivedFile);
	//close file
	fclose(receivedFile);
	//clear the buffer
	memset(buf, 0, MAXDATA);
	return n;
}
/**
 * Server - MAIN
 */
int main(void) {
	printf("anfang main\n");
	fflush(stdout);
	fd_set master;    // Die Master File Deskriptoren Liste
	fd_set read_fds;  // Temporäre File Deskriptor Liste für select()
	int fdmax;        // Die maximale Anzahl an File Deskriptoren

	int listener;     // Listening Socket Deskriptor
	int newfd;        // Neuester akzeptierter Socket Deskriptor
	struct sockaddr_storage remoteaddr; // Client Adresse
	socklen_t addrlen;

	vector v; //Vektor, der Client-Infor enthält.
	vector_init(&v/*clientinfos*/);

	int nbytes;

	char remoteIP[INET6_ADDRSTRLEN];
	unsigned short readport;

	int yes = 1;        // für setsockopt() SO_REUSEADDR, siehe unten
	int i, j, rv;
	printf("vor readfile in main\n");

	struct addrinfo hints, *ai, *p;

	FD_ZERO(&master);    // Platz für die Master- und Temporären Mengen machen
	FD_ZERO(&read_fds);

	// Hole ein Socket und binde es
	memset(&hints, 0, sizeof hints); //Kümmert sich darum, dass das struct leer ist.
	hints.ai_family = AF_INET6;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	if ((rv = getaddrinfo(NULL, PORT, &hints, &ai)) != 0) {
		fprintf(stderr, "selectserver: %s\n", gai_strerror(rv));
		exit(1);
	}
	// Iteriere über die result-Liste und benutze diejenige Adressstruktur,
	// welche als erstes funktioniert. Erzeuge dann den Socket.
for (p = ai; p != NULL; p = p->ai_next) {
	listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
	if (listener < 0) {
		continue;
	}

	setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)); //Setze Socket Optionen
    // binde an listener
	if (bind(listener, p->ai_addr, p->ai_addrlen) < 0) {
		close(listener);
		continue;
	}
	break;
}

// Falls wir hier ankommen, bedeutet das, dass das Binden fehlgeschlagen ist
if (p == NULL) {
	fprintf(stderr, "selectserver: failed to bind\n");
	exit(2);
}

freeaddrinfo(ai); // all done with this
// Höre auf Verbindungen
if (listen(listener, MAXCLIENTS) == -1) {
	perror("listen\n");
	exit(3);
}

	// Füge den Listener zum Master Set hinzu
	FD_SET(listener, &master);

	// Merke Dir den größten File Deskriptor
	fdmax = listener; // Bis jetzt ist es dieser hier
	printf("vor while loop\n");
// main loop	
while (1) {
	char clientname[bufferSize];
	clientinfo* infop = malloc(sizeof(clientinfo));
	clientinfo* c;
	read_fds = master; // Kopieren
	//Kontrolliere mehrere Clients
	if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1) {
		perror("select");
		exit(4);
	} 
// Gehe durch die aktuellen Verbindungen und sieh nach, ob es Daten gibt, die man lesen kann
 for (i = 0; i <= fdmax; i++) {
	 if (FD_ISSET(i, &read_fds)) { // Es gibt Einen!
			if (i == listener) {
				// Verwalte die Verbindungen
				addrlen = sizeof remoteaddr;
				newfd = accept(listener, (struct sockaddr *) &remoteaddr,
						&addrlen);

				if (newfd == -1) {
					perror("accept\n");
				} else {
					FD_SET(newfd, &master); // Zum Master Set hinzufügen
					if (newfd > fdmax) { // Weiterhin gucken, welches das größte ist.
						fdmax = newfd;
					}
					printf("selectserver: new connection from %s on "
							"socket %d\n",
							inet_ntop(remoteaddr.ss_family,get_in_addr(
												(struct sockaddr*) &remoteaddr),
												remoteIP, INET6_ADDRSTRLEN), newfd);
					readport = get_port((struct sockaddr*) &remoteaddr);
					readport = ntohs(readport);
					printf("Port: %hu\n", readport);

		//***********************************************

					read(newfd, clientname, bufferSize);
					printf("CLIENTNAME: %s\n", clientname);
					printf("angekommen!\n");
					char* memclientname = malloc(bufferSize);
					
					clientinfo info;
					info.fd = newfd;
					info.hostname = clientname;
					info.port = readport;

					memcpy(memclientname, &clientname, strlen(clientname));
					
					memcpy(infop, &info, sizeof(info));

					vector_add(&v, infop);
				}
		   } else {
				int nbytes2;
				memset(buf, 0, MAXDATA);
				nbytes = read(i, buf, bufferSize);

				//*******************PUT**************************
                // Empfange Put-Commando vom Client
				if (!strcmp(buf, "Put")) {
					char hostname2[bufferSize];
					char* hostIP;
					char hostInfo[bufferSize];
					char timeStamp[bufferSize];
					time_t t;
					struct tm * ts;

					t = time(NULL);
					ts = localtime(&t);
					sprintf(timeStamp, "%s", asctime(ts));

					struct hostent *host_entry;

					if (gethostname(hostname2, MAXDATA) == -1) {

					}
					host_entry = gethostbyname(hostname2);
					hostIP =inet_ntoa(*((struct in_addr*) host_entry->h_addr_list[0]));
										size_t hostIPSize = strnlen(hostIP, bufferSize);
					printf("HOSTNAME: %s \n", hostname2);
					printf("HOSTIP: %s \n", hostIP);

						//Daten zusammenfügen und abschicken
					strcat(hostInfo, hostname2);
					strcat(hostInfo, " ");
					strcat(hostInfo, hostIP);
					strcat(hostInfo, " ");
					strcat(hostInfo, timeStamp);

					write(i, hostInfo, bufferSize);

					printf("Received put request from Client\n");
					nbytes2 = receiveFileFromClient(i);

					if (nbytes2 < 0) {
						perror("Error during the read function...");
						fflush(stderr);
					}
					//save received client file on disk

					nbytes2 = saveClientFile(nbytes2, buf);
					if (nbytes2 < 0) {
						perror("Error while writing function");
						fflush(stderr);
					}
					// Empfange Get-Commando vom Client
				} else if (!strcmp(buf, "Get")) {

						//********DATAINFOS************************
					int ack = 1;
					write(i, &ack, bufferSize);
					nbytes = read(i, filenamePut, bufferSize);

					struct stat fileInfos;
					struct tm* ts;
					if (stat(filenamePut, &fileInfos) == -1) {
						perror("stat: ");
						fflush(stderr);
						continue;
					}

					time_t lastModified = fileInfos.st_mtime;
					off_t size = fileInfos.st_size;

					char infos[bufferSize] = { 0 };
					char fileSize[bufferSize] = { 0 };
					char timeStamp[bufferSize] = { 0 };
					strftime(timeStamp, bufferSize, "%Y-%m-%d %H:%M:%S",
							localtime(&lastModified));
					sprintf(fileSize, "%li ", size);
						
					strcat(infos, fileSize);
					strcat(infos, timeStamp);
					write(i, infos, bufferSize);
					char readAck[bufferSize];
					read(i, readAck, bufferSize);
					sendFile(filenamePut, i);

					//****************************************
                    //Empfange List-Commando vom Client
				} else if (!strcmp(buf, "List")) {
					fprintf(stdout,"LIST");
					int a;
					int listSize = vector_count(&v);
					char clientInfos[bufferSize] = { 0 };
					char clientfd[bufferSize] = { 0 };
					char clientPort[bufferSize] = { 0 };
					for (a = 0; a < listSize; a++) {
						c = (clientinfo*) vector_get(&v, a);

						sprintf(clientfd, "%i ", c->fd);
						strcat(clientInfos, clientfd);
						strcat(clientInfos, c->hostname);
						strcat(clientInfos, " ");
						if ((a + 1) == listSize) {
							sprintf(clientPort, "%hu", c->port);
						} else {
							sprintf(clientPort, "%hu ", c->port);
						}
						strcat(clientInfos, clientPort);
					}
					write(i, clientInfos, bufferSize);
                // Empfange Quit-Commando vom Client
				} else if (!strcmp(buf, "Quit")) {
					fprintf(stdout,"QUIIIIT\n");
					int listSize = vector_count(&v);
					int a;
					for (a = 0; a < listSize; a++) {
						c = (clientinfo*) vector_get(&v, a);
						fprintf(stdout,"filediskriptor: %i\n",i);
					    if(c->fd == i){
							fprintf(stdout,"LISTE WIRD GELÖSCHT: \n");
							vector_delete(&v, a);
							break;
						}
					}
					int ack = 2;
					write(i, &ack, bufferSize);
					FD_CLR(i,&master);
					
				     }
			} // END Datenbehandlung vom Nutzer
		} // END neue Verbindung bekommen
	} // END Durchlaufen der File Deskriptoren
} // END for(;;)--Und Du dachtest vermutlich schon, dass es nie enden wird.!
return 0;
}
