#pragma once

#include <bits/stdc++.h>

using namespace std;

#define DIE(assertion, call_description)				\
	do {								\
		if (assertion) {					\
			fprintf(stderr, "(%s, %d): ",			\
					__FILE__, __LINE__);		\
			perror(call_description);			\
			exit(errno);					\
		}							\
	} while (0)

#define BUFSIZE 1600
#define PAYLOADSIZE 1501
#define MAX_SUBS 32

typedef struct {
    string id;
    int sockfd;
    bool connected;
    vector<pair<string, regex>> topics;
} subscriber;

//pachet trimis de client udp catre server
typedef struct {
	char topic[50];
	uint8_t type;
	char payload[PAYLOADSIZE];
} udp_message;

//pachet trimits de server catre client tcp
typedef struct {
	struct sockaddr_in udp_addr; //adresa udp care a trimis mesajul catre sv
	char topic[51];
	uint8_t type;
	char payload[PAYLOADSIZE];
} tcp_message;