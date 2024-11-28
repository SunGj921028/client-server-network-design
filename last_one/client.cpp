#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sstream>
#include <pthread.h>

#include "defAndFuc.hpp"

using namespace std;

char clientMsgBuffer[BUFFER_SIZE] = {0};

bool isLoginReady = false;
bool validSending = true;
bool isSending = false;
bool messageReceived = false; // For waiting the server response
bool msgFromClient = false;
bool oneClientSending = false;

pthread_mutex_t mutexReceive = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condReceive = PTHREAD_COND_INITIALIZER;

void* receiveMessages(void* arg) {
    int clientSocket = *(int*)arg;
    char buffer[BUFFER_SIZE];
    while (true) {
        memset(buffer, 0, BUFFER_SIZE);
        ssize_t bytesRead = recv(clientSocket, buffer, BUFFER_SIZE, 0);
        if (bytesRead <= 0) {
            if (bytesRead == 0) {
                cout << "[Server]: Disconnected." << endl;
            } else {
                cerr << "Error receiving message from server." << endl;
            }
            close(clientSocket);
            exit(EXIT_FAILURE);
        }

        // 顯示來自伺服器的訊息
        string message(buffer);
        if(message.find("Client_") != string::npos){
            msgFromClient = true;
        }

        cout << "\r"; // Clear current line
        if(msgFromClient){
            cout << "\n" << message << endl;
        }else{
            cout << "\n[Server]: " << message << endl;
        }

        if(message != goodByeMsg){
            pthread_mutex_lock(&mutexReceive);
            messageReceived = true;
            pthread_cond_signal(&condReceive);
            pthread_mutex_unlock(&mutexReceive);
        }

        // Print the command prompt
        if(message == goodByeMsg){
            close(clientSocket);
            exit(EXIT_SUCCESS);
        }else if(message.find("Server is full") != string::npos){
            close(clientSocket);
            exit(EXIT_FAILURE);
        }else if(message == logoutMsg){
            isLoginReady = false;
        }else if(message.find("Login successful!") != string::npos){
            isLoginReady = true;
        }else if(message == sendCommandInvalid || 
                message.find("Request received") != string::npos || message == invalidCommand ||
                message.find("Receive failed.") != string::npos || message == invalidSendMsg || 
                message == invalidInputFormat || message == msgToolong || message == nameToolong){
            validSending = false;
        }else if(message == "Please input the mode for sending message！"){
            isSending = true;
            validSending = true;
        }else if(message == onlyOneClientOnline){
            oneClientSending = true;
        }
    }
    return nullptr;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <IP_address> <port_number>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char *serverIP = argv[1];
    int client_port = atoi(argv[2]);

    int clientSocket = 0;
    struct sockaddr_in serv_addr;

    if ((clientSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        cerr << "Socket creation error" << endl;
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(client_port);

    if (inet_pton(AF_INET, serverIP, &serv_addr.sin_addr) <= 0) {
        cerr << "Invalid address/ Address not supported" << endl;
        return -1;
    }

    // 與伺服器連線
    if (connect(clientSocket, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        cerr << "Connection Failed" << endl;
        return -1;
    }

    // Enable the receive thread after successful connection
    pthread_t receiveThread;
    if(pthread_create(&receiveThread, NULL, receiveMessages, (void*)&clientSocket) != 0) {
        cerr << "Failed to create receive thread." << endl;
        close(clientSocket);
        return -1;
    }
    pthread_detach(receiveThread); //! Auto release the thread

    while (1) {
        validSending = true;
        isSending = false;
        oneClientSending = false;

        pthread_mutex_lock(&mutexReceive);
        while(1){
            if(!messageReceived){
                pthread_cond_wait(&condReceive, &mutexReceive);
            }else{
                if(!isSending){
                    printHelp();
                    cout << "Enter command: " << flush;
                }
                messageReceived = false;
                break;
            }
        }
        pthread_mutex_unlock(&mutexReceive);
        
        string input;
        getline(cin, input);

        if (input.empty()) continue;

        // Send the command to server
        if(send(clientSocket, input.c_str(), input.length(), 0) < 0) {
            cerr << "Send failed." << endl;
            continue;
        }

        // if(!validSending){ cout << "A\n"; continue;}

        //* For sending movement
        if(input == "send"){
            oneClientSending = false;
            //! If not login, send the login command
            if(!isLoginReady){ continue;}
            
            //? Start to send the mode selection message to server
            string relayMsg = "";
            relayMsg = "";

            // if(!validSending){ cout << "B\n";continue;}

            pthread_mutex_lock(&mutexReceive);
            while(1){
                if(!messageReceived){
                    pthread_cond_wait(&condReceive, &mutexReceive);
                }else{
                    printMsgSelection();
                    messageReceived = false;
                    break;
                }
            }
            pthread_mutex_unlock(&mutexReceive);

            if(!isSending){continue;}

            getline(cin, relayMsg);

            if(send(clientSocket, relayMsg.c_str(), relayMsg.length(), 0) < 0) {
                cerr << "Send failed." << endl;
                continue;
            }

            if((relayMsg != "1" && relayMsg != "2") || relayMsg.empty()){ continue;}

            //? Receive the response from server
            if(!validSending){continue;}

            // Send the actual message
            string message = "";
            pthread_mutex_lock(&mutexReceive);
            while(1){
                if(!messageReceived){
                    pthread_cond_wait(&condReceive, &mutexReceive);
                }else{
                    message = "";
                    if(relayMsg == "1") {
                        serverSendMsgSelection();
                        getline(cin, message);
                    } else if (relayMsg == "2") {
                        if(oneClientSending){break;}
                        clientSendMsgSelection();
                        getline(cin, message);
                    }
                    messageReceived = false;
                    break;
                }
            }
            pthread_mutex_unlock(&mutexReceive);
            if(oneClientSending){continue;}

            if(send(clientSocket, message.c_str(), message.length(), 0) < 0) {
                cerr << "Send failed." << endl;
                continue;
            }
        }
    }

    close(clientSocket);
    return 0;
}
