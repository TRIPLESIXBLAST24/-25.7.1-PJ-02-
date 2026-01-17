#ifndef USER_H
#define USER_H

#include <string>
#include <vector>
#include <memory>

using namespace std;

class User {
private:
    string login;
    string password;
    string name;
    vector<string> friends;
    bool isOnline;

public:
    User(const string& login, const string& password, const string& name);
    
    const string& getLogin() const;
    const string& getName() const;
    const vector<string>& getFriends() const;
    bool getOnlineStatus() const;
    
    void setOnlineStatus(bool status);
    void addFriend(const string& friendLogin);
    void removeFriend(const string& friendLogin);

    bool checkPassword(const string& password) const;
    void changePassword(const string& oldPassword, const string& newPassword);
    
    bool hasFriend(const string& friendLogin) const;
    size_t getFriendCount() const;
};

#endif 