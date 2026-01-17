#include "database.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iostream>

using namespace std;

Database::Database(const string& path) : dbPath(path) {
}

Database::~Database() {
}

string Database::getUsersFilePath() const {
    return dbPath + "/users.txt";
}

string Database::getMessagesFilePath() const {
    return dbPath + "/messages.txt";
}

bool Database::initialize() {
    #ifdef _WIN32
        system(("mkdir " + dbPath + " 2>nul").c_str());
    #else
        system(("mkdir -p " + dbPath + " 2>/dev/null").c_str());
    #endif
    
    ofstream usersFile(getUsersFilePath(), ios::app);
    ofstream messagesFile(getMessagesFilePath(), ios::app);
    
    return usersFile.good() && messagesFile.good();
}

string Database::escapeString(const string& s) const {
    string result;
    for (char c : s) {
        if (c == '|') {
            result += "\\|";
        } else if (c == '\\') {
            result += "\\\\";
        } else if (c == '\n') {
            result += "\\n";
        } else if (c == '\r') {
            result += "\\r";
        } else {
            result += c;
        }
    }
    return result;
}

string Database::unescapeString(const string& s) const {
    string result;
    for (size_t i = 0; i < s.length(); ++i) {
        if (s[i] == '\\' && i + 1 < s.length()) {
            if (s[i+1] == '|') {
                result += '|';
                ++i;
            } else if (s[i+1] == '\\') {
                result += '\\';
                ++i;
            } else if (s[i+1] == 'n') {
                result += '\n';
                ++i;
            } else if (s[i+1] == 'r') {
                result += '\r';
                ++i;
            } else {
                result += s[i];
            }
        } else {
            result += s[i];
        }
    }
    return result;
}

string Database::serializeUser(const UserData& user) const {
    ostringstream oss;
    oss << escapeString(user.login) << "|"
        << escapeString(user.password) << "|"
        << escapeString(user.name) << "|";
    
    for (size_t i = 0; i < user.friends.size(); ++i) {
        if (i > 0) oss << ",";
        oss << escapeString(user.friends[i]);
    }
    
    return oss.str();
}

UserData Database::deserializeUser(const string& line) const {
    UserData user;
    stringstream ss(line);
    string token;
    
    if (!getline(ss, token, '|')) return user;
    user.login = unescapeString(token);
    
    if (!getline(ss, token, '|')) return user;
    user.password = unescapeString(token);
    
    if (!getline(ss, token, '|')) return user;
    user.name = unescapeString(token);
    
    if (getline(ss, token, '|')) {
        stringstream friendsStream(token);
        string friendLogin;
        while (getline(friendsStream, friendLogin, ',')) {
            if (!friendLogin.empty()) {
                user.friends.push_back(unescapeString(friendLogin));
            }
        }
    }
    
    return user;
}

string Database::serializeMessage(const MessageData& msg) const {
    ostringstream oss;
    oss << escapeString(msg.senderLogin) << "|"
        << escapeString(msg.recipientLogin) << "|"
        << escapeString(msg.text) << "|"
        << escapeString(msg.type) << "|"
        << msg.timestamp << "|";
    
    for (size_t i = 0; i < msg.tags.size(); ++i) {
        if (i > 0) oss << ",";
        oss << escapeString(msg.tags[i]);
    }
    
    return oss.str();
}

MessageData Database::deserializeMessage(const string& line) const {
    MessageData msg;
    stringstream ss(line);
    string token;
    
    if (!getline(ss, token, '|')) return msg;
    msg.senderLogin = unescapeString(token);

    if (!getline(ss, token, '|')) return msg;
    msg.recipientLogin = unescapeString(token);
    
    if (!getline(ss, token, '|')) return msg;
    msg.text = unescapeString(token);
    
    if (!getline(ss, token, '|')) return msg;
    msg.type = unescapeString(token);
    
    if (!getline(ss, token, '|')) return msg;
    msg.timestamp = stoll(token);
    
    if (getline(ss, token, '|')) {
        stringstream tagsStream(token);
        string tag;
        while (getline(tagsStream, tag, ',')) {
            if (!tag.empty()) {
                msg.tags.push_back(unescapeString(tag));
            }
        }
    }
    
    return msg;
}

