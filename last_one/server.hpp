#ifndef SERVER_HPP
#define SERVER_HPP

#include <iostream>
#include <queue>
#include <pthread.h>
#include <netinet/in.h>
#include <string>
#include <cstring>
#include <unistd.h>

#define MAX_CLIENTS 3
#define PORT 8080

using namespace std;

class ThreadPool; // 前向宣告

class Server {
    private:
        int serverFd;                     // 伺服器 socket 描述符
        struct sockaddr_in address;       // 伺服器地址

        int currentClients;               // 當前連線數
        ThreadPool* threadPool;           // ThreadPool 物件

        pthread_mutex_t clientCountMutex; // 用於保護 currentClients 的互斥鎖

        void handleNewConnection();       // 處理新連線的邏輯

    public:
        Server(int port);
        ~Server();
        void start(int port);              // 啟動伺服器
        void decrementClientCount();      // Called when a client disconnects
        // friend class ThreadPool;

};

#endif
