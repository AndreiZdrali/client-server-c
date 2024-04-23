#include <bits/stdc++.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <poll.h>
#include "utils.hpp"

using namespace std;

int sockfd_udp, sockfd_tcp, port, ret;
struct sockaddr_in addr_udp, addr_tcp;
socklen_t udp_socklen = sizeof(addr_udp);
char buffer[BUFSIZE];

struct pollfd pfds[MAX_SUBS + 2];
int nfds = 0;
int running = 1;

vector<subscriber> clients;

void init_udp_tcp() {
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
    setsockopt(sockfd_tcp, IPPROTO_TCP, TCP_NODELAY, (char *) &flag, sizeof(int));

    memset((char *) &addr_tcp, 0, sizeof(addr_tcp));
    addr_tcp.sin_family = AF_INET;
    addr_tcp.sin_addr.s_addr = INADDR_ANY;
    addr_tcp.sin_port = htons(port);
    ret = bind(sockfd_tcp, (struct sockaddr *) &addr_tcp, sizeof(addr_tcp));
    DIE(ret < 0, "bind");

    ret = listen(sockfd_tcp, MAX_SUBS);
    DIE(ret < 0, "listen");
}

bool is_subscribed(subscriber sub, string topic) {
    for (size_t i = 0; i < sub.topics.size(); i++) {
        //TODO: verificare topic cu wildcard
    }

    return false;
}

//========================================================

void handle_stdin() {
    fgets(buffer, BUFSIZE, stdin);

    if (strncmp(buffer, "exit", 4) == 0) {
        for (size_t i = 0; i < clients.size(); i++) {
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
    memset(buffer, 0, BUFSIZE);

    ret = recvfrom(sockfd_udp, buffer, BUFSIZE, 0, (struct sockaddr *) &addr_udp, &udp_socklen);
    DIE(ret < 0, "recvfrom");

    udp_message *msg = (udp_message *) buffer;

    tcp_message tcp_msg;
    memset(&tcp_msg, 0, sizeof(tcp_message));

    tcp_msg.udp_addr = addr_udp;
    strcpy(tcp_msg.topic, msg->topic);
    tcp_msg.type = msg->type;
    strcpy(tcp_msg.payload, msg->payload);

    string topic(msg->topic);

    for (size_t i = 0; i < clients.size(); i++) {
        if (!clients[i].connected) {
            continue;
        }

        if (is_subscribed(clients[i], topic)) {
            ret = send(clients[i].sockfd, &tcp_msg, sizeof(tcp_msg), 0);
            DIE(ret < 0, "send");
        }
    }
}

void handle_tcp_request() {
    memset(buffer, 0, BUFSIZE);

    int newsockfd = accept(sockfd_tcp, NULL, NULL);
    DIE(newsockfd < 0, "accept");

    //dezactiveaza alg lui nagle
    int flag = 1;
    setsockopt(newsockfd, IPPROTO_TCP, TCP_NODELAY, (char *) &flag, sizeof(int));

    ret = recv(newsockfd, buffer, BUFSIZE, 0);
    DIE(ret < 0, "recv");

    string id(buffer);

    for (size_t i = 0; i < clients.size(); i++) {
        //clientul este deja in lista
        if (clients[i].id != id) {
            continue;
        }

        if (clients[i].connected) {
            printf("Client %s already connected\n", id.c_str());

            //TODO: trimite mesaj exit catre client
            close(newsockfd);
            return;
        }

        clients[i].connected = true;
        clients[i].sockfd = newsockfd;

        printf("Client %s reconnected\n", id.c_str());
        return;
    }

    //clientul nu este in lista
    pfds[nfds].fd = newsockfd;
    pfds[nfds].events = POLLIN;
    nfds++;

    subscriber sub;
    sub.id = id;
    sub.connected = true;
    sub.sockfd = newsockfd;
    sub.topics = vector<string>();

    clients.push_back(sub);

    printf("New client %s connected\n", id.c_str());
}

void handle_tcp_message() {
    //exit(1);
}

int main(int argc, char *argv[]) {
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);

    if (argc < 2) {
        printf("%s <PORT>\n", argv[0]);
        exit(0);
    }

    port = atoi(argv[1]);

    //TODO: verificari port (> 1024, < 65536, != 0)

    init_udp_tcp();

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

    close(sockfd_udp);
    close(sockfd_tcp);
}