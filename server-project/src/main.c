/*
 * main.c
 *
 * TCP Server - Template for Computer Networks assignment
 *
 * This file contains the boilerplate code for a TCP server
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
#include <time.h>
#include <string.h>
#include "protocol.h"

#define NO_ERROR 0

void clearwinsock() {
#if defined WIN32
	WSACleanup();
#endif
}

void argumentPort_control(int argc, char* argv[], int* port){
	//se voglio cambiare il numero di porta da terminale
	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-p") == 0 && i + 1 < argc) {
			*port = atoi(argv[++i]);
	    }
	}

	//controllo sul numero di porta
	if (*port <= 0 || *port > 65535) {
		fprintf(stderr, "Errore. Porta '%d' non valida.\n", *port);
		exit(EXIT_FAILURE);
    }
}

void errorhandler(char *errorMessage){
	printf("%s", errorMessage);
}


#include <string.h> // Per strcmp()
#include <ctype.h>  // Per tolower()


unsigned int checkResponse(weather_request_t *req) {

    if (req->type != 't' && req->type != 'h' && req->type != 'w' && req->type != 'p') {
        return STATUS_INVALID_REQUEST;
    }

    const char* valid_cities[] = {"bari", "roma", "milano", "napoli", "torino", "palermo", "genova", "bologna", "firenze", "venezia", "reggio calabria", NULL};

    for (int j = 0; req->city[j]; j++) {
        req->city[j] = tolower(req->city[j]);
    }

    unsigned int status = STATUS_CITY_NOT_FOUND;

    for (int i = 0; valid_cities[i] != NULL; i++) {
        if (strcmp(req->city, valid_cities[i]) == 0) {
            status = STATUS_SUCCESS;
            break;
        }
    }
    return status;
}

float get_temperature(void){ // Range: -10.0 to 40.0 °C
	float random_base = (float)rand() / (float)RAND_MAX; //trova un vero float casuale
	float temperature = (random_base * 50.0) - 10.0;
	return temperature;
}
float get_humidity(void){ // Range: 20.0 to 100.0 %
	float random_base = (float)rand() / (float)RAND_MAX;
	float humidity = (random_base * 80.0) + 20.0;
	return humidity;
}
float get_wind(void){  // Range: 0.0 to 100.0 km/h
	float random_base = (float)rand() / (float)RAND_MAX;
	float wind = (random_base * 100.0);
	return wind;
}
float get_pressure(void){  // Range: 950.0 to 1050.0 hPa
	float random_base = (float)rand() / (float)RAND_MAX;
	float pressure = (random_base * 100.0) + 950.0;
	return pressure;
}

float set_value(char type){
	float value = 0.0;
	switch(type){
		case TEMPERATURE:{
			value = get_temperature();
			break;
		}
		case HUMIDITY:{
			value = get_humidity();
			break;
		}
		case WIND_SPEED:{
			value = get_wind();
			break;
		}
		case PRESSURE:{
			value = get_pressure();
			break;
		}
	}
	return value;
}


int main(int argc, char *argv[]) {
	//CONTROLLA IL NUMERO DI PORTA PASSATO TRAMITE ARGOMENTO
	int port = SERVER_PORT;
	argumentPort_control(argc, argv, &port);


#if defined WIN32
	// Initialize Winsock
	WSADATA wsa_data;
	int result = WSAStartup(MAKEWORD(2,2), &wsa_data);
	if (result != NO_ERROR) {
		printf("Error at WSAStartup()\n");
		return 0;
	}
#endif

	srand(time(NULL)); //PERMETTE LA GENERAZIONE DI NUMERI PSEUDO-CASUALI PER I VALORI METEO
	int my_socket;
	my_socket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(my_socket < 0){
		errorhandler("Creazione della socket fallita.\n");
		clearwinsock();
		return -1;
	}

	// CONFIGURAZIONE DELLA SOCKET
	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);
	server_addr.sin_addr.s_addr = INADDR_ANY;

	//ASSEGNAZIONE DELLA SOCKET
	if(bind(my_socket, (struct sockaddr*) &server_addr, sizeof(server_addr)) < 0){
		errorhandler("bind() fallito.\n");
		closesocket(my_socket);
		clearwinsock();
		return -1;
	}

	//SETTAGGIO DELLA SOCKET ALL'ASCOLTO
	if(listen (my_socket, QUEUE_SIZE) < 0){
		errorhandler("listen() fallito.\n");
		closesocket(my_socket);
		clearwinsock();
		return -1;
	}

	//ACCETTARE UNA NUOVA CONNESSIONE

	printf("Server in ascolto sulla porta %d...\n", port);
	while (1) {
		struct sockaddr_in cad; //struttura per l'indirizzo del client
		int client_socket;
		int client_len;
		client_len = sizeof(cad);
		if((client_socket = accept(my_socket, (struct sockaddr*) &cad, &client_len)) < 0){
			errorhandler("accept() fallito.\n");
			closesocket(client_socket);
			clearwinsock();
			return 0;
		}

		//RICEZIONE DELLA RICHIESTA DEL CLIENT
		weather_request_t req;
		int bytes_rcvd;
		if((bytes_rcvd = recv(client_socket, (char*)&req, sizeof(weather_request_t), 0)) <= 0){
			errorhandler("recv(req) fallito o connessione chiusa prematuramente.\n");
			closesocket(client_socket);
			clearwinsock();
			return -1;
		}
		printf("Richiesta '%c %s' dal client ip %s\n", req.type, req.city, inet_ntoa(cad.sin_addr));

		//VALIDAZIONE RICHIESTA
		weather_response_t res;
		res.status = checkResponse(&req);

		//COSTRUZIONE RISPOSTA
		switch(res.status){
			case 0:{
				float value = set_value(req.type);
				res.type = req.type;
				res.value = value;
				break;
			}
			case 1:{
				res.type = '\0';
				res.value = 0.0;
				break;
			}
			case 2:{
				res.type = '\0';
				res.value = 0.0;
				break;
			}
			default:
				printf("Status errato\n");
		}

		//INVIO RISPOSTA
		if(send(client_socket,(char*)&res, sizeof(weather_response_t), 0) != sizeof(weather_response_t)){
			errorhandler("send(res) ha inviato un numero di bytes diverso da quanti ne erano attesi.\n");
			closesocket(client_socket);
			clearwinsock();
			return -1;
		}



		//CHIUSURA CONNESSIONE CLIENT
		closesocket(client_socket);
	}

	printf("Server terminato.\n");
	//CHIUSURA SOKET DI BENVENUTO
	closesocket(my_socket);
	clearwinsock();
	return 0;
} // main end
