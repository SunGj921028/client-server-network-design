#ifndef USER_HPP
#define USER_HPP

#include <string>

using namespace std;

class User {
    private:
        string username;
        string password;
        bool isLoggedIn;

    public:
        User(const string& username, const string& password);
        const string& getUsername() const;
        const string& getPassword() const;
        bool checkPassword(const string& password) const;
        bool getLoginStatus() const;
        void setLoginStatus(bool status);
};

#endif
