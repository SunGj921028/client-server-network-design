#include "User.hpp"

User::User(const string& username, const string& password) 
    : username(username), password(password), isLoggedIn(false) 
{}

const string& User::getUsername() const {
    return username;
}

const string& User::getPassword() const {
    return password;
}

bool User::checkPassword(const string& password) const {
    return this->password == password;
}

bool User::getLoginStatus() const {
    return isLoggedIn;
}

void User::setLoginStatus(bool status) {
    isLoggedIn = status;
}
