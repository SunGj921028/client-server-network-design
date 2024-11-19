#ifndef CLIENT_HPP
#define CLIENT_HPP

#include<iostream>
#include<string>
#include<cstring>
#include<unistd.h>

#include<netinet/in.h>
#include<arpa/inet.h> // sockaddr 相關, inet_addr(), inet_pton()
#include<sys/socket.h> // socket 相關
#include<pthread.h> // Multi-threading server
#include<csignal>

#include"defAndFuc.hpp"

using namespace std;

class Client {
    public:
        Client(char * serverIP, int port);
        void connectTheServer(int port);
        // ~Client();
        // int getClientSocketFD() const {
        //     return clientSocketFD;
        // };
        // struct sockaddr_in getClientAddr() const {
        //     return clientADDR;
        // };

    private:
        struct sockaddr_in clientADDR;
        int clientSocketFD;
};

char msgBufferClient[BUFFER_SIZE];

#endif