bool Database::addUser(const string& login, const string& password, const string& name) {
    if (userExists(login)) {
        return false;
    }
    
    UserData user;
    user.login = login;
    user.password = password;
    user.name = name;
    
    ofstream file(getUsersFilePath(), ios::app);
    if (!file.is_open()) return false;
    
    file << serializeUser(user) << "\n";
    return file.good();
}

bool Database::userExists(const string& login) const {
    ifstream file(getUsersFilePath());
    if (!file.is_open()) return false;
    
    string line;
    while (getline(file, line)) {
        if (line.empty()) continue;
        UserData user = deserializeUser(line);
        if (user.login == login) {
            return true;
        }
    }
    return false;
}

bool Database::checkUserPassword(const string& login, const string& password) const {
    UserData user = getUser(login);
    return user.login == login && user.password == password;
}

UserData Database::getUser(const string& login) const {
    UserData emptyUser;
    ifstream file(getUsersFilePath());
    if (!file.is_open()) return emptyUser;
    
    string line;
    while (getline(file, line)) {
        if (line.empty()) continue;
        UserData user = deserializeUser(line);
        if (user.login == login) {
            return user;
        }
    }
    return emptyUser;
}

vector<UserData> Database::getAllUsers() const {
    vector<UserData> users;
    ifstream file(getUsersFilePath());
    if (!file.is_open()) return users;
    
    string line;
    while (getline(file, line)) {
        if (line.empty()) continue;
        UserData user = deserializeUser(line);
        if (!user.login.empty()) {
            users.push_back(user);
        }
    }
    return users;
}

bool Database::updateUser(const UserData& user) {
    vector<UserData> allUsers = getAllUsers();
    bool found = false;
    
    for (auto& u : allUsers) {
        if (u.login == user.login) {
            u = user;
            found = true;
            break;
        }
    }
    
    if (!found) return false;
    
    ofstream file(getUsersFilePath());
    if (!file.is_open()) return false;
    
    for (const auto& u : allUsers) {
        file << serializeUser(u) << "\n";
    }
    
    return file.good();
}

bool Database::addMessage(const MessageData& message) {
    ofstream file(getMessagesFilePath(), ios::app);
    if (!file.is_open()) return false;
    
    file << serializeMessage(message) << "\n";
    return file.good();
}

vector<MessageData> Database::getAllMessages() const {
    vector<MessageData> messages;
    ifstream file(getMessagesFilePath());
    if (!file.is_open()) return messages;
    
    string line;
    while (getline(file, line)) {
        if (line.empty()) continue;
        MessageData msg = deserializeMessage(line);
        if (!msg.text.empty()) {
            messages.push_back(msg);
        }
    }
    return messages;
}

vector<MessageData> Database::getMessagesForUser(const string& login) const {
    vector<MessageData> allMessages = getAllMessages();
    vector<MessageData> userMessages;
    
    for (const auto& msg : allMessages) {
        if (msg.type == "SYSTEM" || 
            msg.senderLogin == login || 
            msg.recipientLogin == login ||
            (msg.type == "PUBLIC" && msg.recipientLogin.empty())) {
            userMessages.push_back(msg);
        }
    }
    
    return userMessages;
}

bool Database::addFriend(const string& userLogin, const string& friendLogin) {
    UserData user = getUser(userLogin);
    if (user.login.empty()) return false;
    
    if (find(user.friends.begin(), user.friends.end(), friendLogin) == user.friends.end()) {
        user.friends.push_back(friendLogin);
        return updateUser(user);
    }
    return true;
}

bool Database::removeFriend(const string& userLogin, const string& friendLogin) {
    UserData user = getUser(userLogin);
    if (user.login.empty()) return false;
    
    auto it = find(user.friends.begin(), user.friends.end(), friendLogin);
    if (it != user.friends.end()) {
        user.friends.erase(it);
        return updateUser(user);
    }
    return true;
}

vector<string> Database::getUserFriends(const string& login) const {
    UserData user = getUser(login);
    return user.friends;
}

