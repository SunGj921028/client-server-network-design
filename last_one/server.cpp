#include "server.hpp"
#include "ThreadPool.hpp"
#include "defAndFuc.hpp"

using namespace std;

int serverFdTmp;

// 信号处理函数
void handle_signal(int sig) {
    if (sig == SIGINT) {
        printf("\nServer shutting down...\n");
        closeSocket(serverFdTmp);
        exit(EXIT_FAILURE);
    }
}

// 建構函式
Server::Server(int port) 
    : serverFd(0), currentClients(0) {
    pthread_mutex_init(&clientCountMutex, nullptr);

    // 初始化伺服器地址
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);
    
    serverFdTmp = serverFd; //! For signal handler
    
    // Create ThreadPool with reference to this server（主執行緒）
    threadPool = new ThreadPool(10, this);
}

// 解構函式
Server::~Server() {
    if (threadPool) {
        threadPool->shutdown();  // Properly shutdown the thread pool
        delete threadPool;
    }
    closeSocket(serverFd);
    pthread_mutex_destroy(&clientCountMutex);
}

// 處理新連線
void Server::handleNewConnection() {
    int connSocketFD;
    struct sockaddr_in clientADDR;
    socklen_t addrlen = sizeof(clientADDR);

    if ((connSocketFD = accept(serverFd, (struct sockaddr*)&clientADDR, &addrlen)) < 0) {
        perror("Connection: Accept failed");
        return;
    }

    // 檢查連線數是否超過限制
    pthread_mutex_lock(&clientCountMutex);
    cout << "\n--- Origin number of clients: " << currentClients << endl;
    if (currentClients >= MAX_CLIENTS) {
        pthread_mutex_unlock(&clientCountMutex);
        send(connSocketFD, rejectMsg.c_str(), rejectMsg.size(), 0);
        closeSocket(connSocketFD);
        cout << "Rejected a connection: too many clients." << endl;
        return;
    }else{
        send(connSocketFD, sucMsg.c_str(), sucMsg.size(), 0);
    }

    currentClients++;
    cout << "--- Current number of clients: " << currentClients << endl;
    pthread_mutex_unlock(&clientCountMutex);


    threadPool->addJob(connSocketFD); //! 接受的連接會被放入任務佇列
}

// 啟動伺服器
void Server::start(int port) {
    // 建立 socket
    if ((serverFd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        closeSocket(serverFd);
        errorCaller("--- ERROR on server socket ---");
    }

    // 綁定地址
    if (bind(serverFd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        closeSocket(serverFd);
        errorCaller("--- ERROR on server binding ---");
    }

    // 開始監聽
    if (listen(serverFd, MAX_CLIENTS) < 0) {
        closeSocket(serverFd);
        errorCaller("--- ERROR on server listening ---");
    }
    cout << "Server listening on port " << port << endl;

    // 持續接受客戶端連線
    while (1) {
        handleNewConnection();
    }
}

// 減少當前連線數
void Server::decrementClientCount() {
    pthread_mutex_lock(&clientCountMutex);
    if (currentClients > 0) {
        currentClients--;
    }
    cout << "\n--- Current number of clients: " << currentClients << endl;
    pthread_mutex_unlock(&clientCountMutex);
}

int main(int argc, char *argv[]){
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    //? Get the input argv
    int portNumber = atoi(argv[1]);

    //? Calloc the space for server structure variable
    Server server(portNumber);

    //? Step of server initialization with socket and given port number
    signal(SIGINT, handle_signal);

    server.start(portNumber);
    
    return 0;
}