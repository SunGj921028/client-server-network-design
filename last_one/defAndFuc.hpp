#ifndef SPECIALHEADER
#define SPECIALHEADER

#include<iostream>
#include<string>
#include<cstring>
#include<vector>
#include<unistd.h>
#include<unordered_map>
#include<fstream>
#include<sstream>
#include<csignal>
#include<netinet/in.h> // sockaddr_in 相關
#include<arpa/inet.h>   // sockaddr 相關, inet_addr(), inet_pton()
#include<sys/socket.h> // socket 相關

#define BUFFER_SIZE 1024

// In server.hpp, add these constants:
#define MAX_USERNAME_LENGTH 50
#define MAX_PASSWORD_LENGTH 50

using namespace std;

// Declare as extern to avoid multiple definition
// extern const char* optionForClient;
extern const string invalidRegister;
extern const string invalidLogin;
extern const string invalidCommand;
extern const string accountNotAvailable;
extern const string goodByeMsg;
extern const string msgToolong;
extern const string nameToolong;
extern const string alreadyLogin;
extern const string userNameExist;
extern const string logoutToRegister;
extern const string accNotFound;
extern const string incorrectPwd;
//! For multi-threading
extern const string rejectMsg;
extern const string sucMsg;
extern const string logoutMsg;
extern const string notLogin;
extern const string logoutFirst;
extern const string emptyMsg;

extern const string registerFailed;
extern const string onlyOneClientOnline;
extern const string sendCommandInvalid;
extern const string invalidSendOption;
extern const string invalidSendMsg;
extern const string invalidInputFormat;


// Function declarations
void printHelp();
void printMsgSelection();
void clientSendMsgSelection();
void serverSendMsgSelection();
void errorCaller(const char* msg);

void toLowerCase(string &str);
void closeSocket(int socketFD);
bool isValidIPAddress(char* ip);

#endif