#include <bits/stdc++.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <poll.h>
#include "utils.hpp"

using namespace std;

int sockfd_udp, sockfd_tcp, port, ret;
struct sockaddr_in addr_udp, addr_tcp;
char buffer[BUFSIZE];

struct pollfd pfds[MAX_SUBS + 2];
int nfds = 0;
int running = 1;

vector<subscriber> clients;

void start_udp_tcp() {
    //initializare socket udp
    sockfd_udp = socket(AF_INET, SOCK_DGRAM, 0);
    DIE(sockfd_udp < 0, "socket");

    memset((char *) &addr_udp, 0, sizeof(addr_udp));
    addr_udp.sin_family = AF_INET;
    addr_udp.sin_addr.s_addr = INADDR_ANY;
    addr_udp.sin_port = htons(port);
    ret = bind(sockfd_udp, (struct sockaddr *) &addr_udp, sizeof(addr_udp));
    DIE(ret < 0, "bind");

    //initializare socket tcp
    sockfd_tcp = socket(AF_INET, SOCK_STREAM, 0);
    DIE(sockfd_tcp < 0, "socket");

    //dezactiveaza alg lui nagle
    int flag = 1;
    ret = setsockopt(sockfd_tcp, IPPROTO_TCP, TCP_NODELAY, (char *) &flag, sizeof(int));
    DIE(ret < 0, "setsockopt");

    memset((char *) &addr_tcp, 0, sizeof(addr_tcp));
    addr_tcp.sin_family = AF_INET;
    addr_tcp.sin_addr.s_addr = INADDR_ANY;
    addr_tcp.sin_port = htons(port);
    ret = bind(sockfd_tcp, (struct sockaddr *) &addr_tcp, sizeof(addr_tcp));
    DIE(ret < 0, "bind");

    ret = listen(sockfd_tcp, MAX_SUBS);
    DIE(ret < 0, "listen");
}

void handle_stdin() {
    fgets(buffer, BUFSIZE, stdin);

    if (strncmp(buffer, "exit", 4) == 0) {
        for (int i = 0; i < clients.size(); i++) {
            //TODO: trimit exit la toti clientii
        }

        close(sockfd_udp);
        close(sockfd_tcp);
        running = 0;
    } else {
        //FIXME: nu stiu daca printf e conform cerintei
        printf("Invalid command\n");
    }
}

void handle_udp_message() {
    exit(1);
}

void handle_tcp_request() {
    //dezactiveaza alg lui nagle
    int flag = 1;
    ret = setsockopt(sockfd_tcp, IPPROTO_TCP, TCP_NODELAY, (char *) &flag, sizeof(int));
    DIE(ret < 0, "setsockopt");

    //TODO: verifica daca id e unic sau daca a mai fost folosit
    //exit(1);
}

void handle_tcp_message() {
    exit(1);
}

int main(int argc, char *argv[]) {
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);

    port = atoi(argv[1]);

    //TODO: verificari port (> 1024, < 65536, != 0)

    start_udp_tcp();

    pfds[nfds].fd = STDIN_FILENO;
    pfds[nfds].events = POLLIN;
    nfds++;

    pfds[nfds].fd = sockfd_udp;
    pfds[nfds].events = POLLIN;
    nfds++;

    pfds[nfds].fd = sockfd_tcp;
    pfds[nfds].events = POLLIN;
    nfds++;

    while (running) {
        poll(pfds, nfds, -1);

        //input de la stdin
        if ((pfds[0].revents & POLLIN) != 0) {
            handle_stdin();
        }
        //primeste mesaj udp
        else if ((pfds[1].revents & POLLIN) != 0) {
            handle_udp_message();
        }
        //primeste cerere de conexiune tcp
        else if ((pfds[2].revents & POLLIN) != 0) {
            handle_tcp_request();
        }
        //primeste mesaj de la un client
        else {
            handle_tcp_message();
        }
    }
}