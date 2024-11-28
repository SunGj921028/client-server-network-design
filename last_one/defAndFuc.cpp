#include "defAndFuc.hpp"

const string invalidRegister = "Usage: register <username> <password>";
const string invalidLogin = "Usage: login <username> <password>";
const string accountNotAvailable = "Account not available, please try another account.";
const string invalidCommand = "Invalid command, try again.";
const string goodByeMsg = "Goodbye !!";
const string msgToolong = "Message too long.";
const string nameToolong = "Username or password too long.";
const string alreadyLogin = "You have already logged in.";
const string userNameExist = "Username already exists.";
const string logoutToRegister = "Please logout first before register.";
const string accNotFound = "Account not found, please register first.";
const string incorrectPwd = "Incorrect password, please try again.";
const string logoutMsg = "Logout successfuly !!";
const string notLogin = "You haven't logged in yet.";
const string logoutFirst = "Please logout first before exit.";
const string emptyMsg = "Input something !!";

const string registerFailed = "Registration failed.";

const string rejectMsg = "Server is full. Please try again later.";
const string sucMsg = "Welcome to the server!";
const string onlyOneClientOnline = "You are the only client online.";
const string sendCommandInvalid = "Invalid input of send command. Please try again.";
const string invalidSendOption = "Invalid input of send option. Please try again.";
const string invalidSendMsg = "Invalid format of sending input message. Please try again.";

const string invalidInputFormat = "Invalid input format. Please try again.";

void printHelp() {
    cout << "\nAvailable commands:" << endl;
    cout << "1. register <username> <password>" << endl;
    cout << "2. login <username> <password>" << endl;
    cout << "3. logout" << endl;
    cout << "4. exit" << endl;
    cout << "5. send" << endl;
    cout << "-----------------------------------------\n";
}

void printMsgSelection(){
    cout << "\nPlease input the number to choose the mode for sending message:\n";
    cout << "1. send message to server\n";
    cout << "2. send message to specific client（with client's username）\n";
    cout << "-----------------------------------------------------------------\n";
    cout << ">>> ";
}

void clientSendMsgSelection(){
    cout << "\nInput format: send <client username> <message>\n";
    cout << "Your input: ";
}

void serverSendMsgSelection(){
    cout << "\nInput format: send <message>\n";
    cout << "Your input: ";
}

// Function implementations
void errorCaller(const char* msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

void toLowerCase(string &str) {
    for (int i = 0; i < str.size(); i++) {
        str[i] = tolower(str[i]);
    }
}

void closeSocket(int socketFD) {
    if (close(socketFD) < 0) {
        errorCaller("Close socket failed");
    }
}

bool isValidIPAddress(char* ip) {
    struct sockaddr_in test_ip_format;
    return inet_pton(AF_INET, ip, &(test_ip_format.sin_addr)) == 1;
}