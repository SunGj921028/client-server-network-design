#include "ThreadPool.hpp"
#include "server.hpp"
#include "UserManager.hpp"
#include "defAndFuc.hpp"
#include <iostream>
#include <unistd.h>
#include <cstring>
#include <sys/socket.h>
#include <errno.h>     // For errno
#include <string>    // For strerror
#include <sstream>
#include <unordered_map>

char clientMsgBuffer[BUFFER_SIZE] = {0};
char clientMsgBufferForSelection[BUFFER_SIZE] = {0};

// 定義一組顏色代碼
const vector<string> colors = {
    "\033[31m", // 紅色
    "\033[32m", // 綠色
    "\033[33m", // 黃色
    "\033[34m", // 藍色
    "\033[35m", // 紫色
    "\033[36m",  // 青色
};

// Add mutex for color map
pthread_mutex_t colorMapMutex = PTHREAD_MUTEX_INITIALIZER;
unordered_map<int, string> clientColorMp;
string getColor(int socketNum) {
    pthread_mutex_lock(&colorMapMutex);
    if(clientColorMp.find(socketNum) == clientColorMp.end()){ clientColorMp[socketNum] = colors[socketNum % colors.size()]; }
    pthread_mutex_unlock(&colorMapMutex);
    return clientColorMp[socketNum];
}

// Worker Thread 函式
void* ThreadPool::workerThread(void* arg) {
    ThreadPool* pool = static_cast<ThreadPool*>(arg);

    while (true) {
        int clientSocket = -1;

        // Get a job from the queue
        pthread_mutex_lock(&pool->queueMutex);
        while (pool->jobQueue.empty() && pool->isRunning) {
            pthread_cond_wait(&pool->jobAvailable, &pool->queueMutex);
        }

        // Check if we should exit
        if (!pool->isRunning && pool->jobQueue.empty()) {
            pthread_mutex_unlock(&pool->queueMutex);
            break;
        }

        // Get the job if there is one
        if (!pool->jobQueue.empty()) {
            clientSocket = pool->jobQueue.front();
            pool->jobQueue.pop();
        }
        pthread_mutex_unlock(&pool->queueMutex);

        // Process the job if we got one
        if (clientSocket != -1) {
            handleClient(clientSocket, pool->server);
        }
    }

    return nullptr;
}

