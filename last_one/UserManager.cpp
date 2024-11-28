#include "UserManager.hpp"
#include <fstream>
#include <iostream>
#include <cstring>
#include <sstream>

UserManager userManager;

UserManager::UserManager() {
    pthread_mutex_init(&mutex, nullptr);
    clientStatus.clear();
    loadUsers();
}

UserManager::~UserManager() {
    clientStatus.clear();
    pthread_mutex_destroy(&mutex);
}

void UserManager::loadUsers() {
    ifstream file(userFile);
    if (!file.is_open()) return;

    string username, password;
    while (file >> username >> password) {
        users.emplace_back(username, password);
    }
    file.close();
}

void UserManager::saveUsers(string& username, string& password) {
    ofstream file(userFile, ios::app);
    if (!file.is_open()) return;
    file << username << " " << password << "\n";
    file.close();
}

bool UserManager::registerUser(const string& username, const string& password) {
    pthread_mutex_lock(&mutex);
    for (const auto& user : users) {
        if (user.getUsername() == username) { //? Account already exists
            pthread_mutex_unlock(&mutex);
            return false;
        }
    }
    users.emplace_back(username, password);
    pthread_mutex_unlock(&mutex);
    return true;
}

bool UserManager::loginUser(int clientSocket, const string& username, const string& password) {
    pthread_mutex_lock(&mutex);
    for (auto& user : users) {
        if (user.getUsername() == username) {
            if (user.getLoginStatus() || !user.checkPassword(password)) {
                pthread_mutex_unlock(&mutex);
                return false;
            }
            user.setLoginStatus(true);
            clientStatus[clientSocket] = username;
            // cout << "Debug: Added client " << clientSocket << " with username " << username << " to clientStatus" << endl;
            // cout << "Debug: clientStatus size is now " << clientStatus.size() << endl;
            pthread_mutex_unlock(&mutex);
            return true;
        }
    }
    pthread_mutex_unlock(&mutex);
    return false;
}

void UserManager::logoutUser(int clientSocket) {
    pthread_mutex_lock(&mutex);
    auto it = clientStatus.find(clientSocket);
    if (it != clientStatus.end()) {
        // cout << "Debug: Removing client " << clientSocket << " with username " << it->second << " from clientStatus" << endl;
        for (auto& user : users) {
            if (user.getUsername() == it->second) {
                user.setLoginStatus(false);
                break;
            }
        }
        clientStatus.erase(it); //! Remove the client from the login map
        // cout << "Debug: clientStatus size is now " << clientStatus.size() << endl;
    }
    pthread_mutex_unlock(&mutex);
}

bool UserManager::isLoggedIn(int clientSocket) const { //! Check if the client has logged in
    pthread_mutex_lock(&mutex);
    bool loggedIn = clientStatus.find(clientSocket) != clientStatus.end();
    pthread_mutex_unlock(&mutex);
    return loggedIn;
}

bool UserManager::isAccountAvailable(const string& username) const { //! Check if the account whether can be logged in
    pthread_mutex_lock(&mutex);
    for (const auto& user : users) {
        if (user.getUsername() == username && !user.getLoginStatus()) {
            pthread_mutex_unlock(&mutex);
            return true;
        }
    }
    pthread_mutex_unlock(&mutex);
    return false;
}

bool UserManager::isAccountExist(const string& username) const { //! Check if the account exists
    pthread_mutex_lock(&mutex);
    for (const auto& user : users) {
        if (user.getUsername() == username) {
            pthread_mutex_unlock(&mutex);
            return true;
        }
    }
    pthread_mutex_unlock(&mutex);
    return false;
}

string UserManager::getClientUsername(int clientSocket) const { //! Get the matching account name from logged-in accounts
    pthread_mutex_lock(&mutex);
    auto it = clientStatus.find(clientSocket);
    string username = (it != clientStatus.end()) ? it->second : "";
    pthread_mutex_unlock(&mutex);
    return username;
}

string UserManager::printLoginUserList(int clientSocket) const {
    pthread_mutex_lock(&mutex);
    ostringstream oss;
    if (clientStatus.empty()) {
        oss << "No clients are currently logged in." << endl;
    } else {
        oss << "\nBelow is the list of clients who are online except yourself:\n";  
        for (const auto& user : clientStatus) {
            if(user.first != clientSocket){
                oss << "    " << "( client socket: " << user.first << " , username: " << user.second << " )" << endl;
            }
        }
    }
    pthread_mutex_unlock(&mutex);
    return oss.str();
}

int UserManager::countOnlineClients() const {
    pthread_mutex_lock(&mutex);
    int count = clientStatus.size();
    pthread_mutex_unlock(&mutex);
    return count;
}

int UserManager::getClientSocketNumFromStatus(const string& username) const {
    pthread_mutex_lock(&mutex);
    for (const auto& user : clientStatus) {
        if (user.second == username) {
            pthread_mutex_unlock(&mutex);
            return user.first;
        }
    }
    pthread_mutex_unlock(&mutex);
    return -1;
}
