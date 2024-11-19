#ifndef SERVER_HPP
#define SERVER_HPP

#include<iostream>
#include<string>
#include<cstring>
#include<unordered_map>
#include<unistd.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<pthread.h>
#include<csignal>


#include"defAndFuc.hpp"

using namespace std;

class Server {
    public:
        Server(int port);
        // ~Server();
        int waitForConnecting();
        void proc_server(int port);
        int handleUserStatus(int connSocketFD);
        int handleClient(int connSocketFD);
        int getServerSocketFD() const {
            return serverSocketFD;
        };
        struct sockaddr_in getServerAddr() const {
            return serverADDR;
        };
    
    private:
        struct sockaddr_in serverADDR;
        int serverSocketFD;
};

char msgBufferServer[BUFFER_SIZE];

#endif