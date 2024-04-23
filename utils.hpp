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
#define MAX_SUBS 32

//TODO: sa vad daca includ id in structura sau fac un
//unordered_map<string, subscriber> clients
typedef struct {
    string id;
    int sockfd;
    bool connected;
    vector<string> topics;
} subscriber;

//pachet trimis de client udp catre server
typedef struct {
	char topic[50];
	uint8_t type;
	char payload[BUFSIZE];
} udp_message;

//pachet trimits de server catre client tcp
typedef struct {
	struct sockaddr_in udp_addr; //adresa udp care a trimis mesajul catre sv
	char topic[51];
	uint8_t type;
	char payload[BUFSIZE];
} tcp_message;