// 處理客戶端邏輯
void ThreadPool::handleClient(int clientSocket, Server* server) {
    cout << getColor(clientSocket) << "New client connected. (Socket: " << clientSocket << ")" << "\033[0m" << endl;

    while (1) {
        memset(clientMsgBuffer, 0, sizeof(clientMsgBuffer));
        ssize_t bytesRead = recv(clientSocket, clientMsgBuffer, sizeof(clientMsgBuffer), 0); //! Keep reading the next message or command from client
        
        // Handle disconnection or errors
        if (bytesRead <= 0) {
            if (bytesRead == 0) {
                // Client closed connection normally
                dprintf(STDERR_FILENO, "Client %d disconnected gracefully.\n", clientSocket);
            } else {
                // Error occurred
                dprintf(STDERR_FILENO, "Error reading from client %d: %s\n", clientSocket, strerror(errno));
            }
            break;
        }

        string receivedMsg(clientMsgBuffer);
        if(receivedMsg.size() >= BUFFER_SIZE){
            dprintf(STDERR_FILENO, "Client %d sent a message longer than the buffer size.\n", clientSocket);
            continue;
        }
        cout << getColor(clientSocket) << "<Client " << clientSocket << ">: " << "\033[0m" << receivedMsg << endl;

        // Parse command
        istringstream iss(receivedMsg);
        string command;
        iss >> command;

        toLowerCase(command);

        string response;
        string username = "", password = "";

        bool msgSelection = false;

        if (command == "register") {
            iss >> username >> password;

            if (username.empty() || password.empty() || iss.peek() != EOF) { //? Check input parameters
                response = invalidRegister;
            }else if(userManager.isLoggedIn(clientSocket)){
                response = logoutToRegister;
            } else if(username.size() > MAX_USERNAME_LENGTH || password.size() > MAX_PASSWORD_LENGTH){
                response = nameToolong;
            }else if (userManager.registerUser(username, password)) {
                ostringstream oss;
                oss << "Registration successful! Account " << username << " created!";
                response = oss.str();
                userManager.saveUsers(username, password);
            } else {
                response = userNameExist;
            }

        }else if (command == "login") {
            iss >> username >> password;
            if (username.empty() || password.empty() || iss.peek() != EOF) { // ! 長度檢查
                response = "Usage: login <username> <password>";
            }else if(username.size() > MAX_USERNAME_LENGTH || password.size() > MAX_PASSWORD_LENGTH){
                response = nameToolong;
            }else if(userManager.isLoggedIn(clientSocket)){
                response = alreadyLogin;
            }else if(!userManager.isAccountExist(username)){
                response = accNotFound;
            }else if(!userManager.isAccountAvailable(username)){
                response = accountNotAvailable;
            }else if (userManager.loginUser(clientSocket, username, password)) {
                ostringstream oss;
                oss << "Login successful! Welcome " << username << " to the server!";
                response = oss.str();
                // userManager.printLoginUserList();
            } else {
                response = incorrectPwd;
            }

        }else if (command == "logout") {
            if(receivedMsg.size() > 6){
                response = invalidInputFormat;
            }else if (userManager.isLoggedIn(clientSocket)) {
                userManager.logoutUser(clientSocket);
                response = logoutMsg;
            }else {
                response = notLogin;
            }
        }else if (command == "exit") {
            if(receivedMsg.size() > 4){
                response = invalidInputFormat;
            }else if (userManager.isLoggedIn(clientSocket)) {
                response = logoutFirst;
            } else {
                response = goodByeMsg;
                send(clientSocket, response.c_str(), response.length(), 0);
                break;
            }
        }else if (command == "send") {
            if(receivedMsg.size() > 4){
                response = sendCommandInvalid;
                send(clientSocket, response.c_str(), response.length(), 0);
                continue;
            }
            if (!userManager.isLoggedIn(clientSocket)) {
                response = notLogin;
            } else {
                //! First tell the client that the server is ready to receive the message mode selection
                msgSelection = true;
                response = "Please input the mode for sending message！";
                send(clientSocket, response.c_str(), response.length(), 0);

                //! To receive the message option selection（1 or 2） from client
                memset(clientMsgBufferForSelection, 0, sizeof(clientMsgBufferForSelection));
                ssize_t bytesReadforSelection = recv(clientSocket, clientMsgBufferForSelection, sizeof(clientMsgBufferForSelection), 0);
                string relayMsg = string(clientMsgBufferForSelection); // 1 or 2
                if(relayMsg.empty() || relayMsg.size() > 1 || (relayMsg != "1" && relayMsg != "2")  ){
                    response = invalidSendOption;
                    send(clientSocket, response.c_str(), response.length(), 0);
                    continue;
                }
                if(bytesReadforSelection > 0){
                    cout << getColor(clientSocket) << "<Client " << clientSocket << ">: " << "\033[0m" << "selected message sending mode: " << relayMsg << endl;
                }else{
                    cerr << "Receive failed." << endl;
                    continue;
                }

                if(relayMsg == "2"){
                    //! If only one client is online, then don't need to show the list
                    bool multiClientOnline = userManager.countOnlineClients() > 1;
                    if(multiClientOnline){
                        //! Send back a message to client to confirm the selection
                        ostringstream oss;
                        oss << "Request received. Please input your message！";
                        oss << userManager.printLoginUserList(clientSocket);
                        response = oss.str();
                    }else{
                        response = onlyOneClientOnline;
                    }
                    if(send(clientSocket, response.c_str(), response.length(), 0) < 0){
                        cerr << "Send failed." << endl;
                        continue;
                    }
                    if(!multiClientOnline){
                        continue;
                    }
                }else if (relayMsg == "1"){
                    response = "Request received. Please input your message！";
                    send(clientSocket, response.c_str(), response.length(), 0);
                }

                //! Dealing with the message in mode 1 and 2
                char actualMsgBuffer[BUFFER_SIZE] = {0};
                memset(actualMsgBuffer, 0, sizeof(actualMsgBuffer));
                ssize_t actuallBytesRead = recv(clientSocket, actualMsgBuffer, sizeof(actualMsgBuffer), 0);
                if(actuallBytesRead <= 0){
                    cerr << "Receive failed." << endl;
                    continue;
                }
                string actualMsg(actualMsgBuffer);
                cout << getColor(clientSocket) << "<Client " << clientSocket << ">: " << "\033[0m" << actualMsg << endl;
                istringstream iss(actualMsg);
                string cmd = "";
                iss >> cmd; // send
                toLowerCase(cmd);
                if(cmd != "send"){
                    response = invalidSendMsg;
                    send(clientSocket, response.c_str(), response.length(), 0);
                    continue;
                }
                if(relayMsg == "1"){
                    //? Mode 1: Send normal message to server
                    string message = "";
                    getline(iss >> ws, message);
                    if(message.empty()){
                        response = emptyMsg;
                    }else if(message.size() > BUFFER_SIZE){
                        response = msgToolong;
                    }else{
                        response = "(Client " + to_string(clientSocket) + ")" + username + ": " + message;
                    }
                    if(send(clientSocket, response.c_str(), response.length(), 0) < 0){
                        cerr << "Send failed." << endl;
                        continue;
                    }
                }else if(relayMsg == "2"){
                    //? Mode 2: Send message to specific client
                    string targetUsername = "", message = "";
                    iss >> targetUsername;
                    getline(iss >> ws, message);
                    if(targetUsername.empty()){
                        response = invalidSendMsg;
                        send(clientSocket, response.c_str(), response.length(), 0);
                        continue;
                    }else if(message.empty()){
                        response = emptyMsg;
                        send(clientSocket, response.c_str(), response.length(), 0);
                        continue;
                    }else if(message.size() > BUFFER_SIZE){
                        response = msgToolong;
                        send(clientSocket, response.c_str(), response.length(), 0);
                        continue;
                    }else if(targetUsername.size() > MAX_USERNAME_LENGTH){
                        response = nameToolong;
                        send(clientSocket, response.c_str(), response.length(), 0);
                        continue;
                    }
                    int targetClientSocket = userManager.getClientSocketNumFromStatus(targetUsername);
                    if(targetClientSocket == -1){
                        //! Send the message to the client who sent the message
                        response = "[Server]: Can't find the target client.";
                        send(clientSocket, response.c_str(), response.length(), 0);
                        continue;
                    }else{
                        //! Send the message to the client who sent the message
                        response = "(Client " + to_string(clientSocket) + ")" + username + " sent message to " + targetUsername + ": " + message;
                        send(clientSocket, response.c_str(), response.length(), 0);
                        //! Send message to target client
                        string targetResponse = "[Client_" + to_string(clientSocket) + " " + userManager.getClientUsername(clientSocket) + "]: " + message;
                        send(targetClientSocket, targetResponse.c_str(), targetResponse.length(), 0);
                    }
                }
            }
        }else {
            response = invalidCommand;
        }

        if(!msgSelection){
            if (send(clientSocket, response.c_str(), response.length(), 0) < 0) {
                dprintf(STDERR_FILENO, "Error sending response to client %d: %s\n", clientSocket, strerror(errno));
                break;
            }
        }
    }

    // Cleanup
    if (userManager.isLoggedIn(clientSocket)) {
        userManager.logoutUser(clientSocket);
    }
    // shutdown(clientSocket, SHUT_RDWR);
    close(clientSocket);
    cout << getColor(clientSocket) << "Client on socket " << clientSocket << " cleaned up and disconnected." << "\033[0m" << endl;

    // After cleanup, notify server about disconnection
    server->decrementClientCount();
}

