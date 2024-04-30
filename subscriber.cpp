#include <bits/stdc++.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <poll.h>
#include "utils.hpp"

using namespace std;

#define DEBUG false

int sockfd, ret;
struct sockaddr_in serv_addr;
char buffer[BUFSIZE];

struct pollfd pfds[2];
int nfds = 0;
int running = 1;

void debug_printf(const char *format, ...) {
    if (DEBUG) {
        printf("DEBUG: ");
        va_list args;
        va_start(args, format);
        vprintf(format, args);
        va_end(args);
    }
}

void handle_stdin() {
    fgets(buffer, BUFSIZE - 1, stdin);

    if (strncmp(buffer, "exit", 4) == 0) {
        ret = send(sockfd, buffer, strlen(buffer), 0);
        DIE(ret < 0, "send");

        running = 0;
        return;
    }

    buffer[strlen(buffer) - 1] = '\0';
    debug_printf("Command: %s, %d\n", buffer, strlen(buffer));
    
    ret = send(sockfd, buffer, strlen(buffer), 0);
    DIE(ret < 0, "send");

    //TODO: sa rescriu cu find si aici si in server.cpp
    string action, topic, buffer_str(buffer);

    size_t pos = buffer_str.find(" ");
    if (pos == string::npos) {
        debug_printf("Invalid command\n");
        return;
    }

    action = buffer_str.substr(0, pos);

    size_t start = buffer_str.find_first_not_of(" ", pos);
    if (start == string::npos) {
        debug_printf("Invalid command\n");
        return;
    }

    pos = buffer_str.find_first_of(" \n", start);
    topic = buffer_str.substr(start, pos - start);

    if (action == "subscribe") {
        printf("Subscribed to topic %s\n", topic.c_str());
    } else if (action == "unsubscribe") {
        printf("Unsubscribed from topic %s\n", topic.c_str());
    } else {
        debug_printf("Invalid command\n");
    }
}

void handle_tcp() {
    //TODO: mai intai sa citesc structura cu marimea si dupa payload
    int size;
    ret = recv(sockfd, &size, sizeof(int), 0);
    DIE(ret < 0, "recv");

    ret = recv(sockfd, buffer, size, 0);
    DIE(ret < 0, "recv");

    if (strcmp(buffer, "exit") == 0 || ret == 0) {
        running = 0;
        return;
    }

    tcp_message *msg = (tcp_message *)buffer;
    
    char output[2000];
    //"<IP_CLIENT_UDP>:<PORT_CLIENT_UDP> - <TOPIC> - "
    sprintf(output, "%s:%i - %s - ", inet_ntoa(msg->udp_addr.sin_addr), ntohs(msg->udp_addr.sin_port), msg->topic);

    //"<TIP_DATE> - <VALOARE_MESAJ>"
    switch (msg->type) {
        //INT
        case 0:
            uint8_t sign_int;
            int value_int;
            sign_int = *(uint8_t *)msg->payload;
            value_int = ntohl(*(uint32_t *)(msg->payload + 1));
            
            if (sign_int)
                value_int = -value_int;

            sprintf(output + strlen(output), "INT - %d\n", value_int);
            break;
        //SHORT_REAL
        case 1:
            float value_short_real;
            value_short_real = (float)ntohs(*(uint16_t *)msg->payload) / 100.0;

            sprintf(output + strlen(output), "SHORT_REAL - %.2f\n", value_short_real);
            break;
        //FLOAT
        case 2:
            float value_float;
            uint8_t sign_float, power;
            sign_float = *(uint8_t *)msg->payload;
            value_float = ntohl(*(uint32_t *)(msg->payload + 1));
            power = *(uint8_t *)(msg->payload + 5);

            value_float /= pow(10, power);

            if (sign_float)
                value_float = -value_float;

            sprintf(output + strlen(output), "FLOAT - %.4f\n", value_float);
            break;
        //STRING
        case 3:
            sprintf(output + strlen(output), "STRING - %s\n", msg->payload);
            break;
    }

    printf("%s", output);
}

int main(int argc, char *argv[]) {
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);

    if (argc < 4) {
        printf("%s <ID_CLIENT> <IP_SERVER> <PORT_SERVER>\n", argv[0]);
        exit(0);
    }

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    DIE(sockfd < 0, "socket");

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(atoi(argv[3]));
    ret = inet_aton(argv[2], &serv_addr.sin_addr);
    DIE(ret == 0, "inet_aton");

    ret = connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    DIE(ret < 0, "connect");

    int flag = 1;
    setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(int));

    ret = send(sockfd, argv[1], strlen(argv[1]) + 1, 0);
    DIE(ret < 0, "send");

    pfds[nfds].fd = STDIN_FILENO;
    pfds[nfds].events = POLLIN;
    nfds++;

    pfds[nfds].fd = sockfd;
    pfds[nfds].events = POLLIN;
    nfds++;

    while (running) {
        poll(pfds, nfds, -1);

        memset(buffer, 0 , BUFSIZE);

        //stdin
        if ((pfds[0].revents & POLLIN) != 0) {
            handle_stdin();
        }
        //tcp
        else if ((pfds[1].revents & POLLIN) != 0) {
            handle_tcp();
        }
    }

    close(sockfd);

    return 0;
}