#include <bits/stdc++.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <poll.h>
#include <regex.h>
#include "utils.hpp"

using namespace std;

#define DEBUG false

int sockfd_udp, sockfd_tcp, port, ret;
struct sockaddr_in addr_udp, addr_tcp, addr_new;
socklen_t udp_socklen = sizeof(addr_udp);
socklen_t tcp_socklen = sizeof(addr_tcp);
socklen_t new_socklen = sizeof(addr_new);
char buffer[BUFSIZE];

struct pollfd pfds[MAX_SUBS + 2];
int nfds = 0;
int running = 1;

vector<subscriber> clients;

void debug_printf(const char *format, ...) {
    if (DEBUG) {
        printf("DEBUG: ");
        va_list args;
        va_start(args, format);
        vprintf(format, args);
        va_end(args);
    }
}

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
    //SO_REUSEADDR pt a putea folosi acelasi port
    setsockopt(sockfd_tcp, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(int));

    memset((char *) &addr_tcp, 0, sizeof(addr_tcp));
    addr_tcp.sin_family = AF_INET;
    addr_tcp.sin_addr.s_addr = INADDR_ANY;
    addr_tcp.sin_port = htons(port);
    ret = bind(sockfd_tcp, (struct sockaddr *) &addr_tcp, sizeof(addr_tcp));
    DIE(ret < 0, "bind");

    ret = listen(sockfd_tcp, MAX_SUBS);
    DIE(ret < 0, "listen");
}

regex topic_to_regex(string topic) {
    // "*" face match cu orice
    size_t pos = topic.find("*");
    while (pos != string::npos) {
        topic.replace(pos, 1, ".*");
        pos = topic.find("*", pos + 2);
    }

    // "+" face match cu orice in afara de "/"
    pos = topic.find("+");
    while (pos != string::npos) {
        topic.replace(pos, 1, "[^/]*");
        pos = topic.find("+", pos + 4);
    }

    return regex(topic);
}

//pt cand verific daca un client e deja abonat la un topic
bool is_subscribed_topic(subscriber sub, string topic) {
    for (size_t i = 0; i < sub.topics.size(); i++) {
        if (sub.topics[i].first == topic) {
            return true;
        }
    }

    return false;
}

//pt cand verific daca trb sa trimit mesajul unui client
bool is_subscribed_regex(subscriber sub, string topic) {
    for (size_t i = 0; i < sub.topics.size(); i++) {
        if (regex_match(topic, sub.topics[i].second)) {
            return true;
        }
    }

    return false;
}

subscriber *get_subscriber_by_fd(int sockfd) {
    for (size_t i = 0; i < clients.size(); i++) {
        if (clients[i].sockfd == sockfd) {
            return &clients[i];
        }
    }

    return NULL;
}

//========================================================

void handle_stdin() {
    fgets(buffer, BUFSIZE, stdin);

    if (strncmp(buffer, "exit", 4) == 0) {
        for (size_t i = 0; i < clients.size(); i++) {
            //TODO: trimit exit la toti clientii (optional)
        }
        running = 0;
    } else {
        debug_printf("Invalid command\n");
    }
}

void handle_udp_message() {
    memset(buffer, 0, BUFSIZE);

    ret = recvfrom(sockfd_udp, buffer, BUFSIZE, 0, (struct sockaddr *) &addr_udp, &udp_socklen);
    DIE(ret < 0, "recvfrom");

    udp_message *msg = (udp_message *) buffer;

    //mai intai sa trimit marimea, abia dupa structura cu payload trunchiat
    int size = sizeof(struct sockaddr_in) + 51 + sizeof(uint8_t);
    if (msg->type == 0) size += 1 + sizeof(uint32_t);
    else if (msg->type == 1) size += sizeof(uint16_t);
    else if (msg->type == 2) size += 1 + sizeof(uint32_t) + sizeof(uint8_t);
    else if (msg->type == 3) {
        size += strlen(msg->payload) + 1;
        msg->payload[strlen(msg->payload)] = '\0';
    }

    tcp_message tcp_msg;
    memset(&tcp_msg, 0, sizeof(tcp_message));

    tcp_msg.udp_addr = addr_udp;
    memcpy(tcp_msg.topic, msg->topic, 50);
    tcp_msg.type = msg->type;

    memcpy(tcp_msg.payload, msg->payload, size - sizeof(struct sockaddr_in) - 51 - sizeof(uint8_t));

    string topic(msg->topic);

    for (size_t i = 0; i < clients.size(); i++) {
        if (!clients[i].connected) {
            continue;
        }

        if (is_subscribed_regex(clients[i], topic)) {
            ret = send(clients[i].sockfd, &size, sizeof(int), 0);
            DIE(ret < 0, "send");

            ret = send(clients[i].sockfd, &tcp_msg, size, 0);
            DIE(ret < 0, "send");
        }
    }
}

void handle_tcp_request() {
    memset(buffer, 0, BUFSIZE);

    int newsockfd = accept(sockfd_tcp, (sockaddr *) &addr_tcp, &tcp_socklen);
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
            printf("Client %s already connected.\n", id.c_str());

            //TODO: trimite mesaj exit catre client
            close(newsockfd);
            return;
        }

        clients[i].connected = true;
        clients[i].sockfd = newsockfd;

        printf("New client %s connected from %s:%hu.\n", id.c_str(), inet_ntoa(addr_tcp.sin_addr), ntohs(addr_tcp.sin_port));


        debug_printf("Client %s reconnected\n", id.c_str());
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
    sub.topics = vector<pair<string, regex>>();

    clients.push_back(sub);

    printf("New client %s connected from %s:%hu.\n", id.c_str(), inet_ntoa(addr_tcp.sin_addr), ntohs(addr_tcp.sin_port));
}

void handle_tcp_message(int sockfd) {
    //TODO: cazuri cu subscribe, unsubscribe, exit
    memset(buffer, 0, BUFSIZE);

    ret = recv(sockfd, buffer, BUFSIZE, 0);
    DIE(ret < 0, "recv");

    subscriber *sub = get_subscriber_by_fd(sockfd);
    
    if (ret == 0) {
        if (sub != NULL) {
            sub->connected = false;
            sub->sockfd = -1;

            close(sockfd);
            printf("Client %s disconnected.\n", sub->id.c_str());
        }
        return;
    }

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
        if (is_subscribed_topic(*sub, topic)) {
            debug_printf("Client %s already subscribed to topic %s\n", sub->id.c_str(), topic.c_str());
            return;
        }
        sub->topics.push_back(make_pair(topic, topic_to_regex(topic)));
        debug_printf("Client %s subscribed to topic %s\n", sub->id.c_str(), topic.c_str());
    } else if (action == "unsubscribe") {
        for (size_t i = 0; i < sub->topics.size(); i++) {
            if (sub->topics[i].first == topic) {
                sub->topics.erase(sub->topics.begin() + i);
                debug_printf("Client %s unsubscribed from topic %s\n", sub->id.c_str(), topic);
            }
        }
    } else {
        debug_printf("Invalid command\n");
    }
}

void close_sockets() {
    close(sockfd_udp);
    close(sockfd_tcp);

    for (size_t i = 0; i < clients.size(); i++) {
        close(clients[i].sockfd);
    }
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
            for (int i = 3; i < nfds; i++) {
                if ((pfds[i].revents & POLLIN) != 0) {
                    handle_tcp_message(pfds[i].fd);
                }
            }
        }
    }

    close_sockets();
}