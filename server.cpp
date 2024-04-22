#include <bits/stdc++.h>
#include <sys/socket.h>
#include <netinet/in.h>

using namespace std;

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

int sockfd_udp, sockfd_tcp, port, ret;
struct sockaddr_in addr_udp, addr_tcp;
char buffer[BUFSIZE];

vector<subscriber> clients;


void start_udp_tcp() {
    int ret;
    sockfd_udp = socket(AF_INET, SOCK_DGRAM, 0);
    memset((char *) &addr_udp, 0, sizeof(addr_udp));
    addr_udp.sin_family = AF_INET;
    addr_udp.sin_addr.s_addr = INADDR_ANY;
    addr_udp.sin_port = htons(port);
    ret = bind(sockfd_udp, (struct sockaddr *) &addr_udp, sizeof(addr_udp));

    sockfd_tcp = socket(AF_INET, SOCK_STREAM, 0);
    memset((char *) &addr_tcp, 0, sizeof(addr_tcp));
    addr_tcp.sin_family = AF_INET;
    addr_tcp.sin_addr.s_addr = INADDR_ANY;
    addr_tcp.sin_port = htons(port);
    ret = bind(sockfd_tcp, (struct sockaddr *) &addr_tcp, sizeof(addr_tcp));
}

int main(int argc, char *argv[]) {
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);

    port = atoi(argv[1]);

    //TODO: verificari port (> 1024, < 65536, != 0)


    start_udp_tcp();

    //asteapta conexiuni tcp
    ret = listen(sockfd_tcp, MAX_SUBS);

}