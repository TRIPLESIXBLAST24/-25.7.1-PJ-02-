#include "user.h"
#include <string>
#include <algorithm>
#include <stdexcept>

using namespace std;

User::User(const string& login, const string& password, const string& name)
    : login(login), password(password), name(name), isOnline(false) {}

const string& User::getLogin() const {
    return login;
}

const string& User::getName() const {
    return name;
}

const vector<string>& User::getFriends() const {
    return friends;
}

bool User::getOnlineStatus() const {
    return isOnline;
}

void User::setOnlineStatus(bool status) {
    isOnline = status;
}

void User::addFriend(const string& friendLogin) {
    if (friendLogin != login && !hasFriend(friendLogin)) {
        friends.push_back(friendLogin);
    }
}

void User::removeFriend(const string& friendLogin) {
    auto it = find(friends.begin(), friends.end(), friendLogin);
    if (it != friends.end()) {
        friends.erase(it);
    }
}

bool User::checkPassword(const string& password) const {
    return this->password == password;
}

void User::changePassword(const string& oldPassword, const string& newPassword) {
    if (checkPassword(oldPassword)) {
        password = newPassword;
    } else {
        throw runtime_error("Invalid old password");
    }
}

bool User::hasFriend(const string& friendLogin) const {
    return find(friends.begin(), friends.end(), friendLogin) != friends.end();
}

size_t User::getFriendCount() const {
    return friends.size();
} 