// 建構函式
ThreadPool::ThreadPool(int numWorkers, Server* serverPtr) : server(serverPtr), isRunning(true) {
    pthread_mutex_init(&queueMutex, nullptr);
    pthread_cond_init(&jobAvailable, nullptr);

    // Create worker threads
    for (int i = 0; i < numWorkers; i++) {
        pthread_t thread;
        if (pthread_create(&thread, nullptr, workerThread, this) == 0) {
            workers.push_back(thread);
        }
    }
}

// 解構函式
ThreadPool::~ThreadPool() {
    shutdown();
    pthread_mutex_destroy(&queueMutex);
    pthread_cond_destroy(&jobAvailable);
    pthread_mutex_destroy(&colorMapMutex);
}

// 新增任務
void ThreadPool::addJob(int clientSocket) {
    pthread_mutex_lock(&queueMutex);
    if (isRunning) {  // Only add jobs if the thread pool is still running
        jobQueue.push(clientSocket);
        pthread_cond_signal(&jobAvailable);
    } else {
        close(clientSocket);  // If thread pool is shutting down, reject new connections
    }
    pthread_mutex_unlock(&queueMutex);
}

// Add shutdown method
void ThreadPool::shutdown() {
    pthread_mutex_lock(&queueMutex);
    isRunning = false;
    pthread_cond_broadcast(&jobAvailable);  // Wake up all waiting threads
    pthread_mutex_unlock(&queueMutex);

    // Wait for all threads to finish
    for (pthread_t worker : workers) {
        pthread_join(worker, nullptr);
    }
    workers.clear();

    // Clear remaining jobs
    while (!jobQueue.empty()) {
        int clientSocket = jobQueue.front();
        jobQueue.pop();
        close(clientSocket);  // Close any remaining client connections
    }
}
