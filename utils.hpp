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

//TODO: sa vad daca includ id in structura sau fac un
//unordered_map<string, subscriber> clients
typedef struct {
    string id;
    int sockfd;
    bool connected;
    vector<string> topics;
} subscriber;