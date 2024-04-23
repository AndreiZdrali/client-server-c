#include <bits/stdc++.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <poll.h>
#include "utils.hpp"

using namespace std;

int sockfd, ret;
struct sockaddr_in serv_addr;
char buffer[BUFSIZE];

struct pollfd pfds[2];
int nfds = 0;
int running = 1;

void handle_stdin() {
    fgets(buffer, BUFSIZE - 1, stdin);

    if (strncmp(buffer, "exit", 4) == 0) {
        running = 0;
        return;
    }

    buffer[strlen(buffer) - 1] = '\0';
    
    ret = send(sockfd, buffer, strlen(buffer), 0);
    DIE(ret < 0, "send");

    char *action = NULL, *topic = NULL;
    char *p = strtok(buffer, " ");
    strcpy(action, p);
    if (p == NULL) {
        printf("Invalid command\n");
    }

    p = strtok(NULL, " \n");
    strcpy(topic, p);
    if (p == NULL) {
        printf("Invalid command\n");
    }

    if (strncmp(action, "subscribe", 9) == 0) {
        printf("Subscribed to topic %s\n", topic);
    } else if (strncmp(action, "unsubscribe", 11) == 0) {
        printf("Unsubscribed from topic %s\n", topic);
    } else {
        printf("Invalid command\n");
    }
}

void handle_tcp() {
    ret = recv(sockfd, buffer, sizeof(buffer), 0);
    DIE(ret < 0, "recv");

    if (strcmp(buffer, "exit") == 0) {
        running = 0;
        return;
    }

    tcp_message *msg = (tcp_message *)buffer;
    
    //"<IP_CLIENT_UDP>:<PORT_CLIENT_UDP> - <TOPIC> - "
    printf("%s:%i - %s - ", inet_ntoa(msg->udp_addr.sin_addr), ntohs(msg->udp_addr.sin_port), msg->topic);

    //"<TIP_DATE> - <VALOARE_MESAJ>"
    switch (msg->type) {
        //INT
        case 0:
            uint8_t sign_int;
            uint32_t value_int;
            sign_int = *(uint8_t *)msg->payload;
            value_int = ntohl(*(uint32_t *)(msg->payload + 1)); //FIXME: posibil fara ntohl
            
            if (sign_int)
                value_int = -value_int;

            printf("INT - %d\n", value_int);
            break;
        //SHORT_REAL
        case 1:
            float value_short_real;
            value_short_real = (float)ntohs(*(uint16_t *)msg->payload) / 100.0;

            printf("SHORT_REAL - %.2f\n", value_short_real);
            break;
        //FLOAT
        case 2:
            float value_float;
            uint8_t sign_float, power;
            sign_float = *(uint8_t *)msg->payload;
            value_float = ntohl(*(uint32_t *)(msg->payload + 1)); //FIXME: posibil fara ntohl
            power = *(uint8_t *)(msg->payload + 5);

            value_float /= pow(10, power);

            if (sign_float)
                value_float = -value_float;

            printf("FLOAT - %.4f\n", value_float);
            break;
        //STRING
        case 3:
            msg->payload[BUFSIZE] = '\0'; //FIXME: cred ca e inutil asta
            printf("STRING - %s\n", msg->payload);
            break;
    }
}

int main(int argc, char *argv[]) {
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);

    if (argc < 4) {
        printf("%s <ID_CLIENT> <IP_SERVER> <PORT_SERVER>", argv[0]);
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