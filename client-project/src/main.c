/*
 * main.c
 *
 * TCP Client - Template for Computer Networks assignment
 *
 * This file contains the boilerplate code for a TCP client
 * portable across Windows, Linux and macOS.
 */

#if defined WIN32
#include <winsock.h>
#else
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#define closesocket close
#endif

#include <stdio.h>
#include <stdlib.h>
#include "protocol.h"

#define NO_ERROR 0

void clearwinsock() {
#if defined WIN32
	WSACleanup();
#endif
}

void errorhandler(char *errorMessage){
	printf("%s", errorMessage);
}

void arguments_error(const char* prog_name) {
    fprintf(stderr, "Usa: %s [-s server] [-p port] -r \"type city\"\n", prog_name);
    fprintf(stderr, "  -s server: IP address/server hostname(default: %s)\n", DEFAULT_HOST);
    fprintf(stderr, "  -p port:   Server port (default: %d)\n", SERVER_PORT);
    fprintf(stderr, "  -r request: Richiesta obbligatoria (eg. \"t bari\")\n");
    exit(EXIT_FAILURE);
}

void control_arguments(int argc, char* argv[], char** server_host, int* port, char** request_str){
	if (argc < 3) {
			arguments_error(argv[0]);
		}

	    for (int i = 1; i < argc; i++) {
	        if (strcmp(argv[i], "-s") == 0) {
	            if (++i < argc) *server_host = argv[i];
	            else arguments_error(argv[0]);
	        } else if (strcmp(argv[i], "-p") == 0) {
	            if (++i < argc) *port = atoi(argv[i]);
	            else arguments_error(argv[0]);
	        } else if (strcmp(argv[i], "-r") == 0) {
	            if (++i < argc) *request_str = argv[i];
	            else arguments_error(argv[0]);
	        } else {
	            fprintf(stderr, "Errore. Opzione sconosciuta: '%s'\n", argv[i]);
	            arguments_error(argv[0]);
	        }
	    }

	    // CONTROLLA CHE CI SIA -r
	    if (*request_str == NULL) {
	        fprintf(stderr, "Errore: il parametro -r è obbligatorio.\n");
	        arguments_error(argv[0]);
	    }

	    // CONTROLLA CHE LA PORTA SIA VALIDA
	    if (*port <= 0 || *port > 65535) {
	        fprintf(stderr, "Errore. Numero di porta: '%d' non valido.\n", *port);
	        exit(EXIT_FAILURE);
	    }
}

void capitalize_city(char *str) {
    if (str && *str) {
        *str = toupper((unsigned char)*str);
        for (int i = 1; str[i]; i++){
        	str[i] = tolower((unsigned char)str[i]);
        }
    }
}

void build_msg(weather_response_t res, weather_request_t req){
	char displayCity[CITY_NAME_SIZE];
	strncpy(displayCity, req.city, 63);
	displayCity[63] = '\0';
	capitalize_city(displayCity);

	switch(res.type){
		case TEMPERATURE:{
			printf("%s: Temperatura = %.1f°C\n", displayCity, res.value);
			break;
		}
		case HUMIDITY:{
			printf("%s: Umidita' = %.1f%%\n", displayCity, res.value);
			break;
		}
		case WIND_SPEED:{
			printf("%s: Vento = %.1f km/h\n", displayCity, res.value);
			break;
		}
		case PRESSURE:{
			printf("%s: Pressione = %.1f hPa\n", displayCity, res.value);
			break;
		}
	}
}

int main(int argc, char *argv[]) {


	char* server_host = DEFAULT_HOST;
	int port = SERVER_PORT;
	char* request_str = NULL;
	// CONTROLLO ARGOMENTI DA RIGA DI COMANDO
	control_arguments(argc, argv, &server_host, &port, &request_str);
	if (strlen(request_str) < 3) {
		arguments_error(argv[0]);
	}

	char *city_ptr = request_str + 1;

	// Salta spazi tra il tipo e il nome della città
	while (*city_ptr && isspace((unsigned char)*city_ptr)) {
		city_ptr++;
	}

	if (*city_ptr == '\0') {
		fprintf(stderr, "Errore: nome città mancante.\n");
	    exit(EXIT_FAILURE);
	}

    // COSTRUZIONE RICHIESTA CLIENT
    weather_request_t req;
    memset(&req, 0, sizeof(req));
    req.type = request_str[0];
    strncpy(req.city, request_str + 2, CITY_NAME_SIZE - 1); // Copia il resto della stringa (dopo "t ") nella città
    req.city[CITY_NAME_SIZE - 1] = '\0';  // Assicura la terminazione

	//INIZIALIZZAZIONE WINSOCK
	#if defined WIN32
	    WSADATA wsa_data;
	    int result = WSAStartup(MAKEWORD(2,2), &wsa_data);
	    if (result != NO_ERROR) {
	    	printf("Error at WSAStartup()\n");
	    	return 0;
	    }
	#endif



	//CREAZIONE DELLA SOCKET
	int my_socket;
	my_socket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(my_socket < 0){
		errorhandler("Creazione della socket fallita.\n");
		closesocket(my_socket);
		clearwinsock();
	return -1;
	}

	//COSTRUZIONE DELL'INDIRIZZO DEL SERVER
	struct sockaddr_in sad;
	memset(&sad, 0, sizeof(sad));
	sad.sin_family = AF_INET;
	sad.sin_addr.s_addr = inet_addr(server_host);
	sad.sin_port = htons(port);

	//CONNESSIONE AL SERVER
	if(connect(my_socket, (struct sockaddr*)&sad, sizeof(sad)) < 0){
		errorhandler("Connessione fallita.\n");
		closesocket(my_socket);
		clearwinsock();
		return -1;
	}

	//INVIO RICHIESTA METEO
	if(send(my_socket,(char*)&req, sizeof(req), 0) != sizeof(req)){
			errorhandler("send(word1) ha inviato un numero di byte diverso da quelli aspettati.\n");
			closesocket(my_socket);
			clearwinsock();
			return -1;
		}

	//RICEZIONE RISPOSTA SERVER
	weather_response_t res;
	int bytes_rcvd;
	if((bytes_rcvd = recv(my_socket, (char*)&res, sizeof(weather_response_t), 0)) <= 0){
		errorhandler("recv(res) ha fallito o la connessione è stata chiusa prematuramente.\n");
		closesocket(my_socket);
		clearwinsock();
		return -1;
	}

	//COSTRUZIONE DEL MESSAGGIO
	printf("Ricevuto risultato dal server ip %s. ", server_host);
	switch(res.status){
		case 0:{
			build_msg(res, req);
			break;
		}
		case 1:{
			printf("Citta' non disponibile\n");
			break;
		}
		case 2:{
			printf("Richiesta non valida\n");
			break;
		}
		default: printf("Stato sconosciuto.\n");
	}


	//CHIUSURA CONNESSIONE
	printf("Terminazione del client.\n");
	closesocket(my_socket);
	clearwinsock();
	return 0;
} // main end
