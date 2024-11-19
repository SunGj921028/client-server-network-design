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

#define MAX_USERS 1000
#define MAX_CLIENTS 10

#define BUFFER_SIZE 1024

// In server.hpp, add these constants:
#define MAX_USERNAME_LENGTH 50
#define MAX_PASSWORD_LENGTH 50

using namespace std;

typedef struct UserList {
    unordered_map<string, string> users; //? For <username, password>
    int is_cur_logged_in;
    string curUser;
    string curPwd;
} UserList;

UserList userList;

const char* optionForClient = 
    "--- Hello, please input in the given format and word below (Don't input the number)!\n"
    "(1) register <username> <password>\n"
    "(2) login <username> <password>\n"
    "(3) exit\n"
    "--------------------------------------------------\n"
    ">>> Your choice: "
;

// const char* welcomeMsg = "Welcome !!\n";
const char* invalidRegister = "Invalid register input format.\n";
const char* invalidLogin = "Invalid login input format.\n";
const char* accNotFound = "Account not found, please register first.\n";
const char* incorrectPwd = "Incorrect password, please try again.\n";
const char* invalidCommand = "Invalid command, try again.\n";
const char* userNameExist = "Username already exists.\n";

const char* alreadyLogin = "You have already logged in.\n";
const char* notLogin = "You haven't logged in yet.\n";
const char* logoutMsg = "Logout successfuly.\n";
const char* logoutFirst = "Please logout first before exit.\n";
const char* emptyMsg = "Input something !!\n";

const char* goodByeMsg = "Goodbye !!\n";

const char* msgToolong = "Message too long.\n";
const char* nameToolong = "Username or password too long.\n";

const char* justSaySomething = 
    "Please input in the given format and word below (Don't input the number)!\n"
    "(1) send <message>\n"
    "(2) logout\n"
    "------------------------\n"
    "~ "
;

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

void saveUserToFile(const string& username, const string& password) {
    ofstream userFile("userInfo.txt", ios::app);  // Open file in append mode
    if(userFile.is_open()) {
        userFile << username << " " << password << "\n";  // 格式：username password
        userFile.close();
    }else{
        cerr << "Error opening file to save user information.\n";
    }
}

void loadUserFromFile() {
    ifstream userFile("userInfo.txt"); 
    if (!userFile.is_open()) {
        // cerr << "Error opening user info file.\n";
        return;
    }

    string line;
    while (getline(userFile, line)) {
        istringstream iss(line);
        string username, password;

        if (!(iss >> username >> password)) {
            // cerr << "Skipping malformed line in user info file: " << line << "\n";
            continue;
        }

        userList.users[username] = password;
    }

    userFile.close();
    // cout << "User information loaded successfully.\n";
}

#endif