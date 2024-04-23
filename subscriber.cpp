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

    char *action, *topic;
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

int main(int argc, char *argv[]) {
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);

    //TODO: verifica argc == 4

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
            ret = recv(sockfd, buffer, sizeof(buffer), 0);
            DIE(ret < 0, "recv");

            if (strcmp(buffer, "exit") == 0) {
                running = 0;
                continue;
            }

            tcp_message *msg = (tcp_message *)buffer;
            
            //"<IP_CLIENT_UDP>:<PORT_CLIENT_UDP> - <TOPIC> - "
            printf("%s:%i - %s - ", inet_ntoa(msg->udp_addr.sin_addr), ntohs(msg->udp_addr.sin_port), msg->topic);

            //"<TIP_DATE> - <VALOARE_MESAJ>"
            switch (msg->type) {
                //INT
                case 0:
                    uint8_t sign; uint32_t value;
                    sign = *(uint8_t *)msg->payload;
                    value = ntohl(*(uint32_t *)(msg->payload + 1)); //FIXME: posibil fara ntohl
                    
                    if (sign)
                        value = -value;

                    printf("INT - %d\n", value);
                    break;
                //SHORT_REAL
                case 1:
                    //TODO: AICI AM RAMAS
            }
        }
    }

    close(sockfd);

    return 0;
}